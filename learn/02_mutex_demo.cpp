// 实验 2：多线程同时改一个变量，不加锁会发生什么
// 跑法：./dev.sh "g++ -std=c++17 -O2 -pthread learn/02_mutex_demo.cpp -o /tmp/m && /tmp/m"
#include <cstdio>
#include <thread>
#include <mutex>

volatile long long counter = 0; // 两个线程都要改它（volatile 阻止编译器把循环优化掉）
std::mutex m;                   // 一把锁

void add_without_lock() {
    for (int i = 0; i < 1000000; ++i) counter += 1;   // 没有锁
}

void add_with_lock() {
    for (int i = 0; i < 1000000; ++i) {
        std::lock_guard<std::mutex> g(m);              // 加锁：离开作用域自动解锁（就是昨天学的 RAII）
        counter += 1;
    }
}

int main() {
    // 第一轮：不加锁
    counter = 0;
    std::thread t1(add_without_lock), t2(add_without_lock);
    t1.join(); t2.join();
    std::printf("不加锁：期望 2000000，实际 %lld\n", counter);

    // 第二轮：加锁
    counter = 0;
    std::thread t3(add_with_lock), t4(add_with_lock);
    t3.join(); t4.join();
    std::printf("加锁了：期望 2000000，实际 %lld\n", counter);
    return 0;
}
