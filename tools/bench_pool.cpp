// 线程池吞吐测试：对比 1 / 2 / 4 / 8 个 worker 的吞吐
// 跑法：./dev.sh "make bench-pool"
#include "thread_pool.h"

#include <atomic>
#include <chrono>
#include <cstdio>

using clk = std::chrono::steady_clock;

static double ms_since(clk::time_point t0) {
    return std::chrono::duration<double, std::milli>(clk::now() - t0).count();
}

int main() {
    const int N = 1000000;   // 100 万个任务

    std::printf("任务数 %d\n", N);
    std::printf("%-8s %-16s %-16s\n", "workers", "轻任务(万/秒)", "重任务(万/秒)");
    std::printf("-----------------------------------------------\n");

    for (int w : {1, 2, 4, 8}) {
        // 轻任务：只做一次原子加（考察线程池本身的开销）
        std::atomic<long long> c1{0};
        {
            ThreadPool pool(w, 4096);
            auto t0 = clk::now();
            for (int i = 0; i < N; ++i) pool.submit([&c1] { c1.fetch_add(1, std::memory_order_relaxed); });
            pool.shutdown();
            double ms = ms_since(t0);
            std::printf("%-8d %-16.1f", w, N / 10000.0 / (ms / 1000.0));
        }

        // 重任务：每次做 ~2000 次浮点运算（考察真实并行加速比）
        std::atomic<long long> c2{0};
        {
            ThreadPool pool(w, 4096);
            auto t0 = clk::now();
            for (int i = 0; i < N / 10; ++i) {
                pool.submit([&c2] {
                    volatile double x = 1.0;
                    for (int k = 0; k < 2000; ++k) x = x * 1.000001 + 0.5;
                    (void)x;
                    c2.fetch_add(1, std::memory_order_relaxed);
                });
            }
            pool.shutdown();
            double ms = ms_since(t0);
            std::printf("%-16.1f\n", (N / 10) / 10000.0 / (ms / 1000.0));
        }
    }
    return 0;
}
