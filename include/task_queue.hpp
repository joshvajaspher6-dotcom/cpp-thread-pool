#pragma once

#include <queue>
#include <functional>
#include <optional>
#include <mutex>
#include <condition_variable>
#include <atomic>

class TaskQueue
{
    private:
        std::queue<std::function<void()>> queue_;
        mutable std::mutex mtx_;
        std::condition_variable cv_;
        std::atomic<bool> shutdown_{false};

    public:
        void push(std::function<void()> task);
        std::optional<std::function<void()>> pop();
        void stop();
        bool empty() const;
        int size() const;
};