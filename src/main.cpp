#include "../include/thread_pool.hpp"
#include <iostream>
#include <string>
#include <mutex>
#include <vector>
#include <future>

std::mutex cout_mtx;

void safe_print(const std::string& msg) {
    std::lock_guard<std::mutex> lock(cout_mtx);
    std::cout << msg << "\n";
}

int main() {
    std::cout << "=== Thread Pool — Future Support ===\n\n";

    ThreadPool pool(4);

    std::cout << "--- Test 1: Return int ---\n";
    auto f1 = pool.submit_task([]{ return 42; });
    std::cout << "Result: " << f1.get() << "\n\n";

    std::cout << "--- Test 2: Return string ---\n";
    auto f2 = pool.submit_task([]{
        return std::string("hello from thread");
    });
    std::cout << "Result: " << f2.get() << "\n\n";

    std::cout << "--- Test 3: Calculation ---\n";
    auto f3 = pool.submit_task([]{
        int sum = 0;
        for (int i = 1; i <= 100; i++) sum += i;
        return sum;
    });
    std::cout << "Sum 1-100: " << f3.get() << "\n\n";

    std::cout << "--- Test 4: Multiple Futures ---\n";
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 5; i++) {
        futures.push_back(
            pool.submit_task([i]{ return i * i; })
        );
    }
    for (int i = 0; i < 5; i++) {
        std::cout << i << " squared = "
                  << futures[i].get() << "\n";
    }

    std::cout << "\nAll done!\n";
    return 0;
}