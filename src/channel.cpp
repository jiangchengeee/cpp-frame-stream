#include "channel.h"

#include "event_loop.h"

#include <sys/epoll.h>

// TODO 1~4：四个开关（每个两行，见 channel.h 注释）
void Channel::enable_reading() {
    // TODO 1: events_ |= EPOLLIN; 然后通知 loop
    events_|=EPOLLIN;
    loop_->update_channel(this);
}

void Channel::disable_reading() {
    // TODO 2: events_ &= ~EPOLLIN; 然后通知 loop
    events_&=~EPOLLIN;
    loop_->update_channel(this);
}

void Channel::enable_writing() {
    // TODO 3: events_ |= EPOLLOUT; 然后通知 loop
    events_|=EPOLLOUT;
    loop_->update_channel(this);
}

void Channel::disable_writing() {
    // TODO 4: events_ &= ~EPOLLOUT; 然后通知 loop
    events_&=~EPOLLIN;
    loop_->update_channel(this);
}

// TODO 5：按位分发到回调（见 channel.h 注释里的 ①~③）
void Channel::handle_event(std::uint32_t revents) {
    // (void)revents;
    if(revents&EPOLLERR){if(error_cb_)error_cb_();return ;}
    if(revents&(EPOLLIN|EPOLLPRI|EPOLLHUP)){if(read_cb_)read_cb_();}
    if(revents&EPOLLOUT){if(write_cb_)write_cb_();}
}
