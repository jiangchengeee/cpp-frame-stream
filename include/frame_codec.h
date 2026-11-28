// 帧编解码：length-prefix 协议
//
// 帧格式（网络字节序/大端）：
//   +----------------+---------------------+
//   | 4 字节长度 N   |  N 字节 payload      |
//   +----------------+---------------------+
//
// 长度头只表示 payload 的字节数，不包含头本身。
#pragma once

#include "buffer.h"

#include <cstdint>
#include <string>
#include <vector>

// 从 buf 中尽可能多地取出【完整帧】的 payload。
// - 每取出一条完整帧，就要把它从 buf 里消费掉（retrieve）
// - 如果剩余数据不足以构成一条完整帧（半包），必须原样保留在 buf 里，等下次数据到达
// - 返回值：本次取出的所有 payload（可能 0 条、1 条或多条）
std::vector<std::string> decode_frames(Buffer& buf);

// 把一条 payload 编码成帧字节（4 字节长度头 + payload）
std::string encode_frame(const std::string& payload);
