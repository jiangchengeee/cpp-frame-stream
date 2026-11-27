// Buffer 测试：追加 / 查看 / 消费 / 从 socket 读
#include "buffer.h"

#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <string>

#define CHECK(cond)                                                   \
    do {                                                              \
        if (!(cond)) {                                                \
            std::printf("  ✗ FAIL 第 %d 行：%s\n", __LINE__, #cond);  \
            return 1;                                                 \
        }                                                             \
    } while (0)

int main() {
    std::puts("[测试 1] append 之后能读到，readable 长度正确");
    {
        Buffer b;
        b.append("hello", 5);
        CHECK(b.readable() == 5);
        CHECK(std::memcmp(b.peek(), "hello", 5) == 0);
    }
    std::puts("  ✓ 通过");

    std::puts("[测试 2] retrieve 之后，剩余数据从新位置开始");
    {
        Buffer b;
        b.append("abcdef", 6);
        b.retrieve(2);                    // 消费 "ab"
        CHECK(b.readable() == 4);
        CHECK(std::memcmp(b.peek(), "cdef", 4) == 0);
        b.retrieve(4);                    // 全部消费
        CHECK(b.readable() == 0);
    }
    std::puts("  ✓ 通过");

    std::puts("[测试 3] 多次 append 拼接（模拟粘包累积）");
    {
        Buffer b;
        b.append("he", 2);
        b.append("llo", 3);
        CHECK(b.readable() == 5);
        CHECK(std::memcmp(b.peek(), "hello", 5) == 0);
    }
    std::puts("  ✓ 通过");

    std::puts("[测试 4] read_fd 从 socket 读数据");
    {
        int sv[2];
        CHECK(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);
        Buffer b;
        CHECK(::write(sv[1], "frame-data", 10) == 10);
        ssize_t n = b.read_fd(sv[0]);
        CHECK(n == 10);
        CHECK(b.readable() == 10);
        CHECK(std::memcmp(b.peek(), "frame-data", 10) == 0);
        ::close(sv[0]);
        ::close(sv[1]);
    }
    std::puts("  ✓ 通过");

    std::puts("[测试 5] 半包：数据没到齐时，只能读到已有的部分");
    {
        int sv[2];
        CHECK(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);
        Buffer b;
        CHECK(::write(sv[1], "abc", 3) == 3);
        CHECK(b.read_fd(sv[0]) == 3);
        CHECK(b.readable() == 3);          // 只有 3 字节，凑不成完整帧
        CHECK(::write(sv[1], "def", 3) == 3);
        CHECK(b.read_fd(sv[0]) == 3);
        CHECK(b.readable() == 6);          // 现在 6 字节都到了
        ::close(sv[0]);
        ::close(sv[1]);
    }
    std::puts("  ✓ 通过");

    std::puts("\n全部测试通过 ✅");
    return 0;
}
