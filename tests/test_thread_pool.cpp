#include <gtest/gtest.h>
#include "../include/thread_pool.hpp"
#include <mutex>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>



std::mutex mtx;

TEST(ThreadPoolTest, ConcurrentThreadCount)
{
    ThreadPool pool(4);
    EXPECT_EQ(pool.thread_count(),4);
}

TEST(ThreadPoolTest,StartsWithNoActiveTask)
{
    ThreadPool pool(4);
    EXPECT_EQ(pool.active_tasks(), 0);
    EXPECT_EQ(pool.pending_tasks(),0);
}

TEST(ThreadPoolTest, SubmitSingleTask)
{
    ThreadPool pool(4);
    std::atomic<int> result{0};
    pool.submit([&result]
    {
        result = 42;
    });
    pool.wait_all();
    EXPECT_EQ(result, 42);
}

TEST(ThreadPoolTest, SubmitMultipleTask)
{
    ThreadPool pool(4);
    std::atomic<int> count{0};
    for (int i=0;i<10;i++)
    {
        pool.submit([&count]
        {
            count++;
        });
    }
    pool.wait_all();
    EXPECT_EQ(count, 10);
}
TEST(ThreadPoolTest, AllTasksRun) 
{
    ThreadPool pool(4);
    std::atomic<int> count{0};

    for (int i = 0; i < 100; i++)
        pool.submit([&count]
        {
            count++;
        });

    pool.wait_all();
    EXPECT_EQ(count, 100);
}

TEST(ThreadPoolTest, WaitAllBlocksUntilDone) 
{
    ThreadPool pool(4);
    std::atomic<bool> all_done{false};
    std::atomic<int> count{0};

    for (int i = 0; i < 10; i++)
        pool.submit([&count]
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(10));
            count++;
        });

    pool.wait_all();
    all_done = true;

    EXPECT_TRUE(all_done);
    EXPECT_EQ(count, 10);
}

TEST(ThreadPoolTest, ActiveTasksCount) 
{
    ThreadPool pool(4);
    std::atomic<int> count{0};

    for (int i = 0; i < 4; i++)
        pool.submit([&count]
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(100));
            count++;
        });

    pool.wait_all();
    EXPECT_EQ(pool.active_tasks(), 0);
    EXPECT_EQ(count, 4);
}

TEST(ThreadPoolTest, PendingTasksCount) 
{
    ThreadPool pool(4);

    for (int i = 0; i < 4; i++)
        pool.submit([]
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(200));
        });

    pool.wait_all();
    EXPECT_EQ(pool.pending_tasks(), 0);
}

TEST(ThreadPoolTest, ConcurrentTasks) 
{
    ThreadPool pool(8);
    std::atomic<int> count{0};
    std::mutex mtx;
    std::vector<int> results;

    for (int i = 0; i < 50; i++)
        pool.submit([&count, &mtx, &results, i]
        {
            count++;
            std::lock_guard<std::mutex> lock(mtx);
            results.push_back(i);
        });

    pool.wait_all();
    EXPECT_EQ(count, 50);
    EXPECT_EQ(results.size(), 50);
}

TEST(ThreadPoolTest, TasksRunConcurrently) 
{
    ThreadPool pool(4);
    std::atomic<int> concurrent{0};
    std::atomic<int> max_concurrent{0};

    for (int i = 0; i < 4; i++)
        pool.submit([&concurrent, &max_concurrent]
        {
            concurrent++;
            int current = concurrent.load();
            int expected = max_concurrent.load();
            while (current > expected &&
                   !max_concurrent.compare_exchange_weak(
                       expected, current));
            std::this_thread::sleep_for(
                std::chrono::milliseconds(50));
            concurrent--;
        });

    pool.wait_all();
    EXPECT_GT(max_concurrent, 1);
}

TEST(ThreadPoolTest, ShutdownRejectsNewTasks) 
{
    ThreadPool pool(4);
    pool.stop();

    std::atomic<int> count{0};
    pool.submit([&count]
        { 
            count++; 
        });

    std::this_thread::sleep_for(
        std::chrono::milliseconds(100));

    EXPECT_EQ(count, 0);
}

TEST(ThreadPoolTest, ResizeAddsWorkers) 
{
    ThreadPool pool(4);
    EXPECT_EQ(pool.thread_count(), 4);

    pool.resize(8);
    EXPECT_EQ(pool.thread_count(), 8);
}

TEST(ThreadPoolTest, ResizeHandlesMoreTasks) 
{
    ThreadPool pool(2);
    pool.resize(8);

    std::atomic<int> count{0};
    for (int i = 0; i < 50; i++)
        pool.submit([&count]
        {
             count++; 
        });

    pool.wait_all();
    EXPECT_EQ(count, 50);
}

TEST(ThreadPoolTest, HardwareConcurrency) 
{
    ThreadPool pool;
    EXPECT_GT(pool.thread_count(), 0);
    EXPECT_EQ(pool.thread_count(),
        (int)std::thread::hardware_concurrency());
}