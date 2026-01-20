#include "deadlock_detector.h"
#include <pthread.h>
#include <unistd.h>
#include <iostream>

// 全局互斥锁
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

// ============================================
// 线程1：先锁mutex1，再锁mutex2
// ============================================
void* thread1_func(void* arg) {
    std::cout << "\n=== Thread1 started ===\n";
    
    pthread_mutex_lock(&mutex1);
    std::cout << "Thread1: holding mutex1, sleeping...\n";
    sleep(1); // 模拟工作
    
    pthread_mutex_lock(&mutex2);
    std::cout << "Thread1: acquired mutex2!\n";
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    
    std::cout << "=== Thread1 finished ===\n";
    return nullptr;
}

// ============================================
// 线程2：先锁mutex2，再锁mutex1（会产生死锁！）
// ============================================
void* thread2_func(void* arg) {
    std::cout << "\n=== Thread2 started ===\n";
    
    pthread_mutex_lock(&mutex2);
    std::cout << "Thread2: holding mutex2, sleeping...\n";
    sleep(1); // 模拟工作
    
    pthread_mutex_lock(&mutex1); // 这里会死锁！
    std::cout << "Thread2: acquired mutex1!\n";
    
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    
    std::cout << "=== Thread2 finished ===\n";
    return nullptr;
}

// ============================================
// 测试：正常场景（无死锁）
// ============================================
void test_normal_case() {
    std::cout << "\n########## Test: Normal Case ##########\n";
    
    pthread_mutex_lock(&mutex1);
    std::cout << "Main: acquired mutex1\n";
    
    pthread_mutex_unlock(&mutex1);
    std::cout << "Main: released mutex1\n";
    
    DeadlockDetector::instance().print_status();
}

// ============================================
// 测试：死锁场景
// ============================================
void test_deadlock_case() {
    std::cout << "\n########## Test: Deadlock Case ##########\n";
    
    pthread_t t1, t2;
    pthread_create(&t1, nullptr, thread1_func, nullptr);
    pthread_create(&t2, nullptr, thread2_func, nullptr);
    
    // 等待2秒，让死锁发生
    sleep(2);
    
    // 打印状态（会看到两个线程互相等待）
    DeadlockDetector::instance().print_status();
    
    std::cout << "\n⚠️  Deadlock detected! Program will hang here...\n";
    std::cout << "Press Ctrl+C to exit.\n";
    
    pthread_join(t1, nullptr); // 这里会永远等待
    pthread_join(t2, nullptr);
}

int main() {
    std::cout << "Deadlock Detector - Day 1 Test\n";
    std::cout << "================================\n";
    
    // 测试1：正常场景
    test_normal_case();
    
    // 测试2：死锁场景（会卡住程序）
    test_deadlock_case();
    
    return 0;
}