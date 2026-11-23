// 实验 4：explicit 到底拦住了什么
struct Implicit {            // 没有 explicit
    Implicit(int) {}
};
struct Explicit {            // 有 explicit
    explicit Explicit(int) {}
};

void take_implicit(Implicit) {}
void take_explicit(Explicit) {}

int main() {
    take_implicit(5);   // ✅ 编译通过：5 被"偷偷"转成了 Implicit(5)
    // take_explicit(5); // ❌ 编译报错：explicit 禁止这种偷偷转换
    return 0;
}
