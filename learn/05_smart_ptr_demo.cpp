// 实验 5：三种智能指针到底在干什么
// 跑法：./dev.sh "g++ -std=c++17 -O2 learn/05_smart_ptr_demo.cpp -o /tmp/s5 && /tmp/s5"
#include <cstdio>
#include <memory>
#include <string>

struct Node {
    std::string name;
    explicit Node(std::string n) : name(std::move(n)) { std::printf("  构造 %s\n", name.c_str()); }
    ~Node() { std::printf("  析构 %s  ← 资源被释放\n", name.c_str()); }
};

int main() {
    std::puts("【1】unique_ptr：独占，离开作用域自动 delete");
    {
        std::unique_ptr<Node> p(new Node("A"));
        std::puts("  （还在作用域里）");
    }
    std::puts("  （离开作用域了 → 上面应该打印过 A 的析构）\n");

    std::puts("【2】shared_ptr：共享，引用计数到 0 才释放");
    {
        std::shared_ptr<Node> a = std::make_shared<Node>("B");
        std::printf("  创建 a 后 use_count = %ld\n", (long)a.use_count());
        std::shared_ptr<Node> b = a;             // 拷贝一份，计数 +1
        std::printf("  拷贝给 b 后 use_count = %ld\n", (long)a.use_count());
        b.reset();                               // b 放手，计数 -1
        std::printf("  b.reset() 后 use_count = %ld\n", (long)a.use_count());
    }
    std::puts("  （a 离开作用域，计数归零 → B 被释放）\n");

    std::puts("【3】循环引用：A 持有 B，B 也持有 A（都是 shared_ptr）→ 谁也释放不了");
    struct Node2 : std::enable_shared_from_this<Node2> {
        std::string name;
        std::shared_ptr<Node2> other;
        explicit Node2(std::string n) : name(std::move(n)) { std::printf("  构造 %s\n", name.c_str()); }
        ~Node2() { std::printf("  析构 %s  ← 资源被释放\n", name.c_str()); }
    };
    {
        auto x = std::make_shared<Node2>("X");
        auto y = std::make_shared<Node2>("Y");
        x->other = y;      // X 指着 Y
        y->other = x;      // Y 指着 X  ← 环
        std::printf("  x.use_count = %ld, y.use_count = %ld\n", (long)x.use_count(), (long)y.use_count());
    }
    std::puts("  （离开作用域了，但注意：上面没有打印 X/Y 的析构 → 内存泄漏！）\n");

    std::puts("【4】用 weak_ptr 打破环：其中一条边改成 weak_ptr");
    struct Node3 {
        std::string name;
        std::shared_ptr<Node3> other;
        std::weak_ptr<Node3> parent;             // 弱引用：不增加计数
        explicit Node3(std::string n) : name(std::move(n)) { std::printf("  构造 %s\n", name.c_str()); }
        ~Node3() { std::printf("  析构 %s  ← 资源被释放\n", name.c_str()); }
    };
    {
        auto x = std::make_shared<Node3>("P");
        auto y = std::make_shared<Node3>("Q");
        x->other = y;                 // shared：X 拥有 Y
        y->parent = x;                // weak：Y 只是"看着" X，不拥有
        std::printf("  x.use_count = %ld, y.use_count = %ld（weak 不增加计数）\n",
                    (long)x.use_count(), (long)y.use_count());
    }
    std::puts("  （离开作用域 → 两个都正常析构，泄漏被解决）");
    return 0;
}
