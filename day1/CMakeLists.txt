cmake_minimum_required(VERSION 3.10)
project(DeadlockDetector)

# 使用 C++11 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall")

# 包含头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 构建库
add_library(deadlock_detector STATIC
    src/deadlock_detector.cpp
)

# 构建测试程序
add_executable(test_basic
    test/test_basic.cpp
)

# 链接库
target_link_libraries(test_basic
    deadlock_detector
    pthread
)