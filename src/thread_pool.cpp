#include "../include/thread_pool.hpp"
#include "../include/task.hpp"
#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
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
    
}

void cortex::ThreadPool::spawn_worker(int id)
{
   auto w =std::make_unique<Worker>(id,task_queue_,priority_queue_,notify_cv_,notify_mtx_);
   w->start();
   workers_.push_back(std::move(w));
}

void cortex::ThreadPool::stop()
{
    bool expected = false;
    if (!shutdown_.compare_exchange_strong(expected, true)) return;

    task_queue_.stop();
    priority_queue_.stop();
    task_queue_.wake_all();
    priority_queue_.wake_all();

    
    {
        std::lock_guard<std::mutex> lock(notify_mtx_);
        notify_cv_.notify_all();
    }

    
    for (auto& w : workers_) {
        w->request_stop();
    }

    
    {
        std::lock_guard<std::mutex> lock(notify_mtx_);
        notify_cv_.notify_all();
    }

    for (auto& w : workers_) {
        w->join();
    }
}

void cortex::ThreadPool::submit(cortex::Task task)
{
    if (shutdown_) return;
    active_task_.fetch_add(1, std::memory_order_relaxed);
    task_queue_.push(cortex::Task([t = std::move(task), pool = this]() mutable {
        try 
        {
            t();
        } catch (...)
        {
            
            
        }
        pool->active_task_.fetch_sub(1, std::memory_order_relaxed);
        pool->wait_cv_.notify_one();
    }));
    notify_cv_.notify_one();
}


void cortex::ThreadPool::submit(cortex::Task task, Priority priority)
{
    if (shutdown_) return;
    active_task_.fetch_add(1, std::memory_order_relaxed);
     priority_queue_.push(cortex::Task([t = std::move(task), pool = this]() mutable {
        try 
        {
            t();
        } 
        catch (...) 
        {
           
        }
        pool->active_task_.fetch_sub(1, std::memory_order_relaxed);
        pool->wait_cv_.notify_one();
    }), priority);
    notify_cv_.notify_one();
}


void cortex::ThreadPool::submit(cortex::Task task, std::shared_ptr<CancellationToken> token)
{
    if (shutdown_) return;
    if (!token) {
        submit(std::move(task));
        return;
    }

    active_task_.fetch_add(1, std::memory_order_relaxed);

   
    task_queue_.push(cortex::Task([inner = std::move(task), 
                                   tok = std::move(token), 
                                   pool = this]() mutable {
        if (tok->is_cancelled()) {
            pool->active_task_.fetch_sub(1, std::memory_order_relaxed);
            pool->wait_cv_.notify_one();
            return;
        }
        try { inner(); } catch (...) {}
        pool->active_task_.fetch_sub(1, std::memory_order_relaxed);
        pool->wait_cv_.notify_one();
    })); 

    
    notify_cv_.notify_one();
}


void cortex::ThreadPool::submit(std::function<void()> task)
{
    submit(cortex::Task(std::move(task)));
}

void cortex::ThreadPool::submit(std::function<void()> task, Priority priority)
{
    submit(cortex::Task(std::move(task)), priority);
}

void cortex::ThreadPool::submit(std::function<void()> task, std::shared_ptr<CancellationToken> token)
{
    submit(cortex::Task(std::move(task)), std::move(token));
}

void cortex::ThreadPool::wait_all()
{
    std::unique_lock<std::mutex> lock(wait_mutex_);
    wait_cv_.wait(lock, [this] 
        {
        return active_task_.load(std::memory_order_acquire) == 0;
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
         for (int i = 0; i < to_remove && !workers_.empty(); i++)
        {
            auto w = std::move(workers_.back());
            workers_.pop_back();
            w->request_stop();
            workers_to_join.push_back(std::move(w));
        }
        
    
        {
            std::lock_guard<std::mutex> lock(notify_mtx_);
            notify_cv_.notify_all();
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