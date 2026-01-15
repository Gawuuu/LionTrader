#include "TraderEngine.h"
#include "Logger.h"
#include <chrono>

// 构造函数：初始化引用，并绑定 OrderBook 的回调函数
TraderEngine::TraderEngine(RingBuffer<Order, 1024 * 1024> &buffer)
    : order_buffer_(buffer),
      // 使用 std::bind 将 on_match 成员函数绑定给 OrderBook
      order_book_(std::bind(&TraderEngine::on_match, this,
                            std::placeholders::_1, std::placeholders::_2,
                            std::placeholders::_3, std::placeholders::_4)),
      running_(false)
{ }

void TraderEngine::init()
{
    LOG_INFO("TraderEngine Initialized");
}

void TraderEngine::start()
{
    running_ = true;
    thread_ = std::thread(&TraderEngine::run, this);
}

void TraderEngine::stop()
{
    running_ = false;
    if (thread_.joinable())
    {
        thread_.join();
    }
}

void TraderEngine::wait()
{
    if (thread_.joinable())
    {
        thread_.join();
    }
}

void TraderEngine::run()
{
    LOG_INFO("TraderEngine Started");

    int64_t processed_count = 0;
    auto start_time = std::chrono::high_resolution_clock::now();

    while (running_)
    {
        Order o;
        if (order_buffer_.pop(o))
        {
            // 退出循环
            if (o.order_ref == 0)
            {
                LOG_INFO("TraderEngine received stop signal.");
                running_ = false;
                break;
            }

            // 核心逻辑：撮合
            order_book_.addOrder(o);
            processed_count++;
        }
        else
        {
            // 队列为空，忙等待
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

    // 打印统计信息
    LOG_INFO("=== Statistics ===");
    LOG_INFO("Orders Processed: %ld", processed_count);
    LOG_INFO("Trades Generated: %ld", trades_count_);

    if (total_ns > 0)
    {
        double tps = (processed_count * 1e9 / total_ns) / 1000000.0;
        double latency = (double)total_ns / processed_count;
        LOG_INFO("System TPS      : %.2f M/s", tps);
        LOG_INFO("Avg Latency     : %.2f ns", latency);
    }
}

// OrderBook 的回调函数
void TraderEngine::on_match(int64_t bid_id, int64_t ask_id, double price, int32_t qty)
{
    trades_count_++;
    // 在实盘中，这里会生成 Trade 消息发回给网关
    if (trades_count_ % 100000 == 0)
    {
        LOG_INFO("Match Event: %ld <> %ld @ %.2f (Total: %ld)", bid_id, ask_id, price, trades_count_);
    }
}