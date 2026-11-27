// Channel：一个 fd + 它关心的事件 + 一组回调
//
// 为什么需要它：裸 fd 太"素"了——只有数字，没有"我是谁、我关心什么、我出事了找谁"。
// Channel 把这三样捆在一起，EventLoop 只要调 handle_event() 就行。
#pragma once

#include <cstdint>
#include <functional>
#include <utility>

class EventLoop;   // 前向声明（Channel 只知道"有个 loop"，不需要它的完整定义）

class Channel {
public:
    Channel(EventLoop* loop, int fd) : loop_(loop), fd_(fd) {}

    int fd() const { return fd_; }
    std::uint32_t events() const { return events_; }      // 当前关心的事件位
    bool in_poller() const { return in_poller_; }         // 是否已在 epoll 里注册
    void set_in_poller(bool v) { in_poller_ = v; }

    void set_read_cb(std::function<void()> cb)  { read_cb_ = std::move(cb); }
    void set_write_cb(std::function<void()> cb) { write_cb_ = std::move(cb); }
    void set_error_cb(std::function<void()> cb) { error_cb_ = std::move(cb); }

    // ---------------- TODO 1~4：四个开关 ----------------
    // 目标：改事件位，然后通知 EventLoop 去更新 epoll
    // 每个函数都是两步：
    //   enable_reading :  ① events_ |= EPOLLIN;   ② loop_->update_channel(this);
    //   disable_reading:  ① events_ &= ~EPOLLIN;  ② loop_->update_channel(this);
    //   enable_writing :  ① events_ |= EPOLLOUT;  ② loop_->update_channel(this);
    //   disable_writing:  ① events_ &= ~EPOLLOUT; ② loop_->update_channel(this);
    void enable_reading();
    void disable_reading();
    void enable_writing();
    void disable_writing();

    // ---------------- TODO 5：事件分发 ----------------
    // 目标：EventLoop 把"这个 fd 上发生了什么"交给你，你按位判断该调哪个回调
    // 步骤：
    //   ① if (revents & EPOLLERR) { if (error_cb_) error_cb_(); return; }
    //   ② if (revents & (EPOLLIN | EPOLLPRI | EPOLLHUP)) { if (read_cb_) read_cb_(); }
    //   ③ if (revents & EPOLLOUT) { if (write_cb_) write_cb_(); }
    // 注意：回调可能没设置，所以每个调用前都要判 if (xxx_cb_)
    void handle_event(std::uint32_t revents);

private:
    EventLoop* loop_;
    int fd_;
    std::uint32_t events_{0};       // 位标记：EPOLLIN | EPOLLOUT | ...
    bool in_poller_{false};         // 是否已经注册进 epoll（防止重复 add / 忘 del）
    std::function<void()> read_cb_, write_cb_, error_cb_;
};
