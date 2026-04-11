#include "../include/thread_pool.hpp"
#include "../include/cancellation_token.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    cortex::ThreadPool pool(4);
    
   
    auto token = std::make_shared<cortex::CancellationToken>();

    
    std::cout << "Submitting 5 tasks...\n";
    for (int i = 0; i < 5; ++i) {
        pool.submit([i, token]() {
            std::cout << "Task " << i << " running...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::cout << "Task " << i << " finished.\n";
        }, token);
    }

   
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

   
    std::cout << "CANCELLING...\n";
    token->cancel();

   
    pool.wait_all();

    std::cout << "All done. Check console to see which tasks were skipped.\n";

    return 0;
}