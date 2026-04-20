#include "../include/task_queue.hpp"
#include "../include/task.hpp"
#include <mutex>
#include <optional>

void cortex::TaskQueue::push(cortex::Task task)
{
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if(shutdown_) return;
        queue_.push(std::move(task));
    }
    cv_.notify_one();
}
std::optional<cortex::Task> cortex::TaskQueue::pop(const std::atomic<bool>* stop_flag)
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

std::optional<cortex::Task> cortex::TaskQueue::try_pop() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (queue_.empty()) return std::nullopt;
    auto task = std::move(queue_.front());
    queue_.pop();
    return task;
}
void cortex::TaskQueue::stop()
{
    {
        std::lock_guard<std::mutex>lock(mtx_);
        shutdown_ =true;
    }
    cv_.notify_all();
}

void cortex::TaskQueue::wake_all()
{
    cv_.notify_all();
}

bool cortex::TaskQueue::empty() const
{
     std::lock_guard<std::mutex>lock(mtx_);
     return queue_.empty();
}

int cortex::TaskQueue::size() const
{
     std::lock_guard<std::mutex>lock(mtx_);
     return queue_.size();
}
