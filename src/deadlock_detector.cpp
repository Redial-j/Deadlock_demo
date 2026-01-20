#include "deadlock_detector.h"
#include <iostream>
#include <iomanip>

// ============================================
// 钩子函数实现（保持不变）
// ============================================
void DeadlockDetector::on_lock_before(uint64_t thread_id, uint64_t lock_addr) {
    {
        std::lock_guard<std::mutex> guard(mutex_thread_waiting_);
        thread_waiting_[thread_id] = lock_addr;
    }
    
    {
        std::lock_guard<std::mutex> guard(mutex_thread_stacks_);
        thread_stacks_[thread_id] = "[Stack trace placeholder]";
    }
}

void DeadlockDetector::on_lock_after(uint64_t thread_id, uint64_t lock_addr) {
    {
        std::lock_guard<std::mutex> guard(mutex_thread_waiting_);
        thread_waiting_.erase(thread_id);
    }
    
    {
        std::lock_guard<std::mutex> guard(mutex_thread_stacks_);
        thread_stacks_.erase(thread_id);
    }
    
    {
        std::lock_guard<std::mutex> guard(mutex_lock_owners_);
        lock_owners_[lock_addr] = thread_id;
    }
}

void DeadlockDetector::on_unlock_after(uint64_t thread_id, uint64_t lock_addr) {
    std::lock_guard<std::mutex> guard(mutex_lock_owners_);
    lock_owners_.erase(lock_addr);
}

// ============================================
// 新增：构建等待图 
// ============================================
void DeadlockDetector::build_waiting_graph() {
    // 1. 清空旧图
    graph_.clear();
    
    // 2. 获取当前状态的快照（避免长时间持有锁）
    std::map<uint64_t, uint64_t> lock_owners_snapshot;
    std::map<uint64_t, uint64_t> thread_waiting_snapshot;
    
    {
        std::lock_guard<std::mutex> guard(mutex_lock_owners_);
        lock_owners_snapshot = lock_owners_;
    }
    
    {
        std::lock_guard<std::mutex> guard(mutex_thread_waiting_);
        thread_waiting_snapshot = thread_waiting_;
    }
    
    // 3. 构建图：遍历所有"正在等待"的线程
    for (const auto& pair : thread_waiting_snapshot) {
        uint64_t waiting_thread = pair.first;   // 正在等待的线程
        uint64_t requested_lock = pair.second;  // 它想要的锁
        
        // 4. 查询这把锁被谁持有
        auto it = lock_owners_snapshot.find(requested_lock);
        if (it != lock_owners_snapshot.end()) {
            uint64_t owner_thread = it->second; // 持有锁的线程
            
            // 5. 添加边：waiting_thread → owner_thread
            //    含义："等待线程"依赖"持有线程"释放锁
            graph_.add_edge(waiting_thread, owner_thread);
        }
    }
}

// ============================================
// 新增：检查死锁
// ============================================
bool DeadlockDetector::check_deadlock() {
    std::lock_guard<std::mutex> guard(mutex_graph_);
    
    // 1. 构建等待图
    build_waiting_graph();
    
    // 2. 检测环路
    bool has_deadlock = graph_.has_cycle();
    
    return has_deadlock;
}

// ============================================
// 新增：打印死锁信息
// ============================================
void DeadlockDetector::print_deadlock_info() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════╗\n";
    std::cout << "║  ⚠️  DEADLOCK DETECTED!  ⚠️                    ║\n";
    std::cout << "╚════════════════════════════════════════════════╝\n\n";
    
    // 打印涉及死锁的线程
    std::lock_guard<std::mutex> guard(mutex_graph_);
    std::vector<uint64_t> deadlock_threads = graph_.get_all_nodes();
    
    std::cout << "Threads involved in deadlock:\n";
    for (size_t i = 0; i < deadlock_threads.size(); i++) {
        uint64_t tid = deadlock_threads[i];
        
        // 查询该线程在等待什么锁
        uint64_t waiting_lock = 0;
        {
            std::lock_guard<std::mutex> g(mutex_thread_waiting_);
            auto it = thread_waiting_.find(tid);
            if (it != thread_waiting_.end()) {
                waiting_lock = it->second;
            }
        }
        
        // 查询该锁被谁持有
        uint64_t owner = 0;
        {
            std::lock_guard<std::mutex> g(mutex_lock_owners_);
            auto it = lock_owners_.find(waiting_lock);
            if (it != lock_owners_.end()) {
                owner = it->second;
            }
        }
        
        std::cout << "  Thread " << tid 
                  << " is waiting for lock 0x" << std::hex << waiting_lock << std::dec
                  << " (held by Thread " << owner << ")\n";
    }
    
    // 打印图结构
    graph_.print_graph();
    
    std::cout << " Recommendation: Check the lock acquisition order in your code!\n\n";
}

// ============================================
// 调试接口（保持不变）
// ============================================
void DeadlockDetector::print_status() {
    std::cout << "\n========== Deadlock Detector Status ==========\n";
    
    {
        std::lock_guard<std::mutex> guard(mutex_lock_owners_);
        std::cout << "Lock Owners (" << lock_owners_.size() << " locks held):\n";
        for (const auto& pair : lock_owners_) {
            std::cout << "  Lock 0x" << std::hex << pair.first 
                      << " → Thread " << std::dec << pair.second << "\n";
        }
    }
    
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