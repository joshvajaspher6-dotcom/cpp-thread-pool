#include "../include/thread_pool.hpp"
#include "../include/timed_future.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    cortex::ThreadPool pool(4);

   
    auto fast = pool.submit_with_timeout([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 42;
    });
    
    if (auto result = fast.get_with_timeout(std::chrono::seconds(1))) {
        std::cout << "Fast result: " << *result << "\n";  // Prints: 42
    } else {
        std::cout << "Fast task timed out (unexpected)\n";
    }

    
    auto slow = pool.submit_with_timeout([]() {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return 99;
    });
    
    if (auto result = slow.get_with_timeout(std::chrono::milliseconds(500))) {
        std::cout << "Slow result: " << *result << "\n";
    } else {
        std::cout << "Slow task timed out (expected)\n"; 
    }


    auto checkable = pool.submit_with_timeout([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return std::string("done");
    });
    
    while (!checkable.is_ready()) {
        std::cout << "Still working...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cout << "Final: " << checkable.get() << "\n";


    auto failing = pool.submit_with_timeout([]() -> int {
        throw std::runtime_error("Task failed");
    });
    
    try {
        auto result = failing.get_with_timeout(std::chrono::seconds(1));
        if (result) {
            std::cout << "Result: " << *result << "\n";
        }
    } catch (const std::runtime_error& e) {
        std::cout << "Caught exception: " << e.what() << "\n";  // Prints this
    }

    return 0;
}