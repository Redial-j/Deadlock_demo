#ifndef DEADLOCK_DETECTOR_H
#define DEADLOCK_DETECTOR_H

#include <pthread.h>
#include <stdint.h>
#include <map>
#include <string>
#include <mutex>
#include <thread>      // 新增：C++11 线程
#include <atomic>      // 新增：原子变量
#include <sys/syscall.h>
#include <unistd.h>
#include "graph.h"

inline uint64_t get_thread_id() {
    return static_cast<uint64_t>(syscall(SYS_gettid));
}

// ============================================
// 死锁检测器（新增后台检测能力）
// ============================================
class DeadlockDetector {
public:
    static DeadlockDetector& instance() {
        static DeadlockDetector detector;
        return detector;
    }

    // 钩子函数（保持不变）
    void on_lock_before(uint64_t thread_id, uint64_t lock_addr);
    void on_lock_after(uint64_t thread_id, uint64_t lock_addr);
    void on_unlock_after(uint64_t thread_id, uint64_t lock_addr);

    // 检测接口（保持不变）
    bool check_deadlock();
    void print_deadlock_info();
    void print_status();

    // ========================================
    // 新增：后台检测接口
    // ========================================
    
    // 启动后台检测线程
    void start(int interval_seconds = 1);
    
    // 停止后台检测线程
    void stop();
    
    // 查询检测线程是否正在运行
    bool is_running() const { return running_.load(); }
    
    // 设置检测间隔（秒）
    void set_interval(int seconds) { interval_seconds_ = seconds; }

private:
    DeadlockDetector() 
        : running_(false), 
          interval_seconds_(1),
          deadlock_detected_(false) {}
    
    ~DeadlockDetector() {
        stop(); // 确保析构时停止检测线程
    }
    
    DeadlockDetector(const DeadlockDetector&) = delete;
    DeadlockDetector& operator=(const DeadlockDetector&) = delete;

    // 核心数据结构（保持不变）
    std::map<uint64_t, uint64_t> lock_owners_;
    std::map<uint64_t, uint64_t> thread_waiting_;
    std::map<uint64_t, std::string> thread_stacks_;

    // ========================================
    // 互斥锁保护（关键优化：使用 std::lock）
    // ========================================
    std::mutex mutex_lock_owners_;
    std::mutex mutex_thread_waiting_;
    std::mutex mutex_thread_stacks_;
    
    DirectedGraph graph_;
    std::mutex mutex_graph_;
    
    // ========================================
    // 新增：后台检测相关成员
    // ========================================
    std::thread detector_thread_;        // 后台检测线程
    std::atomic<bool> running_;          // 原子变量：线程运行标志
    int interval_seconds_;               // 检测间隔（秒）
    std::atomic<bool> deadlock_detected_; // 是否已检测到死锁
    
    // ========================================
    // 内部辅助函数
    // ========================================
    void build_waiting_graph();
    
    // 后台检测线程的主循环
    void detector_loop();
    
    // 安全地获取快照（使用 std::lock 避免死锁）
    void get_snapshot(
        std::map<uint64_t, uint64_t>& lock_owners,
        std::map<uint64_t, uint64_t>& thread_waiting,
        std::map<uint64_t, std::string>& thread_stacks
    );
};

// 宏定义（保持不变）
#define pthread_mutex_lock(mutex_ptr)                                        \
    do {                                                                     \
        uint64_t tid = get_thread_id();                                     \
        uint64_t lock_addr = reinterpret_cast<uint64_t>(mutex_ptr);        \
        DeadlockDetector::instance().on_lock_before(tid, lock_addr);        \
        pthread_mutex_lock(mutex_ptr);                                      \
        DeadlockDetector::instance().on_lock_after(tid, lock_addr);         \
    } while(0)

#define pthread_mutex_unlock(mutex_ptr)                                      \
    do {                                                                     \
        pthread_mutex_unlock(mutex_ptr);                                    \
        uint64_t tid = get_thread_id();                                     \
        uint64_t lock_addr = reinterpret_cast<uint64_t>(mutex_ptr);        \
        DeadlockDetector::instance().on_unlock_after(tid, lock_addr);       \
    } while(0)

#endif // DEADLOCK_DETECTOR_H