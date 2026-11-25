// 线程池：固定数量的 worker 线程 + 一个任务队列
//
// 你昨天写的 BoundedQueue 在这里当"任务队列"用，T = std::function<void()>（任意可调用对象）
// 需要填 4 个 TODO，都是小段代码。
#pragma once

#include "bounded_queue.h"

#include <atomic>
#include <cstddef>
#include <functional>
#include <thread>
#include <utility>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(std::size_t worker_count, std::size_t queue_cap = 1024)
        : q_(queue_cap) {
        // ---------------- TODO 1：启动 N 个 worker 线程 ----------------
        // 步骤：
        //   for (std::size_t i = 0; i < worker_count; ++i)
        //       workers_.emplace_back([this] { worker_loop(); });
        // 说明：每个线程都去跑 worker_loop()，进去后就会阻塞在 q_.pop() 上等任务
        for(std::size_t i=0;i<worker_count;++i)
           workers_.emplace_back([this]{worker_loop();} );
    }

    ~ThreadPool() { shutdown(); }

    // ---------------- TODO 2：提交任务 ----------------
    // 目标：把 task 放进队列，worker 会取走执行。队列已关闭返回 false。
    // 步骤：
    //   ① return q_.push(std::move(task));
    bool submit(std::function<void()> task) {
        // (void)task;
        return q_.push(std::move(task));
        // return false;
    }

    // ---------------- TODO 3：优雅关闭 ----------------
    // 目标：让所有 worker 干完手上的活后退出，并等它们真的退出（join）
    // 步骤：
    //   ① if (stopped_.exchange(true)) return;   // 已经关过了就别重复关（析构函数也会调这个方法）
    //   ② q_.shutdown();                        // 通知队列：不再收任务，把等待的 worker 全部唤醒
    //   ③ for (auto& t : workers_) if (t.joinable()) t.join();   // 等每个 worker 真正结束
    void shutdown() {
        // TODO 3: 在这里写上面 ①~③
        if(stopped_.exchange(true))return ;
        q_.shutdown();
        for(auto& t:workers_)if(t.joinable())t.join();
    }

    std::size_t pending() const { return q_.size(); }
    std::size_t worker_count() const { return workers_.size(); }

private:
    // ---------------- TODO 4：worker 的主循环 ----------------
    // 目标：不停从队列取任务并执行，直到队列关闭且取空
    // 步骤：
    //   std::function<void()> task;
    //   while (q_.pop(task)) {
    //       task();          // 执行任务
    //   }
    // 说明：q_.pop() 在「队列关闭且为空」时返回 false，循环自然结束 → 线程退出
    void worker_loop() {
        // TODO 4: 在这里写上面这段 while 循环
        std::function<void()>task;
        while(q_.pop(task)){
            task();
        }
    }

    BoundedQueue<std::function<void()>> q_;
    std::vector<std::thread> workers_;
    std::atomic<bool> stopped_{false};
};
