#pragma once

#include <cstdio>
#include <thread>
#include <atomic>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <vector>
#include <iostream> // 解决 std::cerr 报错
#include "RingBuffer.h"

enum class LogLevel : uint8_t
{
    INFO,
    WARNING,
    ERROR
};

struct LogEntry
{
    int64_t timestamp;
    LogLevel level;
    char message[256];
};

class AsyncLogger
{
public:
    static AsyncLogger &instance()
    {
        static AsyncLogger logger;
        return logger;
    }

    void init(const char *filename)
    {
        if (running_)
            return;

        file_ = std::fopen(filename, "w");
        if (!file_)
        {
            std::cerr << "Failed to open log file: " << filename << std::endl;
            return;
        }

        running_ = true;
        writer_thread_ = std::thread(&AsyncLogger::process_logs, this);
    }

    void stop()
    {
        running_ = false;
        if (writer_thread_.joinable())
        {
            writer_thread_.join();
        }
        if (file_)
        {
            std::fclose(file_);
            file_ = nullptr;
        }
    }

    // 变长参数接口 (printf 风格)
    void log(LogLevel level, const char *format, ...)
    {
        if (!running_)
            return;

        LogEntry entry;
        // 1. 暂存时间戳 (HFT 中这里可以用 RDTSC 优化，但在 Log 里用 time 也行)
        // 只要不阻塞 IO 就好
        entry.timestamp = std::time(nullptr);
        entry.level = level;

        // 2. 格式化 (C 风格，极快)
        va_list args;
        va_start(args, format);
        std::vsnprintf(entry.message, sizeof(entry.message), format, args);
        va_end(args);

        // 3. 写入 RingBuffer
        // 关键：如果满了，我们 yield CPU，防止主线程死锁，但理想情况是 Buffer 够大不该满
        while (!buffer_.push(entry))
        {
            std::this_thread::yield();
        }
    }

private:
    AsyncLogger() : running_(false), file_(nullptr) {}
    ~AsyncLogger() { stop(); }

    void process_logs()
    {
        LogEntry entry;
        while (running_ || buffer_.read_available())
        { // 这里的 read_available 需要 RingBuffer 支持，或者我们用下面的暴力法

            // 持续消费直到队列空
            while (buffer_.pop(entry))
            {
                char time_buf[64];
                std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", std::localtime(&entry.timestamp));

                const char *level_str = "INFO";
                if (entry.level == LogLevel::WARNING)
                    level_str = "WARN";
                else if (entry.level == LogLevel::ERROR)
                    level_str = "ERROR";

                std::fprintf(file_, "[%s] [%s] %s\n", time_buf, level_str, entry.message);
            }

            if (running_)
            {
                if (file_)
                    std::fflush(file_); // 刷盘
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            else
            {
                // 如果 stop 了且队列空了，退出
                break;
            }
        }
        if (file_)
            std::fflush(file_);
    }

    AsyncLogger(const AsyncLogger &) = delete;
    AsyncLogger &operator=(const AsyncLogger &) = delete;

private:
    // 修改点：增大缓冲区到 1MB (2^20 / 256 bytes per entry ≈ 4096 entries)
    // 或者直接给大点 65536 条
    static constexpr size_t QUEUE_CAPACITY = 65536;

    RingBuffer<LogEntry, QUEUE_CAPACITY> buffer_;
    std::atomic<bool> running_;
    std::thread writer_thread_;
    std::FILE *file_;
};

// 宏定义
#define LOG_INFO(fmt, ...) AsyncLogger::instance().log(LogLevel::INFO, fmt, ##__VA_ARGS__)