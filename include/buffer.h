// Buffer：应用层的字节缓冲
//
// 为什么需要它：TCP 是"字节流"，不是"消息流"。
//   - 你 send 两次 "abc" "def"，对端可能一次 read 到 "abcdef"（粘包）
//   - 也可能分两次读到 "abc" 和 "def"（半包）
// 所以需要一个缓冲区：先把读到的字节攒着，等凑够一条完整的帧再取走。
#pragma once

#include <sys/types.h>
#include <unistd.h>

#include <cstddef>
#include <cstring>
#include <vector>

class Buffer {
public:
    // ---------------- TODO 1：追加数据 ----------------
    // 目标：把 [data, data+len) 这段字节拷到缓冲区末尾
    // 步骤：buf_.insert(buf_.end(), data, data + len);
    void append(const char* data, std::size_t len) {
        // (void)data;
        // (void)len;
        // TODO 1
        buf_.insert(buf_.end(),data,data+len);
    }

    // ---------------- TODO 2：还剩多少可读字节 ----------------
    // 可读 = 总长度 - 已消费的偏移
    // 步骤：return buf_.size() - read_idx_;
    std::size_t readable() const {
        // TODO 2
        // return 0;
        return buf_.size()-read_idx_;
    }

    // ---------------- TODO 3：看数据但不消费 ----------------
    // 返回"第一条未消费字节"的地址
    // 步骤：return buf_.data() + read_idx_;
    const char* peek() const {
        // TODO 3
        // return nullptr;
        return buf_.data()+read_idx_;
    }

    // ---------------- TODO 4：消费 n 个字节 ----------------
    // 目标：把读指针往前挪 n 字节；如果全消费完了，就把内存也清空
    // 步骤：
    //   ① read_idx_ += n;
    //   ② if (read_idx_ >= buf_.size()) { buf_.clear(); read_idx_ = 0; }
    void retrieve(std::size_t n) {
        // (void)n;
        // TODO 4
        read_idx_+=n;
        if(read_idx_>=buf_.size()){buf_.clear();read_idx_=0;}
    }

    // ---------------- TODO 5：从 fd 读进缓冲区 ----------------
    // 目标：从 socket 读一次，读到的追加到缓冲区；返回读了多少（0=对端关闭，-1=出错）
    // 步骤：
    //   ① char tmp[4096];
    //   ② ssize_t n = ::read(fd, tmp, sizeof(tmp));
    //   ③ if (n > 0) append(tmp, (std::size_t)n);
    //   ④ return n;
    ssize_t read_fd(int fd) {
        // (void)fd;
        // // TODO 5
        // return 0;
        char tmp[4096];
        ssize_t n=::read(fd,tmp,sizeof(tmp));
        if(n>0)append(tmp,(std::size_t)n);
        return n;
    }

private:
    std::vector<char> buf_;        // 所有字节
    std::size_t read_idx_{0};      // 已经消费到哪了（之前的都没用了）
};
