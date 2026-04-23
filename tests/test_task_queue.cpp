#include "../include/task_queue.hpp"
#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include "../include/task.hpp"
using namespace std::chrono_literals;


using TaskQueue = cortex::TaskQueue;

TEST(TaskQueue, StartsEmpty) {
    TaskQueue q;
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0);
}

TEST(TaskQueue, SizeAfterPush) {
    TaskQueue q;
    q.push(cortex::Task([]{}));
    q.push(cortex::Task([]{}));
    q.push(cortex::Task([]{}));
    EXPECT_EQ(q.size(), 3);
    EXPECT_FALSE(q.empty());
}

TEST(TaskQueue, TaskRuns) {
    TaskQueue q;
    int result = 0;
    q.push(cortex::Task([&result] { result = 42; }));
    
    auto task = q.pop();
    ASSERT_TRUE(task.has_value());
    (*task)();
    EXPECT_EQ(result, 42);
}

TEST(TaskQueueTest, FIFOOrder) {
    TaskQueue q;
    std::vector<int> order;
    q.push(cortex::Task([&order]{ order.push_back(1); }));
    q.push(cortex::Task([&order]{ order.push_back(2); }));
    q.push(cortex::Task([&order]{ order.push_back(3); }));
    
    for (int i = 0; i < 3; i++) {
        auto task = q.pop();
        if (task) (*task)();
    }
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

TEST(TaskQueueTest, StopReturnsNullopt) {
    TaskQueue q;
    q.stop();
    auto task = q.pop();
    EXPECT_FALSE(task.has_value());
}

TEST(TaskQueueTest, PushAfterStop) {
    TaskQueue q;
    q.stop();
    q.push(cortex::Task([]{ }));
    EXPECT_EQ(q.size(), 0);
}

TEST(TaskQueueTest, SizeDecreasesAfterPop) {
    TaskQueue q;
    q.push(cortex::Task([]{ }));
    q.push(cortex::Task([]{ }));
    q.push(cortex::Task([]{ }));
    EXPECT_EQ(q.size(), 3);
    
    q.pop(); EXPECT_EQ(q.size(), 2);
    q.pop(); EXPECT_EQ(q.size(), 1);
    q.pop(); EXPECT_EQ(q.size(), 0);
}

TEST(TaskQueue, ConcurrentPushes) {
    TaskQueue q;
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back(cortex::Task([&q] { q.push(cortex::Task([]{})); }));
    }
    for (auto& t : threads) {
        t.join();
    }
    EXPECT_EQ(q.size(), 10);
}

TEST(TaskQueueTest, ConcurrentPushPop) {
    TaskQueue q;
    std::atomic<int> count{0};
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    for (int i = 0; i < 5; i++)
        producers.emplace_back(cortex::Task([&q] { q.push(cortex::Task([]{})); }));

    for (int i = 0; i < 5; i++)
        consumers.emplace_back([&q, &count]{
            auto task = q.pop();
            if (task) {
                (*task)();
                count++;
            }
        });

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();
    EXPECT_EQ(count, 5);
}

TEST(TaskQueueTest, WorkerProcessesTasks) {
    TaskQueue q;
    std::atomic<int> count{0};

    std::thread worker([&q]{
        while (true) {
            auto task = q.pop();
            if (!task) break;
            (*task)();
        }
    });

    for (int i = 0; i < 10; i++)
        q.push(cortex::Task([&count]{ count++; }));

    std::this_thread::sleep_for(300ms);
    q.stop();
    worker.join();

    EXPECT_EQ(count, 10);
}

TEST(TaskQueueTest, MultipleWorkersProcessTasks) {
    TaskQueue q;
    std::atomic<int> count{0};
    std::vector<std::thread> workers;

    for (int i = 0; i < 4; i++)
        workers.emplace_back([&q]{
            while (true) {
                auto task = q.pop();
                if (!task) break;
                (*task)();
            }
        });

    for (int i = 0; i < 20; i++)
        q.push(cortex::Task([&count]{ count++; }));

    std::this_thread::sleep_for(300ms);
    q.stop();
    for (auto& w : workers) w.join();

    EXPECT_EQ(count, 20);
}

TEST(TaskQueueTest, TaskResultIsCorrect) {
    TaskQueue q;
    int result = 0;

    q.push(cortex::Task([&result]{
        for (int i = 1; i <= 100; i++)
            result += i;
    }));

    auto task = q.pop();
    ASSERT_TRUE(task.has_value());
    (*task)();

    EXPECT_EQ(result, 5050);
}

TEST(TaskQueueTest, EmptyAfterAllPopped) {
    TaskQueue q;
    q.push(cortex::Task([]{ }));
    q.push(cortex::Task([]{ }));

    q.pop();
    q.pop();

    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0);
}