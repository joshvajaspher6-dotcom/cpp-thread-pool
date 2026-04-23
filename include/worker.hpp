#pragma once

#include <thread>
#include <atomic>
#include "task_queue.hpp"
#include "priority_task_queue.hpp"


namespace cortex
{
    class Worker
    {
        private:

            TaskQueue& queue_;
            PriorityTaskQueue& priority_queue_;
            std::condition_variable& notify_cv_;
            std::mutex& notify_mtx_;
            std::thread thread_;
            std::atomic<bool> should_stop_{false};
            std::atomic<bool> busy_{false};
            int id_;

            
        public:

            explicit Worker(int id, TaskQueue& queue, PriorityTaskQueue& priority_queue,
                        std::condition_variable& notify_cv, std::mutex& notify_mtx);
            ~Worker();
                
            void run();
            bool is_busy() const;
            int id() const;
            void start();
            void request_stop()
            {
                 should_stop_.store(true, std::memory_order_release); 
            }

            void join() 
            {
                if (thread_.joinable()) {
                    thread_.join();
            }
    }

            Worker(const Worker&) = delete;
            Worker& operator=(const Worker&) = delete;
    };
}