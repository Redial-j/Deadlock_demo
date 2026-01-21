#include "deadlock_detector.h"
#include <pthread.h>
#include <unistd.h>
#include <iostream>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

// ============================================
// 测试1：自动检测简单死锁
// ============================================
void* deadlock_thread1(void* arg) {
    std::cout << "[Thread1] Started\n";
    
    pthread_mutex_lock(&mutex1);
    std::cout << "[Thread1] Acquired mutex1\n";
    sleep(2);
    
    std::cout << "[Thread1] Trying to acquire mutex2...\n";
    pthread_mutex_lock(&mutex2);
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    
    return nullptr;
}

void* deadlock_thread2(void* arg) {
    std::cout << "[Thread2] Started\n";
    
    pthread_mutex_lock(&mutex2);
    std::cout << "[Thread2] Acquired mutex2\n";
    sleep(2);
    
    std::cout << "[Thread2] Trying to acquire mutex1...\n";
    pthread_mutex_lock(&mutex1);
    
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    
    return nullptr;
}

void test_auto_detection() {
    std::cout << "\n╔═════════════════════════════════════════╗\n";
    std::cout << "║  Test 1: Auto Detection (2 threads)    ║\n";
    std::cout << "╚═════════════════════════════════════════╝\n\n";
    
    // 启动后台检测（每1秒检测一次）
    DeadlockDetector::instance().start(1); // 自动化
    
    pthread_t t1, t2;
    pthread_create(&t1, nullptr, deadlock_thread1, nullptr);
    pthread_create(&t2, nullptr, deadlock_thread2, nullptr);
    
    // 等待检测器发现死锁
    std::cout << "\n[Main] Waiting for detector to find deadlock...\n";
    sleep(5);
    
    // 停止检测器
    DeadlockDetector::instance().stop();
    
    std::cout << "\n[Main] Test finished. Press Ctrl+C to exit.\n";
    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);
}

// ============================================
// 测试2：延迟死锁场景（测试检测间隔）
// ============================================
void* delayed_thread1(void* arg) {
    std::cout << "[DelayedThread1] Sleeping 3 seconds...\n";
    sleep(3);
    
    pthread_mutex_lock(&mutex1);
    std::cout << "[DelayedThread1] Acquired mutex1\n";
    sleep(2);
    
    std::cout << "[DelayedThread1] Trying to acquire mutex2...\n";
    pthread_mutex_lock(&mutex2);
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    return nullptr;
}

void* delayed_thread2(void* arg) {
    std::cout << "[DelayedThread2] Sleeping 3 seconds...\n";
    sleep(3);
    
    pthread_mutex_lock(&mutex2);
    std::cout << "[DelayedThread2] Acquired mutex2\n";
    sleep(2);
    
    std::cout << "[DelayedThread2] Trying to acquire mutex1...\n";
    pthread_mutex_lock(&mutex1);
    
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    return nullptr;
}

void test_delayed_deadlock() {
    std::cout << "\n╔═════════════════════════════════════════╗\n";
    std::cout << "║  Test 2: Delayed Deadlock Detection    ║\n";
    std::cout << "╚═════════════════════════════════════════╝\n\n";
    
    // 启动后台检测（每1秒检测一次）
    DeadlockDetector::instance().start(1);
    
    pthread_t t1, t2;
    pthread_create(&t1, nullptr, delayed_thread1, nullptr);
    pthread_create(&t2, nullptr, delayed_thread2, nullptr);
    
    std::cout << "[Main] Threads will deadlock after 5 seconds...\n";
    sleep(8);
    
    DeadlockDetector::instance().stop();
    
    std::cout << "\n[Main] Test finished.\n";
    // 这里pthread-join()是使线程结束的函数，会等待对应线程结束后再回收资源
    // 因此在测试死锁场景结束后会继续执行后面pthread_mutex_unlock()语句，以避免忘记释放互斥锁。
    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);
}

// ============================================
// 测试3：正常场景（无误报）
// ============================================
void* normal_thread(void* arg) {
    int id = *static_cast<int*>(arg);
    
    for (int i = 0; i < 3; i++) {
        pthread_mutex_lock(&mutex1);
        std::cout << "[NormalThread" << id << "] Working with mutex1 (iteration " << i << ")\n";
        usleep(100000); // 0.1秒
        pthread_mutex_unlock(&mutex1);
        
        usleep(50000);
    }
    
    return nullptr;
}

void test_no_false_positive() {
    std::cout << "\n╔═════════════════════════════════════════╗\n";
    std::cout << "║  Test 3: No False Positive             ║\n";
    std::cout << "╚═════════════════════════════════════════╝\n\n";
    
    DeadlockDetector::instance().start(1);
    
    pthread_t t1, t2;
    int id1 = 1, id2 = 2;
    
    pthread_create(&t1, nullptr, normal_thread, &id1);
    pthread_create(&t2, nullptr, normal_thread, &id2);
    
    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);
    
    std::cout << "\n[Main] All threads finished normally.\n";
    sleep(2); // 给检测器时间确认无死锁
    
    DeadlockDetector::instance().stop();
    
    std::cout << " No deadlock detected - this is correct!\n";
}

// ============================================
// 主函数
// ============================================
int main(int argc, char* argv[]) {
    std::cout << "╔══════════════════════════════════════════════╗\n";
    std::cout << "║  Deadlock Detector - Day 3: Background Mode  ║\n";
    std::cout << "╚══════════════════════════════════════════════╝\n";
    
    if (argc < 2) {
        std::cout << "\nUsage: " << argv[0] << " <test_number>\n";
        std::cout << "  1 - Auto detection (immediate deadlock)\n";
        std::cout << "  2 - Delayed deadlock\n";
        std::cout << "  3 - No false positive\n";
        return 1;
    }
    
    int test_num = atoi(argv[1]);
    
    switch (test_num) {
        case 1:
            test_auto_detection();
            break;
        case 2:
            test_delayed_deadlock();
            break;
        case 3:
            test_no_false_positive();
            break;
        default:
            std::cout << "Invalid test number!\n";
            return 1;
    }
    
    return 0;
}