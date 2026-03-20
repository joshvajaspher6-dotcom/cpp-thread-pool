#pragma once 

#include <vector>
#include <atomic>
#include <memory>
#include <functional>
#include <thread>
#include <future>
#include "task_queue.hpp"
#include "worker.hpp"

class ThreadPool
{
    private:
        TaskQueue queue_;
        std::vector<std::unique_ptr<Worker>> workers_;
        std::atomic<bool> shutdown_{false};
        std::atomic<int> active_task_{0};
        std::atomic<int> num_threads_;

        void spawn_worker(int id);

    public:
        
        explicit ThreadPool(int num_threads =
                                std::thread::hardware_concurrency());
        ~ThreadPool();
        
        void submit(std::function<void()> task);
        void stop();
        void wait_all();
        void resize(int new_size);

        int active_tasks() const;
        int pending_tasks() const;
        int thread_count() const;

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator = (const ThreadPool&) = delete;    

};