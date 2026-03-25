#pragma once

#include <iterator>
#include <vector>
#include <atomic>
#include <memory>
#include <functional>
#include <thread>
#include <future>
#include "task_queue.hpp"
#include "worker.hpp"

class ThreadPool {
private:
    TaskQueue                            task_queue_;
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

    template<typename F>
        auto submit_task(F && func) -> std::future<typename std::result_of<F()>::type>
        {
            using return_type = typename std::result_of<F()>::type;

            auto promise = std::make_shared<std::promise<return_type>>();

            std::future<return_type> future = promise->get_future();

            submit([promise,func = std::forward<F>(func)](){
                    promise->set_value(func());
            });

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