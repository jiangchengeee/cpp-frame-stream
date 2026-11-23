// 有界队列的测试：跑通它，就说明你的实现是对的
// 跑法：./dev.sh "make test"
#include "bounded_queue.h"

#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

#define CHECK(cond)                                                  \
    do {                                                             \
        if (!(cond)) {                                               \
            std::printf("  ✗ FAIL  第 %d 行：%s\n", __LINE__, #cond); \
            return 1;                                                \
        }                                                            \
    } while (0)

int main() {
    std::puts("[测试 1] 基本 push / pop");
    {
        BoundedQueue<int> q(4);
        CHECK(q.push(1));
        CHECK(q.push(2));
        int v = 0;
        CHECK(q.pop(v) && v == 1);   // 先进先出：先放 1，先取 1
        CHECK(q.pop(v) && v == 2);
        CHECK(q.size() == 0);
    }
    std::puts("  ✓ 通过");

    std::puts("[测试 2] 队列满了，try_push 应该返回 false");
    {
        BoundedQueue<int> q(2);
        CHECK(q.try_push(1));
        CHECK(q.try_push(2));
        CHECK(!q.try_push(3));       // 满了，非阻塞版本立刻失败
    }
    std::puts("  ✓ 通过");

    std::puts("[测试 3] shutdown 之后，pop / push 都要失败");
    {
        BoundedQueue<int> q(2);
        q.shutdown();
        int v = 0;
        CHECK(!q.pop(v));            // 关闭且没数据 → 消费者退出
        CHECK(!q.push(1));           // 关闭 → 生产者不能再放
    }
    std::puts("  ✓ 通过");

    std::puts("[测试 4] 1 个生产者 + 2 个消费者，1..100 全部被消费完");
    {
        BoundedQueue<int> q(8);
        long long sum = 0;
        std::mutex sm;

        std::thread prod([&] {
            for (int i = 1; i <= 100; ++i) q.push(i);
            q.shutdown();                              // 生产完必须关闭，消费者才知道没有后续了
        });

        std::vector<std::thread> cons;
        for (int k = 0; k < 2; ++k)
            cons.emplace_back([&] {
                int v = 0;
                while (q.pop(v)) {                     // pop 返回 false = 已关闭且空了 → 退出
                    std::lock_guard<std::mutex> g(sm); // 多线程改 sum，要加锁
                    sum += v;
                }
            });

        prod.join();
        for (auto& t : cons) t.join();
        CHECK(sum == 5050);                            // 1+2+...+100 = 5050
    }
    std::puts("  ✓ 通过");

    // ------- TODO 5（自己加一个测试） -------
    // 要求：验证「队列满了之后 push 会阻塞，直到有人 pop 取出元素」
    // 思路：容量 1 的队列 → 放 1 个 → 开一个线程去 pop（它会拿到元素）→ 主线程 push 应该能成功
    // 或者：开线程 push 第 2 个（它会被阻塞）→ 主线程 pop 掉一个 → 那个线程就能完成了
    // 自己写，跑通就算完成

    std::puts("\n全部测试通过 ✅");
    return 0;
}
