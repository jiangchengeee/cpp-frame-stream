// 实验 3：同一个问题的三种解法，比一比
// 跑法：./dev.sh "g++ -std=c++17 -O2 -pthread learn/03_atomic_demo.cpp -o /tmp/a3 && /tmp/a3"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <thread>

constexpr int N = 5000000;          // 每个线程加 500 万次

volatile long long plain = 0;        // 普通变量（volatile 防止编译器把循环折叠成一次加法）
volatile long long locked = 0;      // 普通变量 + 锁
std::mutex m;
std::atomic<long long> atom(0);     // 原子变量（硬件保证不被打断）

using clk = std::chrono::steady_clock;
double ms(clk::time_point a, clk::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

int main() {
    // ① 普通变量，两个线程直接加
    auto t0 = clk::now();
    { std::thread a([]{ for (int i = 0; i < N; ++i) ++plain; });
      std::thread b([]{ for (int i = 0; i < N; ++i) ++plain; });
      a.join(); b.join(); }
    auto t1 = clk::now();
    std::printf("① 不加锁  结果=%lld（应为 %d）耗时 %.1f ms  ← 可能丢更新\n", plain, 2 * N, ms(t0, t1));

    // ② 普通变量 + 锁
    t0 = clk::now();
    { std::thread a([]{ for (int i = 0; i < N; ++i) { std::lock_guard<std::mutex> g(m); ++locked; } });
      std::thread b([]{ for (int i = 0; i < N; ++i) { std::lock_guard<std::mutex> g(m); ++locked; } });
      a.join(); b.join(); }
    t1 = clk::now();
    std::printf("② 加锁    结果=%lld（应为 %d）耗时 %.1f ms  ← 正确但慢\n", (long long)locked, 2 * N, ms(t0, t1));

    // ③ 原子变量，不写锁
    t0 = clk::now();
    { std::thread a([]{ for (int i = 0; i < N; ++i) ++atom; });
      std::thread b([]{ for (int i = 0; i < N; ++i) ++atom; });
      a.join(); b.join(); }
    t1 = clk::now();
    std::printf("③ 原子变量 结果=%lld（应为 %d）耗时 %.1f ms  ← 正确且快\n", atom.load(), 2 * N, ms(t0, t1));
    return 0;
}
