#include "../include/task_queue.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <thread>


TEST(TaskQueue,StartsEmpty)
{
    TaskQueue q;
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(),0);
}

TEST(TaskQueue,SizeAfterPush)
{
    TaskQueue q;
    q.push([]{});
    q.push([]{});
    q.push([]{});
    EXPECT_EQ(q.size(),3);
    EXPECT_FALSE(q.empty());

}
TEST(TaskQueue,TaskRuns)
{
    TaskQueue q;
    int result = 0;
    q.push([&result]
    {
        result = 42;
    });
    auto task = q.pop();
    ASSERT_TRUE(task.has_value());
    (*task)();
    EXPECT_EQ(result,42);
}

TEST(TaskQueueTest, FIFOOrder) {
    TaskQueue q;
    std::vector<int> order;
    q.push([&order]{ order.push_back(1); });
    q.push([&order]{ order.push_back(2); });
    q.push([&order]{ order.push_back(3); });
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
    q.push([]{ });
    EXPECT_EQ(q.size(), 0);
}

TEST(TaskQueueTest, SizeDecreasesAfterPop) {
    TaskQueue q;
    q.push([]{ });
    q.push([]{ });
    q.push([]{ });
    EXPECT_EQ(q.size(), 3);
    q.pop();
    EXPECT_EQ(q.size(), 2);
    q.pop();
    EXPECT_EQ(q.size(), 1);
    q.pop();
    EXPECT_EQ(q.size(), 0);
}

TEST(TaskQueue,ConcurrentPushes)
{
    TaskQueue q;
    std::vector<std::thread> threads;
    for (int i=0;i<10;i++)
    {
        threads.push_back(std::thread([&q]
        {
            q.push([]{});
        }));
    }
    for(auto& t: threads)
    {
        t.join();
    }
    EXPECT_EQ(q.size(),10);
}

TEST(TaskQueueTest, ConcurrentPushPop) {
    TaskQueue q;
    std::atomic<int> count{0};
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    for (int i = 0; i < 5; i++)
        producers.push_back(std::thread([&q]{
            q.push([]{ });
        }));

    for (int i = 0; i < 5; i++)
        consumers.push_back(std::thread([&q, &count]{
            auto task = q.pop();
            if (task) {
                (*task)();
                count++;
            }
        }));

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
        q.push([&count]{ count++; });

    std::this_thread::sleep_for(
        std::chrono::milliseconds(300));

    q.stop();
    worker.join();

    EXPECT_EQ(count, 10);
}

TEST(TaskQueueTest, MultipleWorkersProcessTasks) {
    TaskQueue q;
    std::atomic<int> count{0};
    std::vector<std::thread> workers;

    for (int i = 0; i < 4; i++)
        workers.push_back(std::thread([&q]{
            while (true) {
                auto task = q.pop();
                if (!task) break;
                (*task)();
            }
        }));

    for (int i = 0; i < 20; i++)
        q.push([&count]{ count++; });

    std::this_thread::sleep_for(
        std::chrono::milliseconds(300));

    q.stop();
    for (auto& w : workers) w.join();

    EXPECT_EQ(count, 20);
}

TEST(TaskQueueTest, TaskResultIsCorrect) {
    TaskQueue q;
    int result = 0;

    q.push([&result]{
        for (int i = 1; i <= 100; i++)
            result += i;
    });

    auto task = q.pop();
    ASSERT_TRUE(task.has_value());
    (*task)();

    EXPECT_EQ(result, 5050);
}

TEST(TaskQueueTest, EmptyAfterAllPopped) {
    TaskQueue q;
    q.push([]{ });
    q.push([]{ });

    q.pop();
    q.pop();

    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0);
}
