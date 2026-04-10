#include <gtest/gtest.h>
#include "../include/thread_pool.hpp"
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// ✅ ADD THIS LINE: Allows you to use 'ThreadPool' instead of 'cortex::ThreadPool'
using ThreadPool = cortex::ThreadPool;

TEST(ThreadPoolTest, WaitAllWith1000Tasks) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};

    for (int i = 0; i < 1000; i++) {
        pool.submit([&counter]{ counter++; });
    }

    pool.wait_all();
    EXPECT_EQ(counter, 1000);
    EXPECT_EQ(pool.active_tasks(), 0);
    EXPECT_EQ(pool.pending_tasks(), 0);
}

TEST(ThreadPoolTest, WaitAnyReturnsWhenOneDone) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};
    std::atomic<bool> first_done{false};

    for (int i = 0; i < 100; i++) {
        pool.submit([&counter, &first_done, i]{
            counter++;
            if (i == 0) first_done = true;
        });
    }

    pool.wait_any();
    EXPECT_TRUE(first_done.load());
    EXPECT_GE(counter, 1);
}

TEST(ThreadPoolTest, WaitAnyMultipleWaiters) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};
    std::atomic<int> waiters_awake{0};

    for (int i = 0; i < 50; i++) {
        pool.submit([&counter]{
            std::this_thread::sleep_for(10ms);
            counter++;
        });
    }

    std::vector<std::thread> waiters;
    for (int i = 0; i < 4; i++) {
        waiters.emplace_back([&]{
            pool.wait_any();
            waiters_awake++;
        });
    }

    for (auto& t : waiters) t.join();
    EXPECT_GT(waiters_awake, 0);
}

TEST(ThreadPoolTest, ResizeGrow) {
    ThreadPool pool(2);
    EXPECT_EQ(pool.thread_count(), 2);

    pool.resize(6);
    EXPECT_EQ(pool.thread_count(), 6);

    std::atomic<int> counter{0};
    for (int i = 0; i < 100; i++) {
        pool.submit([&counter] { counter++; });
    }

    pool.wait_all();
    EXPECT_EQ(counter, 100);
}

TEST(ThreadPoolTest, ResizeShrinkIdle) {
    ThreadPool pool(8);
    EXPECT_EQ(pool.thread_count(), 8);

    std::this_thread::sleep_for(100ms);

    pool.resize(2);
    EXPECT_EQ(pool.thread_count(), 2);

    std::atomic<int> counter{0};
    for (int i = 0; i < 50; i++) {
        pool.submit([&counter] { counter++; });
    }

    pool.wait_all();
    EXPECT_EQ(counter, 50);
}

TEST(ThreadPoolTest, ResizeShrinkDuringExecution) {
    ThreadPool pool(8);
    EXPECT_EQ(pool.thread_count(), 8);

    std::atomic<int> counter{0};
    std::atomic<bool> started{false};

    for (int i = 0; i < 500; i++) {
        pool.submit([&counter, &started] {
            started = true;
            std::this_thread::sleep_for(10ms);
            counter++;
        });
    }

    while (!started.load()) {
        std::this_thread::sleep_for(10ms);
    }

    pool.resize(2);
    EXPECT_EQ(pool.thread_count(), 2);

    pool.wait_all();
    EXPECT_EQ(counter, 500);
    EXPECT_EQ(pool.active_tasks(), 0);
}

TEST(ThreadPoolTest, ResizeMultipleTimes) {
    ThreadPool pool(4);
    EXPECT_EQ(pool.thread_count(), 4);

    pool.resize(8);
    EXPECT_EQ(pool.thread_count(), 8);

    pool.resize(2);
    EXPECT_EQ(pool.thread_count(), 2);

    pool.resize(6);
    EXPECT_EQ(pool.thread_count(), 6);

    pool.resize(6);

    std::atomic<int> counter{0};
    for (int i = 0; i < 200; i++) {
        pool.submit([&counter] { counter++; });
    }

    pool.wait_all();
    EXPECT_EQ(counter, 200);
}

TEST(ThreadPoolTest, ResizeShrinkToZero) {
    ThreadPool pool(4);
    EXPECT_EQ(pool.thread_count(), 4);

    pool.resize(0);
    EXPECT_EQ(pool.thread_count(), 0);
    EXPECT_EQ(pool.active_tasks(), 0);
    EXPECT_EQ(pool.pending_tasks(), 0);
    
    std::atomic<int> counter{0};
    pool.submit([&counter] { counter++; });
    
    pool.resize(2);
    pool.wait_all();
    EXPECT_EQ(counter, 1);
}