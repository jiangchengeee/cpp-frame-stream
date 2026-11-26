// Poller：把 epoll 的三个函数封装成一个类
//
// 职责（四层架构的第一层）：
//   - 管理"哪些 fd 被关注、关注什么事件"
//   - 等事件（epoll_wait），把就绪的 fd 交给上层
//   - 记住每个 fd 对应的处理函数（Handler）
//
// 对应 epoll 三件套：
//   构造 → epoll_create1
//   add/mod/del → epoll_ctl(ADD/MOD/DEL)
//   wait → epoll_wait
#pragma once

#include <sys/epoll.h>
#include <unistd.h>

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

class Poller {
public:
    using Handler = std::function<void(std::uint32_t events)>;   // 一个 fd 的事件处理函数
    using Ready = std::pair<int, std::uint32_t>;                 // (fd, 发生了什么事件)

    // ---------------- TODO 1：构造 / 析构 ----------------
    // 构造：创建 epoll 实例，并把事件数组预分配好
    //   Poller() : ep_(::epoll_create1(0)), evs_(64) {}
    // 析构：关闭 epoll fd
    //   ~Poller() { if (ep_ >= 0) ::close(ep_); }
    // 还要禁止拷贝（两个对象不能管同一个 epoll fd）：
    //   Poller(const Poller&) = delete;
    //   Poller& operator=(const Poller&) = delete;
    Poller() : ep_(::epoll_create1(0)), evs_(64) {}
    ~Poller() { if (ep_ >= 0) ::close(ep_); }

    Poller(const Poller&) = delete;
    Poller& operator=(const Poller&) = delete;

    // ---------------- TODO 2：注册 fd ----------------
    // 目标：把 fd 加进 epoll 关注 events，同时记住它的 Handler
    // 步骤：
    //   ① epoll_event ev{}; ev.events = events; ev.data.fd = fd;
    //   ② if (::epoll_ctl(ep_, EPOLL_CTL_ADD, fd, &ev) < 0) return false;
    //   ③ handlers_[fd] = std::move(h);   // 记住这个 fd 的处理函数
    //   ④ return true;
    bool add(int fd, std::uint32_t events, Handler h) {
        // (void)fd; (void)events; (void)h;
        // // TODO 2
        // return false;
        epoll_event ev{};ev.events=events;ev.data.fd=fd;
        if(::epoll_ctl(ep_,EPOLL_CTL_ADD,fd,&ev)<0)return false;
        handlers_[fd]=std::move(h);
        return true;
    }

    // ---------------- TODO 3：修改关注的事件 ----------------
    // 步骤：构造 epoll_event 后调用 ::epoll_ctl(ep_, EPOLL_CTL_MOD, fd, &ev) == 0
    bool mod(int fd, std::uint32_t events) {
        (void)fd; (void)events;
        // TODO 3
        return false;
    }

    // ---------------- TODO 4：取消关注 ----------------
    // 步骤：
    //   ① handlers_.erase(fd);                                   // 忘掉处理函数
    //   ② return ::epoll_ctl(ep_, EPOLL_CTL_DEL, fd, nullptr) == 0;
    bool del(int fd) {
        // (void)fd;
        // // TODO 4
        // return false;
        handlers_.erase(fd);
        return ::epoll_ctl(ep_,EPOLL_CTL_DEL,fd,nullptr)==0;
    }

    // ---------------- TODO 5：等事件并把处理函数跑起来 ----------------
    // 目标：epoll_wait 拿到 n 个就绪事件，逐个调用它注册的 Handler
    // 步骤：
    //   ① int n = ::epoll_wait(ep_, evs_.data(), (int)evs_.size(), timeout_ms);
    //   ② for (int i = 0; i < n; ++i) {
    //          auto it = handlers_.find(evs_[i].data.fd);
    //          if (it != handlers_.end()) it->second(evs_[i].events);
    //      }
    //   ③ return n;
    int dispatch(int timeout_ms) {
        // (void)timeout_ms;
        // // TODO 5
        // return 0;
        int n=::epoll_wait(ep_,evs_.data(),(int)evs_.size(),timeout_ms);
        for(int i=0;i<n;++i){
            auto it=handlers_.find(evs_[i].data.fd);
            if(it!=handlers_.end())it->second(evs_[i].events);
        }
        return n;
    }

    // 已实现：等事件但不分发，把 (fd, events) 装进 out（测试里会用到）
    int wait(int timeout_ms, std::vector<Ready>& out) {
        int n = ::epoll_wait(ep_, evs_.data(), (int)evs_.size(), timeout_ms);
        out.clear();
        for (int i = 0; i < n; ++i) out.emplace_back(evs_[i].data.fd, evs_[i].events);
        return n;
    }

private:
    int ep_;                                       // epoll 实例（本质也是一个 fd）
    std::vector<epoll_event> evs_;                 // epoll_wait 的输出缓冲
    std::unordered_map<int, Handler> handlers_;    // fd → 处理函数
};
