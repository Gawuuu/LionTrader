#include <iostream>
#include <thread>
#include "RingBuffer.h"
#include "Logger.h"
#include "MdEngine.h"
#include "TraderEngine.h"

// 全局通道
RingBuffer<Order, 1024*1024> g_buffer;

int main() {
    // 1. 启动日志
    AsyncLogger::instance().init("lion_trader.log");
    LOG_INFO("LionTrader System Starting...");

    // 2. 初始化引擎
    MdEngine md(g_buffer);
    TraderEngine trader(g_buffer);

    md.init();
    trader.init();

    // 3. 启动线程
    trader.start(); 
    // std::this_thread::sleep_for(...) // 这行可以删掉了，不需要手动 sleep
    md.start();

    // 4. 等待结束
    // 主线程会阻塞在这里，直到 MdEngine 发完 100万单自动退出
    md.wait(); 
    
    // MdEngine 退出后，TraderEngine 退出
    trader.wait();

    LOG_INFO("System Shutdown.");
    AsyncLogger::instance().stop();
    return 0;
}