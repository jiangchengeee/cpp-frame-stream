// 线程池测试：跑通说明 TODO 全对
#include "thread_pool.h"

#include <atomic>
#include <cstdio>

#define CHECK(cond)                                                   \
    do {                                                              \
        if (!(cond)) {                                                \
            std::printf("  ✗ FAIL 第 %d 行：%s\n", __LINE__, #cond);  \
            return 1;                                                 \
        }                                                             \
    } while (0)

int main() {
    std::puts("[测试 1] 提交 10000 个任务，全部被执行");
    {
        ThreadPool pool(4, 2048);
        std::atomic<int> counter{0};
        for (int i = 0; i < 10000; ++i)
            CHECK(pool.submit([&counter] { counter.fetch_add(1); }));
        pool.shutdown();                       // 等任务干完
        CHECK(counter.load() == 10000);
    }
    std::puts("  ✓ 通过");

    std::puts("[测试 2] shutdown 之后 submit 应该失败");
    {
        ThreadPool pool(2);
        pool.shutdown();
        CHECK(!pool.submit([] {}));
    }
    std::puts("  ✓ 通过");

    std::puts("[测试 3] 优雅退出：析构时不能丢任务");
    {
        std::atomic<int> counter{0};
        {
            ThreadPool pool(4, 256);
            for (int i = 0; i < 5000; ++i)
                pool.submit([&counter] { counter.fetch_add(1); });
        }                                       // 这里线程池被析构，内部会 shutdown + join
        CHECK(counter.load() == 5000);          // 一个任务都不能少
    }
    std::puts("  ✓ 通过");

    std::puts("[测试 4] worker 数量正确");
    {
        ThreadPool pool(3);
        CHECK(pool.worker_count() == 3);
    }
    std::puts("  ✓ 通过");

    std::puts("\n全部测试通过 ✅");
    return 0;
}
