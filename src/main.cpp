#include "../include/thread_pool.hpp"
#include <iostream>
#include <string>
#include <mutex>
#include <vector>
#include <future>
#include <stdexcept>

std::mutex cout_mtx;

void safe_print(const std::string& msg) {
    std::lock_guard<std::mutex> lock(cout_mtx);
    std::cout << msg << "\n";
}

int main() {
    std::cout << "=== Thread Pool — Exception Handling ===\n\n";

    ThreadPool pool(4);

    
    std::cout << "--- Test 1: Normal task ---\n";
    auto f1 = pool.submit_task([](){ return 42; });
    std::cout << "Result: " << f1.get() << "\n\n";

   
    std::cout << "--- Test 2: runtime_error ---\n";
    auto f2 = pool.submit_task([]{
        throw std::runtime_error("task failed!");
        return 0;
    });
    try {
        f2.get();
    } catch (const std::runtime_error& e) {
        std::cout << "Caught: " << e.what() << "\n\n";
    }

   
    std::cout << "--- Test 3: logic_error ---\n";
    auto f3 = pool.submit_task([]{
        throw std::logic_error("invalid argument!");
        return 0;
    });
    try {
        f3.get();
    } catch (const std::logic_error& e) {
        std::cout << "Caught: " << e.what() << "\n\n";
    }

    
    std::cout << "--- Test 4: Mixed tasks ---\n";
    std::vector<std::future<int>> futures;

    for (int i = 0; i < 5; i++) {
        futures.push_back(
            pool.submit_task([i]{
                if (i % 2 == 0)
                    throw std::runtime_error(
                        "task " + std::to_string(i) + " failed");
                return i * 10;
            })
        );
    }

    for (int i = 0; i < 5; i++) {
        try {
            int result = futures[i].get();
            std::cout << "Task " << i
                      << " result: " << result << "\n";
        } catch (const std::exception& e) {
            std::cout << "Task " << i
                      << " error: " << e.what() << "\n";
        }
    }

    std::cout << "\nAll done!\n";
    return 0;
}