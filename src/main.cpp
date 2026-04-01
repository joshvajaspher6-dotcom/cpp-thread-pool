#include "../include/thread_pool.hpp"
#include "../include/priority.hpp"
#include <iostream>
#include <string>
#include <mutex>

std::mutex cout_mtx;

void safe_print(const std::string& msg) {
    std::lock_guard<std::mutex> lock(cout_mtx);
    std::cout << msg << "\n";
}

int main() {
    std::cout << "=== Thread Pool — Priority Tasks ===\n\n";

    ThreadPool pool(4);

    pool.submit([]{
        safe_print("LOW      task running");
    }, Priority::LOW);

    pool.submit([]{
        safe_print("MEDIUM   task running");
    }, Priority::MEDIUM);

    pool.submit([]{
        safe_print("HIGH     task running");
    }, Priority::HIGH);

    pool.submit([]{
        safe_print("CRITICAL task running");
    }, Priority::CRITICAL);

    pool.submit([]{
        safe_print("MEDIUM   task 2 running");
    }, Priority::MEDIUM);

    pool.submit([]{
        safe_print("LOW      task 2 running");
    }, Priority::LOW);

    pool.submit([]{
        safe_print("HIGH     task 2 running");
    }, Priority::HIGH);

    pool.wait_all();

    std::cout << "\nAll done!\n";
    return 0;
}