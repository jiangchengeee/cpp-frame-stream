// EventLoop：一条线程的"事件循环"
//
// 它干的事就一件：不停地问 Poller "有事件吗" → 有就让对应 Channel 处理
// loop() 已经写好，你只需要填 update_channel（让 Channel 的开关能真的影响 epoll）
#pragma once

#include "poller.h"

#include <atomic>
#include <cstdint>

class Channel;

class EventLoop {
public:
    EventLoop() = default;
    ~EventLoop() { quit(); }

    // 事件循环：一直转，直到 quit() 被调用
    void loop() {
        while (!quit_) poller_.dispatch(100);   // 100ms 超时，让 quit 有机会被看到
    }

    void quit() { quit_ = true; }

    // ---------------- TODO 6：让 Channel 的事件变化同步到 epoll ----------------
    // 三个分支（这是整个 Reactor 最容易写错的地方）：
    //   std::uint32_t ev = ch->events();
    //   int fd = ch->fd();
    //
    //   ① 不再关心任何事件，但已注册 → 从 epoll 删掉
    //      if (ev == 0 && ch->in_poller()) { poller_.del(fd); ch->set_in_poller(false); }
    //
    //   ② 关心事件，但还没注册 → 注册进去（并把"事件来了就调 handle_event"记成回调）
    //      else if (!ch->in_poller() && ev != 0) {
    //          Channel* c = ch;   // 先存到局部变量，lambda 捕获它（捕获 this 会 dangling）
    //          poller_.add(fd, ev, [c](std::uint32_t revents) { c->handle_event(revents); });
    //          ch->set_in_poller(true);
    //      }
    //
    //   ③ 已注册且还要关心 → 修改关注的事件
    //      else if (ch->in_poller() && ev != 0) { poller_.mod(fd, ev); }
    void update_channel(Channel* ch);

private:
    Poller poller_;
    std::atomic<bool> quit_{false};
};
