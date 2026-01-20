#include "deadlock_detector.h"
#include <pthread.h>
#include <unistd.h>
#include <iostream>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;

// ============================================
// 测试1：简单的双线程死锁（A-B-A 模式）
// ============================================
void* deadlock_thread1(void* arg) {
    std::cout << "[Thread1] Started\n";
    
    pthread_mutex_lock(&mutex1);
    std::cout << "[Thread1] Acquired mutex1\n";
    sleep(1);
    
    std::cout << "[Thread1] Trying to acquire mutex2...\n";
    pthread_mutex_lock(&mutex2);  // 会死锁！
    std::cout << "[Thread1] Acquired mutex2\n";
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    
    return nullptr;
}

void* deadlock_thread2(void* arg) {
    std::cout << "[Thread2] Started\n";
    
    pthread_mutex_lock(&mutex2);
    std::cout << "[Thread2] Acquired mutex2\n";
    sleep(1);
    
    std::cout << "[Thread2] Trying to acquire mutex1...\n";
    pthread_mutex_lock(&mutex1);  // 会死锁！
    std::cout << "[Thread2] Acquired mutex1\n";
    
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    
    return nullptr;
}

void test_simple_deadlock() {
    std::cout << "\n╔═══════════════════════════════════════╗\n";
    std::cout << "║  Test 1: Simple Deadlock (2 threads) ║\n";
    std::cout << "╚═══════════════════════════════════════╝\n\n";
    
    pthread_t t1, t2;
    pthread_create(&t1, nullptr, deadlock_thread1, nullptr);
    pthread_create(&t2, nullptr, deadlock_thread2, nullptr);
    
    // 等待死锁形成
    sleep(2);
    
    // 执行检测
    if (DeadlockDetector::instance().check_deadlock()) {
        DeadlockDetector::instance().print_deadlock_info();
    } else {
        std::cout << " No deadlock detected.\n";
    }
    
    // 注意：这里不能 join，因为线程已经死锁
    std::cout << "\n⚠️  Threads are deadlocked. Press Ctrl+C to exit.\n";
    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);
}

// ============================================
// 测试2：三线程环形死锁（A-B-C-A 模式）
// ============================================
void* cyclic_thread1(void* arg) {
    pthread_mutex_lock(&mutex1);
    std::cout << "[CyclicThread1] Acquired mutex1\n";
    sleep(1);
    pthread_mutex_lock(&mutex2);  // 等待 Thread2
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    return nullptr;
}

void* cyclic_thread2(void* arg) {
    pthread_mutex_lock(&mutex2);
    std::cout << "[CyclicThread2] Acquired mutex2\n";
    sleep(1);
    pthread_mutex_lock(&mutex3);  // 等待 Thread3
    
    pthread_mutex_unlock(&mutex3);
    pthread_mutex_unlock(&mutex2);
    return nullptr;
}

void* cyclic_thread3(void* arg) {
    pthread_mutex_lock(&mutex3);
    std::cout << "[CyclicThread3] Acquired mutex3\n";
    sleep(1);
    pthread_mutex_lock(&mutex1);  // 等待 Thread1
    
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex3);
    return nullptr;
}

void test_cyclic_deadlock() {
    std::cout << "\n╔═══════════════════════════════════════╗\n";
    std::cout << "║  Test 2: Cyclic Deadlock (3 threads) ║\n";
    std::cout << "╚═══════════════════════════════════════╝\n\n";
    
    pthread_t t1, t2, t3;
    pthread_create(&t1, nullptr, cyclic_thread1, nullptr);
    pthread_create(&t2, nullptr, cyclic_thread2, nullptr);
    pthread_create(&t3, nullptr, cyclic_thread3, nullptr);
    
    sleep(2);
    
    if (DeadlockDetector::instance().check_deadlock()) {
        DeadlockDetector::instance().print_deadlock_info();
    } else {
        std::cout << " No deadlock detected.\n";
    }
    
    std::cout << "\n⚠️  Threads are deadlocked. Press Ctrl+C to exit.\n";
    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);
    pthread_join(t3, nullptr);
}

// ============================================
// 测试3：正常场景（无死锁）
// ============================================
void* normal_thread(void* arg) {
    int id = *static_cast<int*>(arg);
    
    // 所有线程按相同顺序加锁
    pthread_mutex_lock(&mutex1);
    std::cout << "[NormalThread" << id << "] Acquired mutex1\n";
    
    pthread_mutex_lock(&mutex2);
    std::cout << "[NormalThread" << id << "] Acquired mutex2\n";
    
    usleep(100000); // 0.1秒
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    
    return nullptr;
}

void test_no_deadlock() {
    std::cout << "\n╔═══════════════════════════════════════╗\n";
    std::cout << "║  Test 3: No Deadlock (correct order) ║\n";
    std::cout << "╚═══════════════════════════════════════╝\n\n";
    
    pthread_t t1, t2;
    int id1 = 1, id2 = 2;
    
    pthread_create(&t1, nullptr, normal_thread, &id1);
    pthread_create(&t2, nullptr, normal_thread, &id2);
    
    sleep(1);
    
    if (DeadlockDetector::instance().check_deadlock()) {
        DeadlockDetector::instance().print_deadlock_info();
    } else {
        std::cout << " No deadlock detected. All threads executed correctly!\n";
    }
    
    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);
}

// ============================================
// 主函数：选择测试场景
// ============================================
int main(int argc, char* argv[]) {
    std::cout << "╔══════════════════════════════════════════════╗\n";
    std::cout << "║  Deadlock Detector - Day 2: Graph Detection  ║\n";
    std::cout << "╚══════════════════════════════════════════════╝\n";
    
    if (argc < 2) {
        std::cout << "\nUsage: " << argv[0] << " <test_number>\n";
        std::cout << "  1 - Simple deadlock (2 threads)\n";
        std::cout << "  2 - Cyclic deadlock (3 threads)\n";
        std::cout << "  3 - No deadlock (correct lock order)\n";
        return 1;
    }
    
    int test_num = atoi(argv[1]);
    
    switch (test_num) {
        case 1:
            test_simple_deadlock();
            break;
        case 2:
            test_cyclic_deadlock();
            break;
        case 3:
            test_no_deadlock();
            break;
        default:
            std::cout << "Invalid test number!\n";
            return 1;
    }
    
    return 0;
}