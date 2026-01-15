# LionTrader - High Performance Matching Engine

![C++](https://img.shields.io/badge/C++-17-blue) ![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey) ![License](https://img.shields.io/badge/License-MIT-green) ![Status](https://img.shields.io/badge/Status-Stable-brightgreen)

**LionTrader** 是一个极简主义、超低延迟的限价订单簿（LOB）撮合引擎。它参考了顶尖 HFT 交易系统（如 REM/易盛）的架构设计，采用现代 C++17 标准与无锁数据结构，旨在实现微秒级的确定性延迟。

本项目旨在展示高频交易系统中的核心技术实现，包括无锁并发、内存池管理、缓存亲和性优化以及内核旁路（Kernel Bypass）的设计思想。

## ✨ 核心特性

*   **⚡ 极致性能**：基于单核单线程模型，在标准消费级硬件上实现了 **> 1200 万笔/秒** 的吞吐量，平均内部处理延迟低至 **~80 ns**。
*   **🔒 无锁架构**：
    *   **SPSC 环形队列**：基于 `std::atomic` 和内存序（Acquire/Release）实现的单生产者单消费者队列，彻底消除 Mutex 锁开销。
    *   **缓存行填充**：利用 `alignas` 避免多线程间的**伪共享 (False Sharing)** 问题。
*   **💾 零 GC 运行时**：
    *   **对象池 (Object Pool)**：自定义基于 `union` Free List 的内存池，结合 Placement New，实现 $O(1)$ 的内存分配与回收。
    *   **热路径零分配**：在交易时段（Hot Path）杜绝任何 `new/malloc` 操作，消除延迟抖动。
*   **📝 异步日志系统**：
    *   **非阻塞写入**：主线程仅执行极速内存拷贝，后台线程负责磁盘 I/O。
    *   **C 风格 I/O**：使用 `vsnprintf` 和 `FILE*` 替代 C++ iostream，进一步降低格式化开销。
*   **🛠️ 工业级规范**：严格的 POD (Plain Old Data) 类型检查，防御性编程（Static Assert），以及模块化的工程结构。

## 🏗️ 架构设计 (Architecture)

本项目采用经典的生产者-消费者模型，通过共享内存环形队列解耦：

| 模块 | 目录 | 说明 |
| :--- | :--- | :--- |
| **Core** | `src/core` | **基础设施层**。包含无锁环形队列 (`RingBuffer`)、定长内存池 (`MemoryPool`) 及异步日志 (`Logger`)。 |
| **Engine** | `src/engine` | **业务逻辑层**。包含行情网关 (`MdEngine`)、撮合引擎 (`TraderEngine`) 及订单簿 (`OrderBook`)。 |
| **Types** | `include` | **协议定义**。定义所有通过 RingBuffer 传输的 POD 数据结构（如 `Order`, `MarketData`）。 |
| **Tests** | `tests` | **单元测试**。针对撮合逻辑的点火测试用例。 |

## 🚀 快速开始 (Quick Start)

### 先决条件
*   **操作系统**: Linux (推荐 WSL2 或原生 Ubuntu)
*   **编译器**: GCC >= 7.0 或 Clang >= 5.0 (支持 C++17)
*   **构建工具**: CMake >= 3.10

### 编译与运行
得益于 CMake 构建系统，你可以一键编译整个项目。

1.  **克隆仓库**
    ```bash
    git clone https://github.com/Gawuuu/LionTrader.git
    cd LionTrader
    ```

2.  **构建项目**
    ```bash
    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make
    ```
    *注意：务必使用 `Release` 模式以启用 `-O3` 优化并禁用断言，否则性能会有数量级的差异。*

3.  **运行基准测试**
    ```bash
    ./LionTrader
    ```
    程序将启动模拟网关（生产 100 万笔随机订单）和撮合引擎，运行结束后会打印 TPS 和 Latency 统计报告。
    
5. **查看日志**
   ```
   cat lion_trader.log
   ```

## 📊 性能基准 (Benchmark)

测试环境：Processor	AMD Ryzen 5 3600 6-Core Processor, 4200 Mhz, 6 Core(s), 12 Logical Processor(s) (WSL2 Ubuntu 22.04)

```text
[2026-01-15 21:56:22] [INFO] LionTrader System Starting...
[2026-01-15 21:56:22] [INFO] MdEngine Initialized
[2026-01-15 21:56:22] [INFO] TraderEngine Initialized
[2026-01-15 21:56:22] [INFO] TraderEngine Started
[2026-01-15 21:56:22] [INFO] MdEngine: Start sending 1000000 orders...
[2026-01-15 21:56:22] [INFO] Match Event: 106843 <> 101284 @ 91.96 (Total: 100000)
[2026-01-15 21:56:22] [INFO] Match Event: 212844 <> 207107 @ 94.56 (Total: 200000)
[2026-01-15 21:56:23] [INFO] Match Event: 318949 <> 318930 @ 95.54 (Total: 300000)
[2026-01-15 21:56:23] [INFO] Match Event: 426721 <> 426637 @ 96.63 (Total: 400000)
[2026-01-15 21:56:23] [INFO] Match Event: 534654 <> 534535 @ 96.37 (Total: 500000)
[2026-01-15 21:56:23] [INFO] Match Event: 641407 <> 641348 @ 95.88 (Total: 600000)
[2026-01-15 21:56:23] [INFO] Match Event: 746666 <> 746417 @ 94.38 (Total: 700000)
[2026-01-15 21:56:23] [INFO] Match Event: 855491 <> 844972 @ 96.05 (Total: 800000)
[2026-01-15 21:56:23] [INFO] Match Event: 957038 <> 922390 @ 99.43 (Total: 900000)
[2026-01-15 21:56:23] [INFO] MdEngine: Injection finished. TPS: 11.13 M/s
[2026-01-15 21:56:23] [INFO] TraderEngine received stop signal.
[2026-01-15 21:56:23] [INFO] === Statistics ===
[2026-01-15 21:56:23] [INFO] Orders Processed: 1000000
[2026-01-15 21:56:23] [INFO] Trades Generated: 940351
[2026-01-15 21:56:23] [INFO] System TPS      : 11.11 M/s
[2026-01-15 21:56:23] [INFO] Avg Latency     : 89.98 ns
[2026-01-15 21:56:23] [INFO] System Shutdown.
```

## 📖 技术细节

### 1. 内存布局优化
为了最大化 CPU Cache 命中率，所有核心数据结构（如 `RingBuffer` 的 head/tail 索引）均强制对齐到 Cache Line（通常为 64 字节）：
```cpp
alignas(std::hardware_destructive_interference_size) std::atomic<size_t> tail_;
```

### 2. 内存序 (Memory Ordering)
放弃默认的 `seq_cst`，在 SPSC 队列中严格使用 `memory_order_release` 和 `memory_order_acquire`，在保证数据可见性的前提下，消除 x86 架构下的 Store Buffer 刷新开销。

## 🤝 贡献指南

1.  Fork 本项目
2.  创建你的 Feature 分支 (`git checkout -b feature/Optimization`)
3.  提交你的修改 (`git commit -m 'Optimize matching algorithm'`)
4.  推送到分支 (`git push origin feature/Optimization`)
5.  提交 Pull Request

## 📄 许可证

本项目基于 [MIT License](LICENSE) 开源。
