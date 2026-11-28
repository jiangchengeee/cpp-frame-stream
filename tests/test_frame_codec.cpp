// 帧编解码测试
#include "buffer.h"
#include "frame_codec.h"

#include <cstdio>
#include <string>
#include <vector>

#define CHECK(cond)                                                   \
    do {                                                              \
        if (!(cond)) {                                                \
            std::printf("  ✗ FAIL 第 %d 行：%s\n", __LINE__, #cond);  \
            return 1;                                                 \
        }                                                             \
    } while (0)

static Buffer make_buf(const std::string& bytes) {
    Buffer b;
    b.append(bytes.data(), bytes.size());
    return b;
}

int main() {
    std::puts("[测试 1] encode / decode 往返一致");
    {
        std::string frame = encode_frame("hello-frame");
        CHECK(frame.size() == 4 + 11);              // 4 字节头 + 11 字节内容
        Buffer b = make_buf(frame);
        auto frames = decode_frames(b);
        CHECK(frames.size() == 1);
        CHECK(frames[0] == "hello-frame");
        CHECK(b.readable() == 0);                   // 完整帧应被消费干净
    }
    std::puts("  ✓ 通过");

    std::puts("[测试 2] 一条完整帧");
    {
        Buffer b = make_buf(encode_frame("abc"));
        auto frames = decode_frames(b);
        CHECK(frames.size() == 1);
        CHECK(frames[0] == "abc");
    }
    std::puts("  ✓ 通过");

    std::puts("[测试 3] 半包：长度头到了但内容不够，必须保留");
    {
        std::string frame = encode_frame("abcdef");   // 4 + 6 = 10 字节
        Buffer b = make_buf(frame.substr(0, 6));      // 只给 6 字节（不够）
        CHECK(decode_frames(b).empty());              // 取不出帧
        CHECK(b.readable() == 6);                     // 数据必须原样保留

        b.append(frame.data() + 6, frame.size() - 6); // 剩余部分到达
        auto frames = decode_frames(b);
        CHECK(frames.size() == 1);
        CHECK(frames[0] == "abcdef");
    }
    std::puts("  ✓ 通过");

    std::puts("[测试 4] 粘包：两条帧一次性到达");
    {
        Buffer b = make_buf(encode_frame("one") + encode_frame("two"));
        auto frames = decode_frames(b);
        CHECK(frames.size() == 2);
        CHECK(frames[0] == "one");
        CHECK(frames[1] == "two");
        CHECK(b.readable() == 0);
    }
    std::puts("  ✓ 通过");

    std::puts("[测试 5] 混合：一条完整 + 一条半包");
    {
        std::string f1 = encode_frame("full");
        std::string f2 = encode_frame("partial");
        Buffer b = make_buf(f1 + f2.substr(0, 5));    // 第二条只有 5 字节
        auto frames = decode_frames(b);
        CHECK(frames.size() == 1);
        CHECK(frames[0] == "full");
        CHECK(b.readable() == 5);                     // 半包留着
    }
    std::puts("  ✓ 通过");

    std::puts("[测试 6] 空 buffer 返回空");
    {
        Buffer b;
        CHECK(decode_frames(b).empty());
    }
    std::puts("  ✓ 通过");

    std::puts("\n全部测试通过 ✅");
    return 0;
}
