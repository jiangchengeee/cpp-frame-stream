// 手写一个 mini_unique_ptr，理解 unique_ptr 的原理
// unique_ptr 内部就三样东西：一个裸指针 + 禁止拷贝 + 支持移动
#pragma once
#include <utility>

template <typename T>
class MiniUniquePtr {
public:
    // ① 构造：把裸指针交给我管
    explicit MiniUniquePtr(T* p = nullptr) : p_(p) {}

    // ② 析构：我死的时候把对象 delete 掉（这是 RAII）
    ~MiniUniquePtr() { delete p_; }

    // ③ 禁止拷贝：两个 unique_ptr 不能同时拥有同一个对象
    //    （copy ctor 和 copy assignment 都 delete 掉）
    MiniUniquePtr(const MiniUniquePtr&) = delete;
    MiniUniquePtr& operator=(const MiniUniquePtr&) = delete;

    // ---------------- TODO 1：移动构造 ----------------
    // 目标：把 other 的指针"偷"过来，并让 other 变成空，这样就不会有两个人管同一个对象
    // 步骤：
    //   ① p_ = other.p_;
    //   ② other.p_ = nullptr;
    MiniUniquePtr(MiniUniquePtr&& other) noexcept {
        // (void)other;
        // p_ = nullptr;   // TODO 1: 把这行替换成上面 ① ②
        p_=other.p_;
        other.p_=nullptr;
    }

    // ---------------- TODO 2：移动赋值 ----------------
    // 目标：a = std::move(b); 把 b 的对象交给 a，注意 a 原来管的对象要先释放，否则泄漏
    // 步骤：
    //   ① if (this == &other) return *this;   // 自己给自己赋值，直接返回
    //   ② delete p_;                          // 先释放我原来管的对象
    //   ③ p_ = other.p_;                      // 偷过来
    //   ④ other.p_ = nullptr;                 // 对方置空
    //   ⑤ return *this;
    MiniUniquePtr& operator=(MiniUniquePtr&& other) noexcept {
        // (void)other;
        // // TODO 2: 在这里写上面 ①~⑤
        // return *this;
        if(this==&other)return *this;
        delete p_;
        p_=other.p_;
        other.p_=nullptr;
        return *this;
    }

    // 下面的已经写好，读懂就行
    T& operator*() const { return *p_; }
    T* operator->() const { return p_; }
    T* get() const { return p_; }

    // release：放弃所有权，把裸指针交还给你（不 delete）
    T* release() {
        T* tmp = p_;
        p_ = nullptr;
        return tmp;
    }

    // reset：换一个对象来管（老的先 delete）
    void reset(T* p = nullptr) {
        if (p != p_) {
            delete p_;
            p_ = p;
        }
    }

    explicit operator bool() const { return p_ != nullptr; }

private:
    T* p_;   // 唯一的成员：一个裸指针
};
