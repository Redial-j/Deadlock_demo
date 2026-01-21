cmake_minimum_required(VERSION 3.10)
project(DeadlockDetector)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -g")

include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(deadlock_detector STATIC
    src/deadlock_detector.cpp
    src/graph.cpp
)

add_executable(test_background
    test/test_background.cpp
)

target_link_libraries(test_background
    deadlock_detector
    pthread
)