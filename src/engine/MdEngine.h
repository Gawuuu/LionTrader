#pragma once
#include <atomic>
#include <thread>
#include "RingBuffer.h"
#include "Types.h"

class MdEngine {
public:
    // 传入全局 RingBuffer 的引用
    MdEngine(RingBuffer<Order, 1024*1024>& buffer);
    
    void init();
    void start();
    void stop();
    void wait();
    
    // 模拟接收行情/订单流
    void run();

private:
    RingBuffer<Order, 1024*1024>& order_buffer_;
    std::atomic<bool> running_;
    std::thread thread_;
};