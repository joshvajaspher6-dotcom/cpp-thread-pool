#pragma once

#include <queue>
#include "priority.hpp" 
#include "task.hpp"
#include <functional>
#include <mutex>
#include <atomic>
#include <optional>
#include <condition_variable>

namespace cortex 
{


struct PrioritizedTask
{
    cortex::Task task;
    Priority priority;

    bool operator<(const PrioritizedTask& other) const
    {
        return priority < other.priority;
    }

};

    class PriorityTaskQueue
    {
        private:
            std::priority_queue<PrioritizedTask> queue_;
            mutable std::mutex mtx_;
            std::condition_variable cv_;
            std::atomic<bool> shutdown_{false};

        public:
            
            void push(cortex::Task task, Priority priority = Priority::MEDIUM)
            {
                {
                    std::lock_guard<std::mutex>lock(mtx_);
                    if(shutdown_) return;
                    queue_.push({std::move(task),priority});
                }
                cv_.notify_one();
            }


            std::optional<cortex::Task> pop()
            {
                std::unique_lock<std::mutex> lock(mtx_);
                cv_.wait(lock,[this]{
                    return !queue_.empty() || shutdown_;
                });

                if(queue_.empty()) return std::nullopt;

                PrioritizedTask top_item = std::move(const_cast<PrioritizedTask&>(queue_.top()));
                queue_.pop();
            
                return std::move(top_item.task);
            }

            std::optional<cortex::Task> try_pop()
            {
            std::lock_guard<std::mutex> lock(mtx_);
            if (queue_.empty())
                return std::nullopt;
            auto task = std::move(const_cast<PrioritizedTask&>(queue_.top()).task);
            queue_.pop();
            return task;
            }

            void stop()
            {
                shutdown_=true;
                cv_.notify_all();
            }

            void wake_all()
            {
                cv_.notify_all();
            }

            bool empty() const
            {
                std::lock_guard<std::mutex> lock(mtx_);
                return queue_.empty();
            }

            size_t size() const
            {
                std::lock_guard<std::mutex> lock(mtx_);
                return queue_.size();
            }
    };    
}    