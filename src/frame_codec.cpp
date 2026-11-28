#include "frame_codec.h"

#include <arpa/inet.h>

#include <cstring>

// ============================================================
// 你的任务：实现下面两个函数（今天不给步骤了，自己想）
//
// 需要的工具（都在 Buffer 里）：
//   buf.readable()      当前可读字节数
//   buf.peek()          可读数据的起始地址
//   buf.retrieve(n)     消费 n 个字节
//
// 建议思路（只是建议，你也可以设计别的）：
//   decode：循环
//     ① 可读字节 < 4 → 连长度头都不完整，break
//     ② 读长度头，转成主机字节序（ntohl），得到 payload 长度 n
//     ③ 可读字节 < 4 + n → payload 还不完整（半包），break（注意不要消费！）
//     ④ 取出 payload 放进结果，retrieve(4 + n)，继续循环
//   encode：4 字节长度（用 htonl 转成大端）+ payload 拼接
//
// 边界情况要自己想到：
//   - 长度头里是垃圾值（比如 0 或特别大）怎么办？
//   - 一条帧都没有时返回什么？
// ============================================================

std::vector<std::string> decode_frames(Buffer& buf) {
    // (void)buf;
    // // TODO: 实现
    // return {};
    std::vector<std::string> frames={};
    while(1){
        if(buf.readable()<4){break;}
        std::uint32_t len;
        std::memcpy(&len,buf.peek(),4);//1.取出长度头
        len=ntohl(len);//2.转本机端
        if(buf.readable()<4+len)break;
        std::string payload(buf.peek()+4,len);
        frames.push_back(std::move(payload));//3.读取拼接信息
        buf.retrieve(4+len);//4.消费信息
    }
    return frames;

}

std::string encode_frame(const std::string& payload) {
    // (void)payload;
    // // TODO: 实现
    // return {};
    std::uint32_t len=htonl((std::uint32_t)payload.size());
    std::string out;
    out.append(reinterpret_cast<const char*>(&len),4);
    out+=payload;
    return out;
}
