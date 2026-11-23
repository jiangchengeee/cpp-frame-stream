// 有界阻塞队列（BoundedQueue）
//
// 作用：固定容量的队列。
//   - 队列满了，push 会「等」，直到有位置或被 shutdown
//   - 队列空了，pop 会「等」，直到有数据或被 shutdown
//   - shutdown 之后，所有等待的线程都被唤醒并退出（这样线程池才能优雅退出）
//
// 你只需要填 4 个 TODO，每个 TODO 的步骤已经写在注释里，照着写即可。

#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <utility>

template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t cap) : cap_(cap), closed_(false) {}

    // ---------------- TODO 1：阻塞式入队 ----------------
    // 目标：有空位就放进去；满了就等；队列已关闭则返回 false。
    // 步骤（照着代码顺序写）：
    //   ① std::unique_lock<std::mutex> lock(m_);
    //   ② not_full_.wait(lock, [this] { return q_.size() < cap_ || closed_; });
    //      ——「等到有空位，或者队列已关闭」，不满足就一直睡在这行
    //   ③ if (closed_) return false;          // 关了就不收了
    //   ④ q_.push(std::move(v));              // 放进去
    //   ⑤ not_empty_.notify_one();            // 叫醒一个正在等的消费者
    //   ⑥ return true;
    bool push(T v) {
        // (void)v;
        // TODO 1: 在这里写上面 ①~⑥
        std::unique_lock<std::mutex>lock(m_);
        not_full_.wait(lock,[this]{return q_.size()<cap_||closed_;});
        if(closed_)return false;
        q_.push(std::move(v));
        not_empty_.notify_one();
        return true;
        // return false;
    }

    // ---------------- TODO 2：阻塞式出队 ----------------
    // 目标：有数据就取走；空了就等；队列已关闭且没数据时返回 false。
    // 步骤：
    //   ① std::unique_lock<std::mutex> lock(m_);
    //   ② not_empty_.wait(lock, [this] { return !q_.empty() || closed_; });
    //      ——「等到有数据，或者队列已关闭」
    //   ③ if (q_.empty()) return false;       // 关闭了且没数据，消费者该退出了
    //   ④ out = std::move(q_.front());        // 取出队头
    //      q_.pop();                           // 删掉队头
    //   ⑤ not_full_.notify_one();             // 叫醒一个正在等的生产者
    //   ⑥ return true;
    bool pop(T& out) {
        // (void)out;
        // TODO 2: 在这里写上面 ①~⑥
        std::unique_lock<std::mutex>lock(m_);
        not_empty_.wait(lock,[this]{return !q_.empty()||closed_;});
        if(q_.empty())return false;
        out=std::move(q_.front());
        q_.pop();
        not_full_.notify_one();
        return true;
        // return false;
    }

    // ---------------- TODO 3：非阻塞入队（做背压用） ----------------
    // 目标：有空位就放进去返回 true；满了立刻返回 false，绝不等待。
    // 步骤：
    //   ① { std::lock_guard<std::mutex> lock(m_);
    //        if (q_.size() >= cap_ || closed_) return false;
    //        q_.push(std::move(v));  }
    //      —— 注意大括号：锁的范围只有这几行，出了大括号就自动解锁（RAII）
    //   ② not_empty_.notify_one();
    //   ③ return true;
    bool try_push(T v) {
        // (void)v;
        // TODO 3: 在这里写上面 ①~③
        {   std::lock_guard<std::mutex>lock(m_);
            if(q_.size()>=cap_||closed_)return false;
            q_.push(std::move(v));   }
        not_empty_.notify_one();
        return true;
        // return false;
    }

    // ---------------- TODO 4：关闭队列（最简单的一个） ----------------
    // 目标：把所有正在等的线程叫醒，让它们发现 closed_ 为真后退出。
    // 步骤：
    //   ① { std::lock_guard<std::mutex> lock(m_); closed_ = true; }
    //   ② not_empty_.notify_all();   // 叫醒所有消费者
    //   ③ not_full_.notify_all();    // 叫醒所有生产者
    //
    // 想一想：为什么 notify 要写在锁外面？（提示：锁内唤醒会让对方立刻又卡在锁上，白跑一趟）
    void shutdown() {
        // TODO 4: 在这里写上面 ①~③
        {std::lock_guard<std::mutex>lock(m_);closed_=true;}
        not_empty_.notify_all();
        not_full_.notify_all();
    }

    // 当前元素个数（线程安全，已实现，你可以读它来理解 lock_guard 的用法）
    std::size_t size() const {
        std::lock_guard<std::mutex> lock(m_);
        return q_.size();
    }

private:
    mutable std::mutex m_;              // 保护下面的数据
    std::condition_variable not_full_;  // 「不满」条件：生产者在这等空位
    std::condition_variable not_empty_; // 「不空」条件：消费者在这等数据
    std::queue<T> q_;                   // 真正存数据的队列
    std::size_t cap_;                   // 容量上限
    bool closed_;                       // 是否已关闭
};
