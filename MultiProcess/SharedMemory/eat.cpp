#include <iostream>
#include <thread>
#include <mutex>
#include <semaphore.h>
#include <vector>
#include <unistd.h>

const int NUM_PHILOSOPHERS = 5;

// 信号量，用于限制最多 4 位哲学家同时尝试进餐（避免死锁）
std::binary_semaphore table(NUM_PHILOSOPHERS - 1);

// 互斥锁，用于每根筷子的访问
std::vector<std::mutex> chopsticks(NUM_PHILOSOPHERS);

void philosopher(int id) {
    while (true) {
        // 哲学家思考
        std::cout << "Philosopher " << id << " is thinking.\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // 尝试拿筷子前，进入临界区（确保不会死锁）
        table.acquire();

        // 拿左筷子
        chopsticks[id].lock();
        std::cout << "Philosopher " << id << " picked up left chopstick.\n";

        // 拿右筷子
        chopsticks[(id + 1) % NUM_PHILOSOPHERS].lock();
        std::cout << "Philosopher " << id << " picked up right chopstick.\n";

        // 进餐
        std::cout << "Philosopher " << id << " is eating.\n";
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // 放下右筷子
        chopsticks[(id + 1) % NUM_PHILOSOPHERS].unlock();
        std::cout << "Philosopher " << id << " put down right chopstick.\n";

        // 放下左筷子
        chopsticks[id].unlock();
        std::cout << "Philosopher " << id << " put down left chopstick.\n";

        // 离开临界区
        table.release();
    }
}

int main() {
    // 创建并启动哲学家的线程
    std::vector<std::thread> philosophers;
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        philosophers.emplace_back(philosopher, i);
    }

    // 等待所有线程结束（实际运行中此程序不会终止）
    for (auto& p : philosophers) {
        p.join();
    }

    return 0;
}