cmake_minimum_required(VERSION 3.10)
project(LionTrader)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# HFT 常用编译选项：最大优化，针对当前 CPU 指令集优化
# -O3: 最高优化等级
# -march=native: 使用本机 CPU 的所有高级指令集 (AVX2, AVX512 等)
# -Wall: 开启警告
# -pthread: 多线程支持
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -Wall -pthread")

# 打印一下当前的构建模式
message(STATUS "Current Build Type: ${CMAKE_BUILD_TYPE}")

# 包含头文件路径
include_directories(include)
include_directories(src/core)
include_directories(src/engine)

# --- 主程序 ---
# file(GLOB_RECURSE SOURCES "src/*.cpp") # 暂时注释掉，避免 main.cpp 和 test_match.cpp 冲突
add_executable(LionTrader 
    src/main.cpp 
    src/engine/MdEngine.cpp 
    src/engine/TraderEngine.cpp
)

# --- 单元测试 ---
# 编译 tests 目录下的测试代码
add_executable(UnitTest tests/test_match.cpp)