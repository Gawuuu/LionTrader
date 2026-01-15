#pragma once

#include <vector>
#include <cassert>
#include <iostream>
#include <limits>

template <typename T, size_t BlockSize = 4096>
class ObjectPool 
{
private:
    // 联合体
    // 当这个槽位空闲时，next_free_idx 存储下一个空闲槽位的下标
    // 当这个槽位被分配时，data 存储真实数据
    struct Slot 
    {
        union 
        {
            T data;
            size_t next_free_idx;
        };
        // 构造函数不初始化 data，省时间
        Slot() : next_free_idx(0) {} 
    };

    std::vector<Slot> pool_;
    size_t first_free_idx_; // 链表头：指向第一个空闲的位置

public:
    ObjectPool(size_t initial_size = BlockSize) 
    {
        pool_.resize(initial_size);
        // 初始化 Free List：每个槽位指向下一个
        for (size_t i = 0; i < initial_size - 1; ++i) 
        {
            pool_[i].next_free_idx = i + 1;
        }
        pool_[initial_size - 1].next_free_idx = -1; // 链表尾
        first_free_idx_ = 0;
    }

    // 分配一个对象 (替代 new)
    // 返回指针，也可以改造成返回智能指针或 ID
    template <typename... Args>
    T* allocate(Args&&... args) 
    {
        // 如果池子满了，扩容
        if (first_free_idx_ == std::numeric_limits<size_t>::max()) 
        {
            expand();
        }

        // 1. 拿到头节点
        size_t idx = first_free_idx_;
        Slot& slot = pool_[idx];

        // 2. 更新头节点指向下一个
        first_free_idx_ = slot.next_free_idx;

        // 3. 在这块内存上原地构造对象 (Placement New)
        // 在已分配的内存地址上调用构造函数
        // std::forward 完美转发
        T* ptr = &slot.data;
        new (ptr) T(std::forward<Args>(args)...); 

        return ptr;
    }

    // 释放一个对象 (替代 delete)
    void deallocate(T* ptr) 
    {
        if (!ptr) return;
        assert(ptr >= &pool_[0].data && ptr <= &pool_.back().data);
        // 1. 调用析构函数
        ptr->~T();

        // 2. 计算指针对应的下标
        // 指针减法：计算偏移量
        Slot* slot_ptr = reinterpret_cast<Slot*>(ptr);
        size_t idx = slot_ptr - &pool_[0];

        // 3. 头插法：把这个槽位放回 Free List 的头部
        // 这样刚刚释放的热内存，下次分配时马上又能用到 (Cache Friendly)
        slot_ptr->next_free_idx = first_free_idx_;
        first_free_idx_ = idx;
    }

private:
    void expand() 
    {
        size_t old_size = pool_.size();
        size_t new_size = old_size * 2;
        pool_.resize(new_size);

        // 重新链接新生成的空闲块
        for (size_t i = old_size; i < new_size - 1; ++i) 
        {
            pool_[i].next_free_idx = i + 1;
        }
        pool_[new_size - 1].next_free_idx = first_free_idx_; // 接上旧的链表头
        first_free_idx_ = old_size;
        
        // 扩容会导致 pool_ 的地址改变，之前分配出去的指针可能会失效！
        std::cerr << "[WARN] ObjectPool expanded to " << new_size << "! Pointers might be invalid!" << std::endl;
    }
};