# cpp-frame-stream · 传感器帧流处理服务

Linux 上的 C++17 多线程服务：**接收帧 → 有界队列 → 工作线程处理 → 输出**，带背压与压测。

## 状态：🚧 开发中（2026.11.22 起，计划 14 天）

| 里程碑 | 内容 | 状态 |
|---|---|---|
| M1 | 工程可编译 | ✅ |
| M2 | 有界队列 + 线程池 | ⬜ |
| M3 | epoll echo（LT） | ⬜ |
| M4 | 长度头分帧 + ET 非阻塞 | ⬜ |
| M5 | 帧流管线 + 背压 | ⬜ |
| M6 | 压测数据（QPS / P50 / P99） | ⬜ |
| M7 | README 定稿 | ⬜ |

## 架构

```
client ──TCP──▶ [epoll Reactor] ──完整帧──▶ [BoundedQueue] ──▶ [worker 1..N] ──▶ 输出
                                                  ▲
                                             背压策略（拒收 / 丢帧 / 降速）
```

## 构建

```bash
# macOS 先装工具链
brew install cmake

mkdir -p build && cd build
cmake .. && make -j
./frame_stream
```

> 第 2 周写 epoll 时需要有 Linux 环境：Docker Desktop（`docker run -it --rm -v $PWD:/app -w /app ubuntu:24.04 bash`）
> 或云服务器 / UTM 虚拟机。

## 设计取舍

（随实现补充，最终至少写清 3 条：为什么用长度头、为什么有界、为什么单 Reactor）

## 验收问题（面试原题，见 `../02-项目规格.md`）

1. 为什么 4 字节长度头而不是分隔符？
2. ET 与 LT 的区别，ET 为什么必须非阻塞？
3. 队列满了怎么办，三种背压策略的取舍？
4. 优雅退出怎么保证不丢任务？
5. QPS / P99 多少，瓶颈在哪？
