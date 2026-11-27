// frame_stream —— 用四层 Reactor 实现的回显服务器
//
// 结构：
//   EventLoop  ← 事件循环（一条线程）
//     └ Poller ← epoll 封装
//         └ Channel ← 每个 fd 的"工单本"（关心什么事件 + 回调）
//             └ Buffer ← 每个连接的读写缓冲（解决粘包/半包）
//
// 运行：
//   ./build/frame_stream          启动服务器，监听 127.0.0.1:9999
//   ./build/frame_stream selftest 自测：起服务器 + 内置客户端，验证回显
#include "buffer.h"
#include "channel.h"
#include "event_loop.h"

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
#include <map>
#include <memory>
#include <string>
#include <thread>

static void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// 一条客户端连接：Channel + Buffer
struct Session {
    Buffer in;                       // 收到的数据
    std::unique_ptr<Channel> channel; // 这条连接的事件簿
};

class EchoServer {
public:
    EchoServer(EventLoop* loop, int port) : loop_(loop) {
        listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        addr.sin_port = htons((uint16_t)port);
        if (::bind(listen_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
            std::perror("bind");
            return;
        }
        ::listen(listen_fd_, 64);
        set_nonblock(listen_fd_);

        accept_channel_ = std::make_unique<Channel>(loop_, listen_fd_);
        accept_channel_->set_read_cb([this] { on_accept(); });
        accept_channel_->enable_reading();
        std::printf("[server] listen 127.0.0.1:%d\n", port);
        std::fflush(stdout);
    }

    ~EchoServer() {
        for (auto& [fd, s] : sessions_) close_session(fd);
        if (listen_fd_ >= 0) ::close(listen_fd_);
    }

private:
    void on_accept() {
        while (true) {
            int conn = ::accept(listen_fd_, nullptr, nullptr);
            if (conn < 0) break;                 // 非阻塞：没有更多连接了
            set_nonblock(conn);
            auto s = std::make_unique<Session>();
            s->channel = std::make_unique<Channel>(loop_, conn);
            Channel* ch = s->channel.get();
            s->channel->set_read_cb([this, ch, fd = conn] { on_readable(fd); });
            s->channel->enable_reading();
            sessions_[conn] = std::move(s);
            std::printf("[server] 新连接 fd=%d（当前 %zu 条）\n", conn, sessions_.size());
            std::fflush(stdout);
        }
    }

    void on_readable(int fd) {
        auto it = sessions_.find(fd);
        if (it == sessions_.end()) return;
        Session& s = *it->second;

        ssize_t n = s.in.read_fd(fd);            // 读进 Buffer
        if (n > 0) {
            std::printf("[server] fd=%d 收到 %zd 字节\n", fd, n);
            std::fflush(stdout);
            // echo：把缓冲区里可读的字节原样写回去
            ::write(fd, s.in.peek(), s.in.readable());
            s.in.retrieve(s.in.readable());
        } else if (n == 0) {
            std::printf("[server] fd=%d 对端关闭\n", fd);
            std::fflush(stdout);
            close_session(fd);
        } else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) close_session(fd);
        }
    }

    void close_session(int fd) {
        sessions_.erase(fd);                     // Channel 的析构会把它从 epoll 里摘掉
        ::close(fd);
    }

    EventLoop* loop_;
    int listen_fd_{-1};
    std::unique_ptr<Channel> accept_channel_;
    std::map<int, std::unique_ptr<Session>> sessions_;
};

// 自测客户端：连接、发 3 条消息、验证回显
static bool selftest_client(int port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons((uint16_t)port);
    if (::connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::perror("connect");
        return false;
    }
    bool ok = true;
    char buf[256];
    for (int i = 0; i < 3; ++i) {
        std::string msg = "frame-" + std::to_string(i);
        if (::write(fd, msg.data(), msg.size()) != (ssize_t)msg.size()) { ok = false; break; }
        ssize_t r = ::read(fd, buf, sizeof(buf));
        if (r <= 0) { ok = false; break; }
        buf[r] = '\0';
        std::printf("[client] 发 %-8s 收到 %s\n", msg.c_str(), buf);
        if (msg != buf) ok = false;
    }
    ::close(fd);
    return ok;
}

int main(int argc, char** argv) {
    const int kPort = 9999;
    const bool selftest = (argc > 1 && std::strcmp(argv[1], "selftest") == 0);

    EventLoop loop;
    EchoServer server(&loop, kPort);

    if (selftest) {
        // 起一个线程跑事件循环，主线程当客户端
        std::thread loop_thread([&] { loop.loop(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        bool ok = selftest_client(kPort);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        loop.quit();
        loop_thread.join();
        std::puts(ok ? "[main] selftest OK ✅" : "[main] selftest FAILED ❌");
        return ok ? 0 : 1;
    }

    loop.loop();     // 服务器模式：一条线程跑到 quit
    return 0;
}
