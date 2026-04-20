#pragma once

#include <queue>
#include <functional>
#include <optional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "task.hpp"

namespace cortex
{

    class TaskQueue
    {
        private:
            std::queue<cortex::Task> queue_;
            mutable std::mutex mtx_;
            std::condition_variable cv_;
            std::atomic<bool> shutdown_{false};
            

        public:
            void push(cortex::Task task);
            std::optional<cortex::Task> pop(const std::atomic<bool>* stop_flag = nullptr);
            std::optional<cortex::Task> try_pop();
            void stop();
            void wake_all();
            bool empty() const;
            int size() const;
    };
}