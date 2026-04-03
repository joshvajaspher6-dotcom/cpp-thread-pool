#pragma once

#include <thread>
#include <atomic>
#include "task_queue.hpp"
#include "priority_task_queue.hpp"

class Worker
{
    private:
        std::thread thread_;
        TaskQueue& queue_;
        PriorityTaskQueue& priority_queue_;
        std::atomic<bool> busy_{false};
        int id_;

        void run();

    public:
        explicit Worker(int id,TaskQueue& queue_,
                PriorityTaskQueue& priority_queue_);
        ~Worker();
        
        bool is_busy() const;
        int id() const;

        Worker(const Worker&) = delete;
        Worker& operator=(const Worker&) = delete;
};