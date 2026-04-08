#include "../include/thread_pool.hpp"
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include "../include/priority.hpp"
#include "../include/priority_task_queue.hpp"


ThreadPool::ThreadPool(int num_threads):num_threads_(num_threads)
{
    for (int i=0;i<num_threads;i++)
    {
        spawn_worker(i);
    }
}

ThreadPool::~ThreadPool()
{
    stop();
}

void ThreadPool::spawn_worker(int id)
{
    workers_.push_back(
        std::make_unique<Worker>(id,task_queue_,priority_queue_)
    );

}


void ThreadPool::stop()
{
    shutdown_ = true;
    task_queue_.stop();
    priority_queue_.stop();
}

void ThreadPool::submit(std::function<void()> task)
{
    if(shutdown_) return;
    active_task_.fetch_add(1,std::memory_order_relaxed);
    task_queue_.push([this,task = std::move(task)]{
        task();
        active_task_.fetch_sub(1,std::memory_order_relaxed);
        wait_cv_.notify_all();
    });
}

void ThreadPool::submit(std::function<void()> task,
                        Priority priority)
{
    if (shutdown_) return;
    active_task_.fetch_add(1,std::memory_order_relaxed);
    priority_queue_.push(
        [this, task = std::move(task)]{
            task();
            active_task_.fetch_sub(1,std::memory_order_relaxed);
            wait_cv_.notify_all();
        },priority);
}

void ThreadPool::wait_all()
{
    while (active_task_ > 0 || !task_queue_.empty()|| !priority_queue_.empty())
    {
        std::unique_lock<std::mutex> lock(wait_mutex_);
        wait_cv_.wait(lock,[this]
        {
            return active_task_==0;
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void ThreadPool::wait_any()
{
    std::unique_lock<std::mutex> lock(wait_mutex_);
    if (active_task_ == 0)
        return;
    
    int initial_count = active_task_;
    wait_cv_.wait(lock,[this,initial_count]
    {
        return active_task_ < initial_count;
    });
}

void ThreadPool::resize(int new_size)
{
    if (new_size > num_threads_)
    {
        while (num_threads_ < new_size)
            spawn_worker(num_threads_++);
    }
    else if (new_size < num_threads_)
    {
        int to_remove =num_threads_ - new_size;

        for (int i=0;i< to_remove;i++)
        {
            if(!workers_.empty())
            {
                workers_.back()->request_stop();
                
            }
        }
        for (int i = 0; i < to_remove; i++) 
        {
            if (!workers_.empty()) 
            {
                auto& worker = workers_.back();
                worker->request_stop();
                task_queue_.wake_all();
                priority_queue_.wake_all();
                worker->join();  
                workers_.pop_back();
            }
  }

        num_threads_ = new_size;

       

    }
}

int ThreadPool::active_tasks() const
{
    return active_task_;
}

int ThreadPool::pending_tasks() const
{
    return task_queue_.size()+ priority_queue_.size();
}

int ThreadPool::thread_count() const
{
    return num_threads_;
}