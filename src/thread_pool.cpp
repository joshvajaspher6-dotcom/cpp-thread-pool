#include "../include/thread_pool.hpp"
#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include "../include/priority.hpp"
#include "../include/priority_task_queue.hpp"

cortex::ThreadPool::ThreadPool(int num_threads) : num_threads_(num_threads)
{
    for (int i = 0; i < num_threads; i++)
    {
        spawn_worker(i);
    }
}

cortex::ThreadPool::~ThreadPool()
{
    stop();
    for (auto& w : workers_) {
        w->join();
    }
}

void cortex::ThreadPool::spawn_worker(int id)
{
    workers_.push_back(
        std::make_unique<Worker>(id, task_queue_, priority_queue_)
    );
}

void cortex::ThreadPool::stop()
{
    bool expected = false;
    if (!shutdown_.compare_exchange_strong(expected, true)) return;

    task_queue_.stop();
    priority_queue_.stop();
    
    task_queue_.wake_all();
    priority_queue_.wake_all();

    for (auto& w : workers_) {
        w->join();
    }
}

void cortex::ThreadPool::submit(std::function<void()> task)
{
    if (shutdown_) return;
    active_task_.fetch_add(1, std::memory_order_relaxed);
    task_queue_.push([this, task = std::move(task)] {
        task();
        active_task_.fetch_sub(1, std::memory_order_relaxed);
        wait_cv_.notify_all();
    });
}

void cortex::ThreadPool::submit(std::function<void()> task,
                                Priority priority)
{
    if (shutdown_) return;
    active_task_.fetch_add(1, std::memory_order_relaxed);
    priority_queue_.push(
        [this, task = std::move(task)] {
            task();
            active_task_.fetch_sub(1, std::memory_order_relaxed);
            wait_cv_.notify_all();
        }, priority);
}

void cortex::ThreadPool::wait_all()
{
    std::unique_lock<std::mutex> lock(wait_mutex_);
    wait_cv_.wait(lock, [this] {
        return active_task_.load(std::memory_order_acquire) == 0 &&
               task_queue_.empty() && 
               priority_queue_.empty();
    });
}

void cortex::ThreadPool::wait_any()
{
    std::unique_lock<std::mutex> lock(wait_mutex_);
    if (active_task_ == 0)
        return;
    
    int initial_count = active_task_;
    wait_cv_.wait(lock, [this, initial_count] {
        return active_task_.load(std::memory_order_acquire) < initial_count;
    });
}

void cortex::ThreadPool::resize(int new_size)
{
    if (new_size > num_threads_)
    {
        while (num_threads_ < new_size)
        {
            spawn_worker(num_threads_);
            num_threads_++;
        }
    }
    else if (new_size < num_threads_)
    {
        int to_remove = num_threads_ - new_size;
        
        std::vector<std::unique_ptr<Worker>> workers_to_join;
        for (int i = 0; i < to_remove; i++)
        {
            if (!workers_.empty())
            {
                auto w = std::move(workers_.back());
                workers_.pop_back();
                w->request_stop();
                workers_to_join.push_back(std::move(w));
            }
        }
        
        task_queue_.wake_all();
        priority_queue_.wake_all();

        for (auto& w : workers_to_join)
        {
            w->join();
        }

        num_threads_ = new_size;
    }
}

void cortex::ThreadPool::submit(std::function<void()> task,
            std::shared_ptr<CancellationToken> token)
{
    if(shutdown_)
        return;

    if(!token)
    {
        submit(std::move(task));
    }

    active_task_.fetch_add(1,std::memory_order_relaxed);

    task_queue_.push([this,task=std::move(task),token=std::move(token)]
    {
        if(token->is_cancelled())
        {
            active_task_.fetch_sub(1,std::memory_order_relaxed);
            wait_cv_.notify_all();
            return;
        }
    

    try{
        task();
    }
    catch (...)
    {}

    active_task_.fetch_sub(1,std::memory_order_relaxed);
    wait_cv_.notify_all();

    });
}            



int cortex::ThreadPool::active_tasks() const
{
    return active_task_;
}

int cortex::ThreadPool::pending_tasks() const
{
    return task_queue_.size() + priority_queue_.size();
}

int cortex::ThreadPool::thread_count() const
{
    return num_threads_;
}