#ifndef DEADLOCK_DETECTOR_H
#define DEADLOCK_DETECTOR_H

#include <pthread.h>
#include <stdint.h>
#include <map>
#include <string>
#include <mutex>
#include <sys/syscall.h>
#include <unistd.h>
#include "graph.h"  // 新增：引入图结构

// 获取线程ID
inline uint64_t get_thread_id() {
    return static_cast<uint64_t>(syscall(SYS_gettid));
}

// ============================================
// 死锁检测器（新增图论检测能力）
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

    // ========================================
    // 新增：死锁检测接口
    // ========================================
    
    // 执行死锁检测（构建图 + 环路检测）
    bool check_deadlock();
    
    // 打印死锁信息
    void print_deadlock_info();

    // 调试接口
    void print_status();

private:
    DeadlockDetector() = default;
    ~DeadlockDetector() = default;
    DeadlockDetector(const DeadlockDetector&) = delete;
    DeadlockDetector& operator=(const DeadlockDetector&) = delete;

    // 核心数据结构（保持不变）
    std::map<uint64_t, uint64_t> lock_owners_;
    std::map<uint64_t, uint64_t> thread_waiting_;
    std::map<uint64_t, std::string> thread_stacks_;

    // 互斥锁保护
    std::mutex mutex_lock_owners_;
    std::mutex mutex_thread_waiting_;
    std::mutex mutex_thread_stacks_;
    
    // ========================================
    // 新增：图结构（用于死锁检测）
    // ========================================
    DirectedGraph graph_;
    std::mutex mutex_graph_;
    
    // ========================================
    // 内部辅助函数
    // ========================================
    
    // 根据当前状态构建等待图
    void build_waiting_graph();
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