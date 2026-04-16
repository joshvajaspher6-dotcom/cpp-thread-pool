#include "../include/thread_pool.hpp"
#include "../include/cancellation_token.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    cortex::ThreadPool pool(4);

    std::cout << "Starting Chain...\n";

    auto final_future = pool.submit_with_timeout([]() {
        std::cout << "Step 1: Calculating...\n";
        return 15;
    })
    .then([](int val) {
        std::cout << "Step 2: Received " << val << ". Doubling it...\n";
        return val * 2;
    })
    .then([](int val) {
        std::cout << "Step 3: Received " << val << ". Converting to string...\n";
        return std::to_string(val) + " OK";
    })
    .then([](std::string msg) {
        std::cout << "Step 4: Final Result: " << msg << "\n";
    });

    final_future.get();

    std::cout << "Chain Complete.\n";
    return 0;
}