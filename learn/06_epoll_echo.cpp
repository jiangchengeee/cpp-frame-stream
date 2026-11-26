// 实验 6：最小 epoll 回显服务器（一个线程做服务器，主线程当客户端）
// 跑法：./dev.sh "make learn-6"
//
// 这个程序只做三件事：
//   1. 创建监听 socket，放进 epoll
//   2. epoll_wait 等事件（有新连接 / 有数据可读）
//   3. 收到数据就原样发回去（echo）
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

static std::atomic<bool> g_stop{false};

// 把 fd 设成非阻塞
static void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// ---------------- 服务器 ----------------
static void server_thread() {
    // ① 创建监听 socket
    int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");   // 只监听本机
    addr.sin_port = htons(9999);
    if (::bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::perror("bind");
        return;
    }
    ::listen(listen_fd, 16);
    set_nonblock(listen_fd);                          // ② 非阻塞：accept 没连接时立刻返回
    std::printf("[server] 监听 127.0.0.1:9999\n");

    // ③ 创建 epoll，把监听 fd 加进去，关心"可读"（= 有新连接进来）
    int ep = ::epoll_create1(0);
    epoll_event ev{};
    ev.events = EPOLLIN;          // 关注可读事件
    ev.data.fd = listen_fd;
    ::epoll_ctl(ep, EPOLL_CTL_ADD, listen_fd, &ev);

    // ④ 事件循环
    epoll_event events[16];
    while (!g_stop.load()) {
        // 等事件，最多等 500ms（-1 = 一直等）
        int n = ::epoll_wait(ep, events, 16, 500);
        if (n < 0) { std::perror("epoll_wait"); break; }
        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            if (fd == listen_fd) {
                // ⑤ 有新连接：accept 出来，也加进 epoll
                while (true) {
                    int conn = ::accept(listen_fd, nullptr, nullptr);
                    if (conn < 0) break;              // 非阻塞，没连接了就退出循环
                    set_nonblock(conn);
                    epoll_event cev{};
                    cev.events = EPOLLIN;
                    cev.data.fd = conn;
                    ::epoll_ctl(ep, EPOLL_CTL_ADD, conn, &cev);
                    std::printf("[server] 新连接 fd=%d\n", conn);
                }
            } else {
                // ⑥ 有数据可读：读出来，原样写回去
                char buf[1024];
                ssize_t r = ::read(fd, buf, sizeof(buf));
                if (r > 0) {
                    std::printf("[server] 收到 %zd 字节：%.*s\n", r, (int)r, buf);
                    ::write(fd, buf, (size_t)r);       // echo
                } else if (r == 0) {
                    std::printf("[server] 连接关闭 fd=%d\n", fd);
                    ::epoll_ctl(ep, EPOLL_CTL_DEL, fd, nullptr);
                    ::close(fd);
                }
            }
        }
    }
    ::close(listen_fd);
    ::close(ep);
}

// ---------------- 客户端（放在主线程，方便对比）----------------
static void client() {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(9999);
    if (::connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) { std::perror("connect"); return; }

    const char* msgs[] = {"hello-1", "hello-2", "hello-3"};
    char buf[1024];
    for (const char* m : msgs) {
        ::write(fd, m, std::strlen(m));
        ssize_t r = ::read(fd, buf, sizeof(buf));
        if (r > 0) {
            buf[r] = '\0';
            std::printf("[client] 发 %-8s 收到 %s\n", m, buf);
        }
    }
    ::close(fd);
}

int main() {
    std::thread srv(server_thread);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));   // 等服务器起来
    client();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    g_stop = true;
    srv.join();
    std::puts("[main] 结束");
    return 0;
}
