#include "../include/worker.hpp"



cortex::Worker::Worker(int id, TaskQueue& queue, PriorityTaskQueue& priority_queue,
                       std::condition_variable& notify_cv, std::mutex& notify_mtx)
    : queue_(queue), priority_queue_(priority_queue), id_(id), 
      notify_cv_(notify_cv), notify_mtx_(notify_mtx), thread_(&Worker::run, this) {}

cortex::Worker::~Worker()
{
    if(thread_.joinable())
    {
        thread_.join();
    }
}



void cortex::Worker::run() {
    std::unique_lock<std::mutex> lock(notify_mtx_, std::defer_lock);
    
    while (!should_stop_.load()) {
        auto task = priority_queue_.try_pop();
        if (!task) task = queue_.try_pop();
        
        if (!task) {
            lock.lock();
            notify_cv_.wait(lock, [this] {
                return should_stop_.load() || !priority_queue_.empty() || !queue_.empty();
            });
            lock.unlock();
            
            if (should_stop_.load()) break;
            task = priority_queue_.try_pop();
            if (!task) task = queue_.try_pop();
        }
        
        if (task) {
            busy_ = true;
            (*task)();
            busy_ = false;
        }
    }
}

bool cortex::Worker::is_busy() const
{
    return busy_;
}

int cortex::Worker::id() const
{
    return id_;
}