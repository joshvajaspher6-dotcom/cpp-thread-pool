#include "../include/thread_pool.hpp"
#include <mutex>
#include <iostream>
#include <string>

std::mutex cout_mtx;

void safe_print(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(cout_mtx);
    std::cout << msg <<std::endl;
}

int main()
{
    std::cout << "=== Thread Pool Test ===\n\n";

    ThreadPool pool(4);

    std::cout << "Thread count: "
              << pool.thread_count() << "\n\n";

    for (int i=0;i<4;i++)
    {
        pool.submit([i]{
            safe_print("Task" + std::to_string(i) + "running");
        
    });
    }
     pool.wait_all();

    std::cout << "\nAll tasks done!\n";
    std::cout << "Active tasks:  "
              << pool.active_tasks() << "\n";
    std::cout << "Pending tasks: "
              << pool.pending_tasks() << "\n";

}