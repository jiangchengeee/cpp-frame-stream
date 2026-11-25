// mini_unique_ptr 的测试：跑通说明你理解了 move
#include "mini_unique_ptr.h"

#include <cstdio>
#include <utility>

static int g_ctor = 0;
static int g_dtor = 0;

struct Probe {
    Probe() { ++g_ctor; }
    ~Probe() { ++g_dtor; }
};

#define CHECK(cond)                                                   \
    do {                                                              \
        if (!(cond)) {                                                \
            std::printf("  ✗ FAIL 第 %d 行：%s\n", __LINE__, #cond);  \
            return 1;                                                 \
        }                                                             \
    } while (0)

int main() {
    std::puts("[测试 1] 构造 + 离开作用域自动 delete");
    {
        MiniUniquePtr<Probe> p(new Probe());
        CHECK(p.get() != nullptr);
        CHECK((bool)p);
    }
    CHECK(g_ctor == 1);
    CHECK(g_dtor == 1);      // 离开作用域应该自动 delete 掉
    std::puts("  ✓ 通过");

    std::puts("[测试 2] 移动构造：所有权转移，原来的变空");
    {
        MiniUniquePtr<Probe> a(new Probe());
        MiniUniquePtr<Probe> b(std::move(a));     // TODO 1 正确才能通过
        CHECK(a.get() == nullptr);                // a 应该变空
        CHECK(b.get() != nullptr);                // b 接手
    }
    CHECK(g_ctor == 2);
    CHECK(g_dtor == 2);      // 只应该 delete 一次（没有 double free）
    std::puts("  ✓ 通过");

    std::puts("[测试 3] 移动赋值：老对象先释放，不泄漏");
    {
        MiniUniquePtr<Probe> a(new Probe());
        MiniUniquePtr<Probe> b(new Probe());
        b = std::move(a);                          // TODO 2 正确才能通过
        CHECK(a.get() == nullptr);
        CHECK(b.get() != nullptr);
    }
    CHECK(g_ctor == 4);
    CHECK(g_dtor == 4);      // 两个都释放了，不多不少
    std::puts("  ✓ 通过");

    std::puts("\n全部测试通过 ✅");
    return 0;
}
