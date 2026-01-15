#include "MdEngine.h"
#include "Logger.h"
#include <random>
#include <chrono>
#include <cstring> // for strncpy

// 将 RandomWalk 放在匿名命名空间或内部，作为辅助工具
namespace
{
    struct RandomWalk
    {
        std::mt19937 gen;
        std::uniform_int_distribution<> side_dist;
        std::uniform_int_distribution<> price_dist;
        std::uniform_int_distribution<> qty_dist;
        double current_price;

        RandomWalk(double start_price)
            : gen(12345), side_dist(0, 1), price_dist(-5, 5), qty_dist(1, 100), current_price(start_price) {}

        Order next(int64_t id)
        {
            Order o;
            o.order_ref = id;
            std::strncpy(o.instrument_id, "BTC-USDT", 16);
            o.direction = side_dist(gen) == 0 ? 'B' : 'S';

            double change = price_dist(gen) * 0.01;
            o.limit_price = current_price + change;
            if (id % 10 == 0)
                current_price += change;

            o.volume = qty_dist(gen);
            o.offset_flag = 'O';
            return o;
        }
    };
}

MdEngine::MdEngine(RingBuffer<Order, 1024 * 1024> &buffer)
    : order_buffer_(buffer), running_(false) {}

void MdEngine::init()
{
    LOG_INFO("MdEngine Initialized");
}

void MdEngine::start()
{
    running_ = true;
    thread_ = std::thread(&MdEngine::run, this);
}

void MdEngine::stop()
{
    running_ = false;
    if (thread_.joinable())
    {
        thread_.join();
    }
    LOG_INFO("MdEngine Stopped");
}

void MdEngine::wait()
{
    if (thread_.joinable())
    {
        thread_.join();
    }
}

void MdEngine::run()
{
    // 模拟发单配置
    const int TEST_COUNT = 1000000;
    RandomWalk rw(100.0);

    LOG_INFO("MdEngine: Start sending %d orders...", TEST_COUNT);

    // 计时
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < TEST_COUNT && running_; ++i)
    {
        Order o = rw.next(i + 1);

        // 忙等待写入
        while (!order_buffer_.push(o))
        {
            // 如果满了，让出 CPU，防止死锁
            std::this_thread::yield();
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

    LOG_INFO("MdEngine: Injection finished. TPS: %.2f M/s", (TEST_COUNT * 1e9 / ns) / 1000000.0);

    // 通知消费者结束
    Order stop_signal;
    stop_signal.order_ref = 0; // 约定 ID=0 为结束信号
    while (!order_buffer_.push(stop_signal))
    {
        std::this_thread::yield();
    }
}