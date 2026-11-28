// 实验 8：把 encode 出来的"帧"按字节打印出来，看清 4 字节长度头
#include <arpa/inet.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

static void dump(const char* title, const std::string& s) {
    std::printf("%s（共 %zu 字节）：\n  ", title, s.size());
    for (unsigned char c : s) std::printf("%02X ", c);
    std::printf("\n  ");
    for (unsigned char c : s) std::printf("%c", (c >= 32 && c < 127) ? c : '.');
    std::printf("\n\n");
}

int main() {
    // 和 encode_frame 里做的事一模一样
    for (const char* msg : {"abc", "hello-frame"}) {
        std::uint32_t len = htonl((std::uint32_t)std::strlen(msg));
        std::string frame;
        frame.append(reinterpret_cast<const char*>(&len), 4);   // 4 字节长度头
        frame += msg;                                          // 内容
        char title[64];
        std::snprintf(title, sizeof title, "encode_frame(\"%s\")", msg);
        dump(title, frame);
    }
    return 0;
}
