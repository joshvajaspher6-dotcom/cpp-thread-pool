#include "../include/task_queue.hpp"
#include <mutex>
#include <optional>

void TaskQueue::push(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if(shutdown_) return;
        queue_.push(std::move(task));
    }
    cv_.notify_one();
}
std::optional<std::function<void()>> TaskQueue::pop(const std::atomic<bool>* stop_flag)
{
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock,[this, stop_flag]{
        return !queue_.empty() || shutdown_ || (stop_flag && stop_flag->load());
    });
    if ((stop_flag && stop_flag->load()) || queue_.empty()) {
        return std::nullopt;
    }

    auto task = std::move(queue_.front());
    queue_.pop();

    return task;
}

void TaskQueue::stop()
{
    {
        std::lock_guard<std::mutex>lock(mtx_);
        shutdown_ =true;
    }
    cv_.notify_all();
}

void TaskQueue::wake_all()
{
    cv_.notify_all();
}

bool TaskQueue::empty() const
{
     std::lock_guard<std::mutex>lock(mtx_);
     return queue_.empty();
}

int TaskQueue::size() const
{
     std::lock_guard<std::mutex>lock(mtx_);
     return queue_.size();
}
