#include "deadlock_detector.h"
#include <iostream>
#include <iomanip>
#include <chrono>
/*
死锁检测器是被多个线程同时使用的
例如，业务线程会访问与修改映射表和图的状态，检测线程检测死锁同样也会访问图,
因此对于每张映射表和图，也各需要一个锁以让这些线程互斥访问这些资源。
这就同样带来了死锁检测器自身可能产生的死锁问题，需要采用一些措施来规避。
*/
// ============================================
// 钩子函数
// 修改：使用 std::lock 避免死锁
// 核心原理：同时获取多个锁，获取不到就等待, 本质上是一种原子操作
// ============================================
void DeadlockDetector::on_lock_before(uint64_t thread_id, uint64_t lock_addr) {
    // 关键：同时获取两个锁，避免死锁
    std::lock(mutex_thread_waiting_, mutex_thread_stacks_);
    
    {
        std::lock_guard<std::mutex> guard(mutex_thread_waiting_, std::adopt_lock);
        thread_waiting_[thread_id] = lock_addr;
    }
    
    {
        std::lock_guard<std::mutex> guard(mutex_thread_stacks_, std::adopt_lock);
        thread_stacks_[thread_id] = "[Stack trace placeholder]";
    }
}

void DeadlockDetector::on_lock_after(uint64_t thread_id, uint64_t lock_addr) {
    // 同时获取三个锁
    std::lock(mutex_thread_waiting_, mutex_thread_stacks_, mutex_lock_owners_);
    
    {
        std::lock_guard<std::mutex> guard(mutex_thread_waiting_, std::adopt_lock);
        thread_waiting_.erase(thread_id);
    }
    
    {
        std::lock_guard<std::mutex> guard(mutex_thread_stacks_, std::adopt_lock);
        thread_stacks_.erase(thread_id);
    }
    
    {
        std::lock_guard<std::mutex> guard(mutex_lock_owners_, std::adopt_lock);
        lock_owners_[lock_addr] = thread_id;
    }
}

void DeadlockDetector::on_unlock_after(uint64_t thread_id, uint64_t lock_addr) {
    std::lock_guard<std::mutex> guard(mutex_lock_owners_);
    lock_owners_.erase(lock_addr);
}

/*
构建死锁等待图的时间可能很长，为了避免开销，采用快速复制的方式将某一时刻的映射表存起来
这样可以避免死锁检测线程占用锁的时间过长而影响业务线程的性能。
*/
// ============================================
// 新增：安全获取快照（核心优化）
// ============================================
void DeadlockDetector::get_snapshot(
    std::map<uint64_t, uint64_t>& lock_owners,
    std::map<uint64_t, uint64_t>& thread_waiting,
    std::map<uint64_t, std::string>& thread_stacks) {
    
    // 使用 std::lock 同时获取所有锁，避免死锁
    std::lock(mutex_lock_owners_, mutex_thread_waiting_, mutex_thread_stacks_);
    
    {
        std::lock_guard<std::mutex> g1(mutex_lock_owners_, std::adopt_lock);
        lock_owners = lock_owners_;
    }
    
    {
        std::lock_guard<std::mutex> g2(mutex_thread_waiting_, std::adopt_lock);
        thread_waiting = thread_waiting_;
    }
    
    {
        std::lock_guard<std::mutex> g3(mutex_thread_stacks_, std::adopt_lock);
        thread_stacks = thread_stacks_;
    }
}


// ============================================
// 构建等待图（使用快照数据）
// ============================================
void DeadlockDetector::build_waiting_graph() {
    graph_.clear();
    
    // 获取快照
    std::map<uint64_t, uint64_t> lock_owners_snapshot;
    std::map<uint64_t, uint64_t> thread_waiting_snapshot;
    std::map<uint64_t, std::string> thread_stacks_snapshot;
    
    get_snapshot(lock_owners_snapshot, thread_waiting_snapshot, thread_stacks_snapshot);
    
    // 构建图
    for (const auto& pair : thread_waiting_snapshot) {
        uint64_t waiting_thread = pair.first;
        uint64_t requested_lock = pair.second;
        
        auto it = lock_owners_snapshot.find(requested_lock);
        if (it != lock_owners_snapshot.end()) {
            uint64_t owner_thread = it->second;
            graph_.add_edge(waiting_thread, owner_thread);
        }
    }
}

// ============================================
// 检查死锁（保持不变）
// ============================================
bool DeadlockDetector::check_deadlock() {
    std::lock_guard<std::mutex> guard(mutex_graph_);
    build_waiting_graph();
    return graph_.has_cycle();
}

// ============================================
// 打印死锁信息（保持不变）
// ============================================
void DeadlockDetector::print_deadlock_info() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════╗\n";
    std::cout << "║  ⚠️  DEADLOCK DETECTED!  ⚠️                    ║\n";
    std::cout << "╚════════════════════════════════════════════════╝\n\n";
    
    std::lock_guard<std::mutex> guard(mutex_graph_);
    std::vector<uint64_t> deadlock_threads = graph_.get_all_nodes();
    
    std::cout << "Threads involved in deadlock:\n";
    for (size_t i = 0; i < deadlock_threads.size(); i++) {
        uint64_t tid = deadlock_threads[i];
        
        uint64_t waiting_lock = 0;
        {
            std::lock_guard<std::mutex> g(mutex_thread_waiting_);
            auto it = thread_waiting_.find(tid);
            if (it != thread_waiting_.end()) {
                waiting_lock = it->second;
            }
        }
        
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
    
    graph_.print_graph();
    
    std::cout << " Recommendation: Check the lock acquisition order in your code!\n\n";
}

// 该死锁检测循环函数由函数指针传给自己创建的检测线程调用，并不影响业务线程的运行，而是与业务线程并发运行。 
// 两个线程的栈区独立，堆区共享
// ============================================
// 新增：后台检测线程的主循环
// ============================================
void DeadlockDetector::detector_loop() {
    std::cout << "[Detector Thread] Started, checking every " 
              << interval_seconds_ << " second(s)\n";
    
    while (running_.load()) {
        // 等待检测间隔
        std::this_thread::sleep_for(std::chrono::seconds(interval_seconds_));
        
        // 执行检测
        if (check_deadlock()) {
            // 检测到死锁
            if (!deadlock_detected_.load()) {
                // 第一次检测到，打印报警
                deadlock_detected_.store(true);
                
                std::cout << "\n[Detector Thread] ⚠️  Deadlock detected at "
                          << std::time(nullptr) << "\n";
                print_deadlock_info();
                
                // 发现死锁后退出检测循环
                break;
            }
        }
        // else{ // 检验死锁检测间隔时用
        //    std::cout << "There's no deadlock in the last " << interval_seconds_ << " second" << "\n";
        // }
    }
    
    std::cout << "[Detector Thread] Stopped\n";
}

// ============================================
// 新增：启动后台检测
// ============================================
void DeadlockDetector::start(int interval_seconds) {
    if (running_.load()) {
        std::cout << "[Warning] Detector thread is already running!\n";
        return;
    }
    
    interval_seconds_ = interval_seconds;
    running_.store(true);
    deadlock_detected_.store(false);
    
    // 创建检测线程
    detector_thread_ = std::thread(&DeadlockDetector::detector_loop, this);
    
    std::cout << "[DeadlockDetector] Background detection started\n";
}

// 必须在死锁检测结束后调用，手动释放死锁检测进程
// ============================================
// 新增：停止后台检测
// ============================================
void DeadlockDetector::stop() {
    if (!running_.load()) {
        return; // 已经停止
    }
    
    std::cout << "[DeadlockDetector] Stopping background detection...\n";
    
    running_.store(false);
    
    // 等待检测线程退出
    if (detector_thread_.joinable()) {
        detector_thread_.join();
    }
    
    std::cout << "[DeadlockDetector] Background detection stopped\n";
}

// ============================================
// 打印状态（保持不变）
// ============================================
void DeadlockDetector::print_status() {
    std::cout << "\n========== Deadlock Detector Status ==========\n";
    
    std::map<uint64_t, uint64_t> lock_owners_snapshot;
    std::map<uint64_t, uint64_t> thread_waiting_snapshot;
    std::map<uint64_t, std::string> thread_stacks_snapshot;
    
    get_snapshot(lock_owners_snapshot, thread_waiting_snapshot, thread_stacks_snapshot);
    
    std::cout << "Lock Owners (" << lock_owners_snapshot.size() << " locks held):\n";
    for (const auto& pair : lock_owners_snapshot) {
        std::cout << "  Lock 0x" << std::hex << pair.first 
                  << " → Thread " << std::dec << pair.second << "\n";
    }
    
    std::cout << "Threads Waiting (" << thread_waiting_snapshot.size() << " threads):\n";
    for (const auto& pair : thread_waiting_snapshot) {
        std::cout << "  Thread " << pair.first 
                  << " → waiting for lock 0x" << std::hex << pair.second << std::dec << "\n";
    }
    
    std::cout << "=============================================\n\n";
}