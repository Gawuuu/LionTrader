# LionTrader - High Performance Matching Engine

## Overview
LionTrader is a minimalist, ultra-low latency Limit Order Book (LOB) matching engine written in modern C++17. It mimics the architecture of top-tier HFT systems (e.g., REM/Esunny), focusing on cache friendliness, lock-free concurrency, and zero-allocation runtime.

**Benchmark Result (Single Core):**
*   **Throughput**: > 12 Million Orders/sec
*   **Latency**: ~80 ns per order

## Core Architecture

### 1. Lock-free Communication
*   Implemented a **SPSC (Single-Producer Single-Consumer) Ring Buffer**.
*   Utilized C++17 `std::atomic` with explicit memory ordering (`acquire`/`release`) to achieve synchronization without Mutex/Lock.
*   Prevented **False Sharing** using `alignas(std::hardware_destructive_interference_size)`.

### 2. Zero-GC Memory Management
*   Custom **Object Pool** with a union-based free list.
*   Utilized **Placement New** for object construction on pre-allocated memory blocks.
*   Achieved zero `malloc`/`free` overhead during the trading session (Hot Path).

### 3. Asynchronous Logging
*   Designed a non-blocking Logger using a dedicated background thread.
*   Optimized with `vsnprintf` and `FILE*` (C-style IO) to bypass C++ iostream overhead.
*   Ensures minimal latency jitter on the main trading thread.

### 4. Matching Logic
*   Standard Price/Time priority matching algorithm.
*   Supports Limit Orders (IOC/FOK partial support).
*   Optimized order book data structures for fast insertion and deletion.

## Build & Run

### Prerequisites
*   CMake >= 3.10
*   GCC/Clang supporting C++17
*   Linux Environment (Recommended)

### Build
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make