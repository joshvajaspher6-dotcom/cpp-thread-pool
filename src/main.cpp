#include "../include/thread_pool.hpp"
#include "../include/task_group.hpp"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    cortex::ThreadPool pool(4);

    std::cout << "=== Group Wait Test ===\n";
    cortex::TaskGroup group1;
    for (int i = 0; i < 4; ++i) {
        group1.run(pool, [i]() {
            std::cout << "Group1 Task " << i << " running\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            std::cout << "Group1 Task " << i << " done\n";
        });
    }
    std::cout << "Waiting for Group1...\n";
    group1.wait();
    std::cout << "Group1 complete.\n";

    std::cout << "\n=== Group Cancel Test ===\n";
    cortex::TaskGroup group2;
    for (int i = 0; i < 5; ++i) {
        group2.run(pool, [i]() {
            std::cout << "Group2 Task " << i << " running\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::cout << "Group2 Task " << i << " done\n";
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::cout << "Cancelling Group2...\n";
    group2.cancel();
    group2.wait();
    std::cout << "Group2 finished.\n";

    return 0;
}