#include <gtest/gtest.h>
#include "../include/thread_pool.hpp"
#include "../include/task_group.hpp"
#include "../include/cancellation_token.hpp"
#include <atomic>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;
using ThreadPool = cortex::ThreadPool;

TEST(AdvancedTest, CancellationSkipsQueuedTasks) {
    ThreadPool pool(1);
    std::atomic<int> executed{0};
    std::atomic<bool> first_task_started{false};
    auto token = std::make_shared<cortex::CancellationToken>();

    
    pool.submit([&]() {
        first_task_started = true; // ✅ Signal we passed the cancellation check
        std::this_thread::sleep_for(50ms);
        executed++;
    }, token);

    // Wait until Task 1 is actually running
    while (!first_task_started.load()) {
        std::this_thread::sleep_for(1ms);
    }

    
    pool.submit([&]() {
        executed++; 
    }, token);

    
    token->cancel();
    pool.wait_all();

    EXPECT_EQ(executed, 1);
}

TEST(AdvancedTest, TaskGroupWaitBlocksCorrectly) {
    ThreadPool pool(4);
    cortex::TaskGroup group;
    std::atomic<int> counter{0};

    for (int i = 0; i < 10; ++i) {
        group.run(pool, [&counter]() {
            std::this_thread::sleep_for(50ms);
            counter++;
        });
    }

    group.wait();
    EXPECT_EQ(counter, 10);
}

TEST(AdvancedTest, TaskGroupCancelWorks) {
    ThreadPool pool(1); // Force queuing
    cortex::TaskGroup group;
    std::atomic<int> counter{0};

    
    for (int i = 0; i < 5; ++i) {
        group.run(pool, [&counter]() { counter++; });
    }

   
    group.cancel();
    group.wait();

    
    EXPECT_LT(counter.load(), 5);
}

TEST(AdvancedTest, TaskChainingPassesValue) {
    ThreadPool pool(1);
    
    auto future = pool.submit_with_timeout([]() { return 10; });
    auto chained = future.then([](int val) { return val * 2; });

    EXPECT_EQ(chained.get(), 20);
}

TEST(AdvancedTest, ChainingPropagatesExceptions) {
    ThreadPool pool(1);

    auto future = pool.submit_with_timeout([]() { return 10; });
    auto chained = future.then([](int val) {
        throw std::runtime_error("Chain error");
        return val;
    });

    EXPECT_THROW(chained.get(), std::runtime_error);
}