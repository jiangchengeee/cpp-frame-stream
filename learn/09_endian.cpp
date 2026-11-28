// 实验 9：本机是什么端序？htonl 到底做了什么？
#include <arpa/inet.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

static void print_bytes(const char* tag, std::uint32_t v) {
    unsigned char b[4];
    std::memcpy(b, &v, 4);                     // 把这 4 个字节的内存原样拷出来
    std::printf("%-28s = %02X %02X %02X %02X   (= 十进制 %u)\n", tag, b[0], b[1], b[2], b[3], v);
}

int main() {
    std::uint32_t x = 3;
    print_bytes("本机存的数字 3", x);
    print_bytes("htonl(3) 之后", htonl(x));

    std::uint32_t y = 0x12345678;
    print_bytes("本机存的 0x12345678", y);
    print_bytes("htonl(0x12345678)", htonl(y));
    std::printf("ntohl(htonl(0x12345678)) = 0x%X（转一圈回来还是原值）\n", ntohl(htonl(y)));
    return 0;
}
