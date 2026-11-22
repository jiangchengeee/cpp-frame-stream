// 实验 1：为什么"自动归还"能防止忘记归还
// 跑法：./dev.sh "g++ -std=c++17 learn/01_raii_demo.cpp -o /tmp/a && /tmp/a"
#include <cstdio>

struct Guard {                       // 一个"看门狗"对象
    Guard()  { std::puts("① 拿到资源（比如打开了文件 / new 了一块内存）"); }
    ~Guard() { std::puts("③ 归还资源（自动关文件 / 自动 delete）"); }
};

void work() {
    Guard g;                          // 拿到资源
    std::puts("② 干活中...");
    std::puts("   中途 return 了！");
    return;                          // 提前退出，注意：没有手动归还
}

int main() {
    std::puts("--- 进入 work() ---");
    work();
    std::puts("--- 离开 work() ---");
    return 0;
}
