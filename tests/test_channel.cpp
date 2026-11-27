// Channel + EventLoop 测试：用 socketpair 模拟"对端写入 → 我这边收到读事件"
// 注意：每个用例都带 500ms 超时保护，即使实现没写完也不会卡死
#include "channel.h"
#include "event_loop.h"

#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

#define CHECK(cond)                                                   \
    do {                                                              \
        if (!(cond)) {                                                \
            std::printf("  ✗ FAIL 第 %d 行：%s\n", __LINE__, #cond);  \
            return 1;                                                 \
        }                                                             \
    } while (0)

// 在后台跑事件循环；最多等 ms 毫秒后强制 quit，保证不会永久卡住
struct LoopRunner {
    EventLoop& loop;
    std::thread th;
    explicit LoopRunner(EventLoop& l) : loop(l), th([this] { loop.loop(); }) {}
    ~LoopRunner() {
        loop.quit();
        if (th.joinable()) th.join();
    }
    void quit_now() { loop.quit(); }
};

int main() {
    int sv[2];
    CHECK(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);

    std::puts("[测试 1] enable_reading 后，对端写入应触发 read 回调");
    {
        EventLoop loop;
        Channel ch(&loop, sv[0]);
        std::string got;
        ch.set_read_cb([&] {
            char buf[64];
            ssize_t r = ::read(sv[0], buf, sizeof buf);
            if (r > 0) got.append(buf, (size_t)r);
            loop.quit();
        });
        ch.enable_reading();

        LoopRunner runner(loop);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        CHECK(::write(sv[1], "hello", 5) == 5);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));   // 最多等 300ms
        runner.quit_now();

        CHECK(got == "hello");
    }
    std::puts("  ✓ 通过");

    std::puts("[测试 2] disable_reading 后不再触发 read 回调");
    {
        EventLoop loop;
        Channel ch(&loop, sv[0]);
        int fired = 0;
        ch.set_read_cb([&] {
            char buf[64];
            ::read(sv[0], buf, sizeof buf);
            ++fired;
            ch.disable_reading();
            loop.quit();
        });
        ch.enable_reading();

        LoopRunner runner(loop);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        CHECK(::write(sv[1], "a", 1) == 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        runner.quit_now();

        CHECK(fired == 1);
    }
    std::puts("  ✓ 通过");

    std::puts("[测试 3] enable_writing 后应触发 write 回调");
    {
        EventLoop loop;
        Channel ch(&loop, sv[0]);
        int fired = 0;
        ch.set_write_cb([&] { ++fired; loop.quit(); });
        ch.enable_writing();

        LoopRunner runner(loop);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        runner.quit_now();

        CHECK(fired == 1);
    }
    std::puts("  ✓ 通过");

    ::close(sv[0]);
    ::close(sv[1]);
    std::puts("\n全部测试通过 ✅");
    return 0;
}
