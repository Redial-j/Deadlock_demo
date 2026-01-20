#include "deadlock_detector.h"
#include <iostream>
#include <iomanip>

// ============================================
// 实现：加锁前的钩子
// ============================================
void DeadlockDetector::on_lock_before(uint64_t thread_id, uint64_t lock_addr) {
    // 1. 记录"线程thread_id正在申请锁lock_addr"
    {
        std::lock_guard<std::mutex> guard(mutex_thread_waiting_);
        thread_waiting_[thread_id] = lock_addr;
    }

    // 2. 保存堆栈信息（今天暂时用占位字符串，明天实现真正的堆栈捕获）
    {
        std::lock_guard<std::mutex> guard(mutex_thread_stacks_);
        thread_stacks_[thread_id] = "[Stack trace placeholder]";
    }

    // 调试输出
    std::cout << "[BEFORE] Thread " << thread_id 
              << " is requesting lock 0x" << std::hex << lock_addr << std::dec
              << std::endl;
}

// ============================================
// 实现：加锁成功后的钩子
// ============================================
void DeadlockDetector::on_lock_after(uint64_t thread_id, uint64_t lock_addr) {
    // 1. 清除申请记录（因为已经拿到锁了）
    {
        std::lock_guard<std::mutex> guard(mutex_thread_waiting_);
        thread_waiting_.erase(thread_id);
    }

    // 2. 清除堆栈记录
    {
        std::lock_guard<std::mutex> guard(mutex_thread_stacks_);
        thread_stacks_.erase(thread_id);
    }

    // 3. 记录"锁lock_addr被线程thread_id持有"
    {
        std::lock_guard<std::mutex> guard(mutex_lock_owners_);
        lock_owners_[lock_addr] = thread_id;
    }

    // 调试输出
    std::cout << "[AFTER]  Thread " << thread_id 
              << " acquired lock 0x" << std::hex << lock_addr << std::dec
              << std::endl;
}

// ============================================
// 实现：解锁后的钩子
// ============================================
void DeadlockDetector::on_unlock_after(uint64_t thread_id, uint64_t lock_addr) {
    // 清除"锁被持有"的记录
    {
        std::lock_guard<std::mutex> guard(mutex_lock_owners_);
        lock_owners_.erase(lock_addr);
    }

    // 调试输出
    std::cout << "[UNLOCK] Thread " << thread_id 
              << " released lock 0x" << std::hex << lock_addr << std::dec
              << std::endl;
}

// ============================================
// 实现：打印当前系统状态
// ============================================
void DeadlockDetector::print_status() {
    std::cout << "\n========== Deadlock Detector Status ==========\n";

    // 打印"锁的持有者"表
    {
        std::lock_guard<std::mutex> guard(mutex_lock_owners_);
        std::cout << "Lock Owners (" << lock_owners_.size() << " locks held):\n";
        for (const auto& pair : lock_owners_) {
            std::cout << "  Lock 0x" << std::hex << pair.first 
                      << " → Thread " << std::dec << pair.second << "\n";
        }
    }

    // 打印"线程等待"表
    {
        std::lock_guard<std::mutex> guard(mutex_thread_waiting_);
        std::cout << "Threads Waiting (" << thread_waiting_.size() << " threads):\n";
        for (const auto& pair : thread_waiting_) {
            std::cout << "  Thread " << pair.first 
                      << " → waiting for lock 0x" << std::hex << pair.second << std::dec << "\n";
        }
    }

    std::cout << "=============================================\n\n";
}