// frame_stream —— 四层 Reactor + 线程池的帧流处理服务
//
// 数据流：
//   TCP 字节 → Buffer 攒 → frame_codec 切帧 → ThreadPool（有界队列）
//            → 业务处理 → encode_frame → 写回连接
//   队列满时：直接丢弃该帧并计数（这就是背压的第一种策略：丢帧）
#include "buffer.h"
#include "channel.h"
#include "event_loop.h"
#include "frame_codec.h"
#include "thread_pool.h"

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

struct Session {
    Buffer in;
    std::unique_ptr<Channel> channel;
};

class FrameServer {
public:
    FrameServer(EventLoop* loop, int port, int workers, std::size_t queue_cap)
        : loop_(loop), pool_(workers, queue_cap) {
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
        ::listen(listen_fd_, 128);
        set_nonblock(listen_fd_);

        accept_channel_ = std::make_unique<Channel>(loop_, listen_fd_);
        accept_channel_->set_read_cb([this] { on_accept(); });
        accept_channel_->enable_reading();
        std::printf("[server] listen 127.0.0.1:%d, workers=%d, queue_cap=%zu\n",
                    port, workers, queue_cap);
        std::fflush(stdout);
    }

    ~FrameServer() {
        for (auto& kv : sessions_) ::close(kv.first);
        if (listen_fd_ >= 0) ::close(listen_fd_);
    }

    void stop() { pool_.shutdown(); }

    std::atomic<long long> processed_{0};
    std::atomic<long long> dropped_{0};

private:
    // ---------- 连接管理 ----------
    void on_accept() {
        while (true) {
            int conn = ::accept(listen_fd_, nullptr, nullptr);
            if (conn < 0) break;
            set_nonblock(conn);
            auto s = std::make_unique<Session>();
            s->channel = std::make_unique<Channel>(loop_, conn);
            s->channel->set_read_cb([this, fd = conn] { on_readable(fd); });
            s->channel->enable_reading();
            sessions_[conn] = std::move(s);
        }
    }

    // ---------- 收到数据：切帧 → 入队 ----------
    void on_readable(int fd) {
        auto it = sessions_.find(fd);
        if (it == sessions_.end()) return;
        Session& s = *it->second;

        ssize_t n = s.in.read_fd(fd);
        if (n > 0) {
            auto frames = decode_frames(s.in);            // 可能 0 条、1 条或多条
            for (auto& f : frames) {
                // 把「fd + payload」打包成一个任务丢给线程池
                std::string p = std::move(f);
                bool ok = pool_.try_submit([this, fd, p = std::move(p)]() mutable {
                    handle_frame(fd, p);
                });
                if (!ok) dropped_.fetch_add(1, std::memory_order_relaxed);  // ★ 背压：队列满→丢帧
            }
        } else if (n == 0) {
            close_session(fd);
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            close_session(fd);
        }
    }

    // ---------- worker 里执行：处理一帧并写回 ----------
    void handle_frame(int fd, const std::string& payload) {
        std::string result = process(payload);           // ← 业务处理（TODO 在下面）
        std::string frame = encode_frame(result);        // 处理结果重新打包成帧
        ::write(fd, frame.data(), frame.size());         // 写回（大 EAGAIN 时忽略）
        processed_.fetch_add(1, std::memory_order_relaxed);
    }

    // ============================================================
    // TODO（你写）：业务处理逻辑
    // 现在先原样返回，所以功能上等价于"帧回显"，但已经跑在 worker 线程里了。
    // 想一个"像传感器数据处理"的逻辑，例如：
    //   - 返回 payload 的长度信息："len=7"（最简单，能让客户端看出处理过）
    //   - 或者把 payload 反转、转大写、求简单校验和
    // 要求：处理结果仍是字符串，会被 encode_frame 包成帧写回。
    // ============================================================
    static std::string process(const std::string& payload) {
        return payload;
    }

    void close_session(int fd) {
        sessions_.erase(fd);
        ::close(fd);
    }

    EventLoop* loop_;
    int listen_fd_{-1};
    std::unique_ptr<Channel> accept_channel_;
    std::map<int, std::unique_ptr<Session>> sessions_;
    ThreadPool pool_;
};

// ---------------- 自测客户端（用帧协议收发）----------------
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

    Buffer in;
    bool ok = true;
    for (int i = 0; i < 3; ++i) {
        std::string msg = "frame-" + std::to_string(i);
        std::string frame = encode_frame(msg);
        if (::write(fd, frame.data(), frame.size()) != (ssize_t)frame.size()) { ok = false; break; }

        // 等回包：最多等 1 秒
        std::vector<std::string> got;
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
        while (got.empty() && std::chrono::steady_clock::now() < deadline) {
            in.read_fd(fd);
            got = decode_frames(in);
        }
        if (got.size() != 1) { ok = false; break; }
        std::printf("[client] 发 %-10s 收到长度=%zu 内容=%s\n",
                    msg.c_str(), got[0].size(), got[0].c_str());
        if (got[0] != msg) ok = false;     // 因为 process() 目前原样返回
    }
    ::close(fd);
    return ok;
}

int main(int argc, char** argv) {
    const int kPort = 9999;
    const bool selftest = (argc > 1 && std::strcmp(argv[1], "selftest") == 0);

    EventLoop loop;
    FrameServer server(&loop, kPort, /*workers=*/4, /*queue_cap=*/1024);

    // metrics 线程：每秒打印一次处理/丢弃情况
    std::atomic<bool> metrics_stop{false};
    std::thread metrics([&] {
        long long last = 0;
        while (!metrics_stop.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            long long now = server.processed_.load();
            std::printf("[metrics] 已处理=%lld (+%lld/秒)  丢弃=%lld\n",
                        now, now - last, server.dropped_.load());
            std::fflush(stdout);
            last = now;
        }
    });

    int rc = 0;
    if (selftest) {
        std::thread loop_thread([&] { loop.loop(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        bool ok = selftest_client(kPort);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        loop.quit();
        loop_thread.join();
        std::puts(ok ? "[main] selftest OK ✅" : "[main] selftest FAILED ❌");
        rc = ok ? 0 : 1;
    } else {
        loop.loop();
    }

    metrics_stop = true;
    metrics.join();
    server.stop();
    return rc;
}
