// cpp-frame-stream · main
// 第 1 天：只保证工程能编译、能打印版本，后面逐步替换成真正的服务入口。

#include <cstdio>

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    std::printf("cpp-frame-stream v0.1 (dev)\n");
    std::printf("milestone: M1 - project compiles\n");
    std::printf("next: bounded queue + thread pool\n");
    return 0;
}
