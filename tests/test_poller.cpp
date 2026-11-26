// Poller 测试：用 socketpair 模拟"一端写、一端可读"
#include "poller.h"

#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <vector>

#define CHECK(cond)                                                   \
    do {                                                              \
        if (!(cond)) {                                                \
            std::printf("  ✗ FAIL 第 %d 行：%s\n", __LINE__, #cond);  \
            return 1;                                                 \
        }                                                             \
    } while (0)

int main() {
    int sv[2];
    CHECK(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);

    std::puts("[测试 1] 注册 fd，没有数据时 wait 超时返回 0");
    Poller p;
    std::vector<Poller::Ready> ready;
    CHECK(p.add(sv[0], EPOLLIN, [](std::uint32_t) {}));
    CHECK(p.wait(50, ready) == 0);
    std::puts("  ✓ 通过");

    std::puts("[测试 2] 对端写入后，wait 应返回 1 个 EPOLLIN 事件");
    CHECK(::write(sv[1], "x", 1) == 1);
    CHECK(p.wait(500, ready) == 1);
    CHECK((ready[0].second & EPOLLIN) != 0);
    CHECK(ready[0].first == sv[0]);
    std::puts("  ✓ 通过");

    std::puts("[测试 3] dispatch 应调用注册的 Handler");
    int fired = 0;
    CHECK(p.del(sv[0]) || true);
    CHECK(p.add(sv[0], EPOLLIN, [&fired](std::uint32_t ev) { if (ev & EPOLLIN) ++fired; }));
    int n = p.dispatch(500);
    CHECK(n == 1);
    CHECK(fired == 1);
    std::puts("  ✓ 通过");

    std::puts("[测试 4] del 之后不再报事件");
    CHECK(p.del(sv[0]));
    CHECK(::write(sv[1], "y", 1) == 1);
    CHECK(p.wait(100, ready) == 0);
    std::puts("  ✓ 通过");

    ::close(sv[0]);
    ::close(sv[1]);
    std::puts("\n全部测试通过 ✅");
    return 0;
}
