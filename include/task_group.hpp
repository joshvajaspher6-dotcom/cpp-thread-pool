#pragma once 

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <memory>
#include "thread_pool.hpp"
#include "cancellation_token.hpp"

namespace cortex
{
    class TaskGroup
    {
        private:
            mutable std::atomic<int> active_count_{0};
            mutable std::condition_variable completion_cv_;
            mutable std::mutex completion_mtx_;
            std::shared_ptr<CancellationToken> cancel_token_ = std::make_shared<CancellationToken>();

            void notify_completion() const
            {
                active_count_.fetch_sub(1,std::memory_order_release);
                 std::unique_lock<std::mutex> lock(completion_mtx_);
                completion_cv_.notify_all();
            }
        
        public:
            TaskGroup() = default;
            ~TaskGroup() = default;
            
            template<typename F>
            void run(ThreadPool& pool,F&& task)
            {
                active_count_.fetch_add(1,std::memory_order_relaxed);
                pool.submit([this,task=std::forward<F>(task),token = cancel_token_]()mutable
                {
                    if(token->is_cancelled())
                    {
                        notify_completion();
                        return;
                    }
                    try {
                    {
                        task();
                    }
                    } catch (...) {

                    }
                    notify_completion();
                });
            }

            void wait() const
            {
                std::unique_lock<std::mutex> lock(completion_mtx_);
                completion_cv_.wait(lock,[this]
                {
                    return active_count_.load(std::memory_order_acquire)==0;
                });

            }
            void cancel()
            {
                cancel_token_->cancel();
            }

    };
}