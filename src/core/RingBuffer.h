#pragma once

#include <atomic> // for std::memory_order_acquire
#include <vector>
#include <type_traits>
#include <new> // for std::hardware_destructive_interference_size

// 跨平台处理缓存行大小
#ifdef __cpp_lib_hardware_interference_size
using std::hardware_destructive_interference_size;
#else
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

template <typename T, size_t Capacity>
class RingBuffer
{
    // ========================================================================
    // 类型检查 (Type Traits)
    // ========================================================================

    // 1. 检查 T 是否可以安全地进行内存拷贝 (memcpy)。
    //    HFT 系统为了极致性能，通常假设数据是 POD (Plain Old Data)。
    //    如果传入了 std::string 或带虚函数的类，这里会直接编译报错，防止运行时 Crash。
    static_assert(std::is_trivially_copyable<T>::value, "RingBuffer T must be trivially copyable (POD)");

    // 2. 检查 T 是否有默认构造函数。
    //    因为我们在数组 buffer_ 初始化时需要默认构造。
    static_assert(std::is_default_constructible<T>::value, "RingBuffer T must be default constructible");

    // 3. 检查 Capacity 是否是 2 的幂 (位运算优化)
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");

public:
    RingBuffer() : head_(0), tail_(0) {}

    // 【生产者】写入队尾 (Tail)
    bool push(const T &val)
    {
        // 1. 加载消费者位置 (head)
        // acquire: 确保我看到了消费者最新的进度，别覆盖了没读的数据
        size_t head = head_.load(std::memory_order_acquire);
        size_t tail = tail_.load(std::memory_order_relaxed);

        // 2. 判满
        // tail 追上了 head (差一圈)
        if (((tail + 1) & (Capacity - 1)) == (head & (Capacity - 1)))
        {
            return false;
        }

        // 3. 写数据 (无锁，普通写)
        buffer_[tail & (Capacity - 1)] = val;

        // 4. 更新 tail
        // release: 保证"写数据"发生在"更新tail"之前
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    // 【消费者】从队头读取 (Head)
    bool pop(T &val)
    {
        size_t tail = tail_.load(std::memory_order_acquire);
        size_t head = head_.load(std::memory_order_relaxed);

        // 1. 判空
        if (head == tail)
        {
            return false;
        }

        // 2. 读数据（必定可见）
        val = buffer_[head & (Capacity - 1)];

        // 3. 更新 head
        // release: 告诉生产者这块地我用完了，你可以覆盖了
        head_.store(head + 1, std::memory_order_release);
        return true;
    }
    // 判断队列是否不为空
    bool read_available() const
    {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t tail = tail_.load(std::memory_order_relaxed);
        return head != tail;
    }

private:
    // --- 内存布局优化 ---

    // 消费者修改 head_
    alignas(hardware_destructive_interference_size) std::atomic<size_t> head_;

    // 生产者修改 tail_
    // 只要这两个变量在不同的 Cache Line，就不会伪共享
    alignas(hardware_destructive_interference_size) std::atomic<size_t> tail_;

    // 数据区
    T buffer_[Capacity];
};