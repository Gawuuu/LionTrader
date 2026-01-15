#pragma once
#include <atomic>
#include <thread>
#include "RingBuffer.h"
#include "OrderBook.h"
#include "Types.h"

class TraderEngine {
public:
    TraderEngine(RingBuffer<Order, 1024*1024>& buffer);
    
    void init();
    void start();
    void stop();
    void wait();
    
    void run();
    void on_match(int64_t bid_id, int64_t ask_id, double price, int32_t qty);

private:
    RingBuffer<Order, 1024*1024>& order_buffer_;
    OrderBook order_book_;
    std::atomic<bool> running_;
    std::thread thread_;
    int64_t trades_count_ = 0;
};