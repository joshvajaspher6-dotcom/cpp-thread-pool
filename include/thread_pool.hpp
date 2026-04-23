#pragma once

#include <condition_variable>
#include <exception>
#include <iterator>
#include <vector>
#include <atomic>
#include <memory>
#include <functional>
#include <thread>
#include <future>
#include <stdexcept> 
#include "priority.hpp"
#include "priority_task_queue.hpp"
#include "task_queue.hpp"
#include "worker.hpp"
#include "timed_future.hpp"
#include "cancellation_token.hpp"
#include "task.hpp"

namespace cortex
{
    class ThreadPool {
    private:
        TaskQueue                            task_queue_;
        PriorityTaskQueue                    priority_queue_;
        std::vector<std::unique_ptr<Worker>> workers_;
        alignas(64) std::atomic<bool>                    shutdown_{false};
        alignas(64) std::atomic<int>                     active_task_{0};
        std::atomic<int>                     num_threads_;
        std::mutex                           wait_mutex_;
        std::condition_variable              wait_cv_;
        std::condition_variable notify_cv_;
        std::mutex notify_mtx_;

        void spawn_worker(int id);

    public:
        explicit ThreadPool(int num_threads =
            std::thread::hardware_concurrency());
        ~ThreadPool();

        void submit(std::function<void()> task);
        void submit(std::function<void()> task, Priority priority);
        void submit(std::function<void()> task, std::shared_ptr<CancellationToken> token);

        void submit(cortex::Task task);
        void submit(cortex::Task task, Priority priority);
        void submit(cortex::Task task, std::shared_ptr<CancellationToken> token);

        
        template<typename F>
        auto submit_task(F && func) -> std::future<typename std::result_of<F()>::type>
        {
            using return_type = typename std::result_of<F()>::type;
            auto promise = std::make_shared<std::promise<return_type>>();
            std::future<return_type> future = promise->get_future();

            submit([promise, func = std::forward<F>(func)]() mutable {
                try {
                    promise->set_value(func());
                } catch (const std::exception& ) {
                    promise->set_exception(std::current_exception());
                }
            });
            return future;
        }

           template<typename F, typename... Args>
        auto submit_with_timeout(F&& f, Args&&... args)
            -> TimedFuture<
                typename std::invoke_result<
                    typename std::decay<F>::type,
                    typename std::decay<Args>::type...
                >::type>
        {
            using ReturnType = typename std::invoke_result<
                typename std::decay<F>::type,
                typename std::decay<Args>::type...
            >::type;

            auto task = std::make_shared<std::packaged_task<ReturnType()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );

            std::future<ReturnType> future = task->get_future();

            
            auto wrapped = [this, task]() {
                try 
                {
                    (*task)(); 
                } catch (...) 
                {
                    
                }
                
                active_task_.fetch_sub(1, std::memory_order_release);
                wait_cv_.notify_all();
            };

            active_task_.fetch_add(1, std::memory_order_relaxed);

            if (!shutdown_) {
                task_queue_.push(cortex::Task(std::move(wrapped)));
                notify_cv_.notify_one(); 
            } else {
                active_task_.fetch_sub(1, std::memory_order_relaxed);
                wait_cv_.notify_all();
                throw std::runtime_error("cannot submit :: threadpool is shutting down");
            }

            return TimedFuture<ReturnType>(std::move(future));
        }

        
        void stop();
        void wait_all();
        void wait_any();
        void resize(int new_size);

        int active_tasks()  const;
        int pending_tasks() const;
        int thread_count()  const;

        ThreadPool(const ThreadPool&)            = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
    };
}