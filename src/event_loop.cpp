#include "event_loop.h"

#include "channel.h"

#include <sys/epoll.h>

// TODO 6：三个分支（见 event_loop.h 注释里的 ①~③）
void EventLoop::update_channel(Channel* ch) {
    // (void)ch;
    std::uint32_t ev=ch->events();
    int fd=ch->fd();
    if(ev==0&&ch->in_poller()){poller_.del(fd);ch->set_in_poller(false);}
    else if(!ch->in_poller()&&ev!=0){
        Channel*c=ch;
        poller_.add(fd, ev, [c](std::uint32_t revents) { c->handle_event(revents); });
        ch->set_in_poller(true);
    }else if(ch->in_poller()&&ev!=0){
        poller_.mod(fd,ev);
    }
}
