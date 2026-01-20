#ifndef DEADLOCK_DETECTOR_H
#define DEADLOCK_DETECTOR_H

#include <pthread.h>
#include <stdint.h>
#include <map>
#include <string>
#include <mutex>
#include <sys/syscall.h>
#include <unistd.h>

// ============================================
// 获取当前线程ID的跨平台封装
// ============================================
inline uint64_t get_thread_id() {
    return static_cast<uint64_t>(syscall(SYS_gettid));
}

// ============================================
// 死锁检测器核心类（单例模式）
// ============================================
class DeadlockDetector {
public:
    // 获取全局唯一实例
    static DeadlockDetector& instance() {
        static DeadlockDetector detector;
        return detector;
    }

    // ========================================
    // 核心钩子函数
    // ========================================
    
    // 在 pthread_mutex_lock 之前调用
    void on_lock_before(uint64_t thread_id, uint64_t lock_addr);
    
    // 在 pthread_mutex_lock 成功之后调用
    void on_lock_after(uint64_t thread_id, uint64_t lock_addr);
    
    // 在 pthread_mutex_unlock 之后调用
    void on_unlock_after(uint64_t thread_id, uint64_t lock_addr);

    // ========================================
    // 调试接口
    // ========================================
    
    // 打印当前系统状态（用于调试）
    void print_status();

private:
    // 单例模式：构造函数私有化
    DeadlockDetector() = default;
    ~DeadlockDetector() = default;
    DeadlockDetector(const DeadlockDetector&) = delete;
    DeadlockDetector& operator=(const DeadlockDetector&) = delete;

    // ========================================
    // 核心数据结构（三张映射表）
    // ========================================
    
    std::map<uint64_t, uint64_t> lock_owners_;      // 锁地址 → 持有者线程ID
    std::map<uint64_t, uint64_t> thread_waiting_;   // 线程ID → 正在申请的锁地址
    std::map<uint64_t, std::string> thread_stacks_; // 线程ID → 堆栈信息（今天暂时为空）

    // ========================================
    // 线程安全保护（今天先用简单的互斥锁）
    // ========================================
    
    std::mutex mutex_lock_owners_;
    std::mutex mutex_thread_waiting_;
    std::mutex mutex_thread_stacks_;
};

// ============================================
// 宏定义：拦截 pthread_mutex_lock
// ============================================
#define pthread_mutex_lock(mutex_ptr)                                        \
    do {                                                                     \
        uint64_t tid = get_thread_id();                                     \
        uint64_t lock_addr = reinterpret_cast<uint64_t>(mutex_ptr);        \
        DeadlockDetector::instance().on_lock_before(tid, lock_addr);        \
        pthread_mutex_lock(mutex_ptr);  /* 实际加锁 */                       \
        DeadlockDetector::instance().on_lock_after(tid, lock_addr);         \
    } while(0)

// ============================================
// 宏定义：拦截 pthread_mutex_unlock
// ============================================
#define pthread_mutex_unlock(mutex_ptr)                                      \
    do {                                                                     \
        pthread_mutex_unlock(mutex_ptr); /* 实际解锁 */                      \
        uint64_t tid = get_thread_id();                                     \
        uint64_t lock_addr = reinterpret_cast<uint64_t>(mutex_ptr);        \
        DeadlockDetector::instance().on_unlock_after(tid, lock_addr);       \
    } while(0)

#endif // DEADLOCK_DETECTOR_H