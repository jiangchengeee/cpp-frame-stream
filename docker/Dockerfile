# Linux C++ 开发环境（第 2 周写 epoll 用）
# 用法：docker run --rm -it -v "$PWD":/work -w /work frame-dev bash
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake gdb git vim strace \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /work
CMD ["bash"]
