cmake_minimum_required(VERSION 3.10)
project(DeadlockDetector)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -g")

include_directories(${PROJECT_SOURCE_DIR}/include)

# 构建库（新增 graph.cpp）
add_library(deadlock_detector STATIC
    src/deadlock_detector.cpp
    src/graph.cpp
)

# 构建测试程序
add_executable(test_detection
    test/test_detection.cpp
)

target_link_libraries(test_detection
    deadlock_detector
    pthread
)