#pragma once

#include <exception>
#include <iterator>
#include <vector>
#include <atomic>
#include <memory>
#include <functional>
#include <thread>
#include <future>
#include "priority.hpp"
#include "priority_task_queue.hpp"
#include "priority.hpp"
#include "task_queue.hpp"
#include "worker.hpp"

class ThreadPool {
private:
    TaskQueue                            task_queue_;
    PriorityTaskQueue                    priority_queue_;
    std::vector<std::unique_ptr<Worker>> workers_;
    std::atomic<bool>                    shutdown_{false};
    std::atomic<int>                     active_task_{0};
    std::atomic<int>                     num_threads_;

    void spawn_worker(int id);


public:
    explicit ThreadPool(int num_threads =
        std::thread::hardware_concurrency());
    ~ThreadPool();

    void submit(std::function<void()> task);

    void submit(std::function<void()> task, Priority priority);

    template<typename F>
        auto submit_task(F && func) -> std::future<typename std::result_of<F()>::type>
        {
            using return_type = typename std::result_of<F()>::type;

            auto promise = std::make_shared<std::promise<return_type>>();

            std::future<return_type> future = promise->get_future();

            submit([promise,func = std::forward<F>(func)]()mutable{
                try{
                    promise->set_value(func());
                }
                catch(const std::exception& e)
                {
                    promise->set_exception(std::current_exception());
                }
            });

            return future;
        }

    template<typename F>
    auto submit_task(F&& func, Priority priority)
        -> std::future<typename std::result_of<F()>::type>
    {
        using ReturnType =
            typename std::result_of<F()>::type;

        auto promise = std::make_shared<
            std::promise<ReturnType>>();

        std::future<ReturnType> future =
            promise->get_future();

        submit([promise,
                func = std::forward<F>(func)]() mutable {
            try {
                promise->set_value(func());
            } catch (...) {
                promise->set_exception(
                    std::current_exception());
            }
        }, priority);

        return future;
    }    

    void stop();
    void wait_all();
    void resize(int new_size);

    int active_tasks()  const;
    int pending_tasks() const;
    int thread_count()  const;

    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
};