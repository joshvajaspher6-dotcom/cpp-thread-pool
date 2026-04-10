// tests/test_timeout.cpp
#include <gtest/gtest.h>
#include "../include/thread_pool.hpp"
#include "../include/timed_future.hpp"
#include <thread>
#include <chrono>


using namespace std::chrono_literals;

TEST(TimeoutTest, FastTaskCompletes) {
    cortex::ThreadPool pool(2);
    auto future = pool.submit_with_timeout([]() { return 123; });
    
    auto result = future.get_with_timeout(1s);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 123);
}

TEST(TimeoutTest, SlowTaskTimesOut) {
    cortex::ThreadPool pool(2);
    auto future = pool.submit_with_timeout([]() {
        std::this_thread::sleep_for(2s);
        return 456;
    });
    
    auto result = future.get_with_timeout(100ms);
    EXPECT_FALSE(result.has_value());  
    
    
    std::this_thread::sleep_for(2s);
}

TEST(TimeoutTest, IsReadyNonBlocking) {
    cortex::ThreadPool pool(2);
    auto future = pool.submit_with_timeout([]() {
        std::this_thread::sleep_for(200ms);
        return std::string("hello");
    });
    
    EXPECT_FALSE(future.is_ready());
    std::this_thread::sleep_for(300ms);
    EXPECT_TRUE(future.is_ready());
    EXPECT_EQ(future.get(), "hello");
}

TEST(TimeoutTest, ExceptionPropagates) {
    cortex::ThreadPool pool(2);
    auto future = pool.submit_with_timeout([]() -> int {
        throw std::logic_error("test error");
    });
    
    EXPECT_THROW(future.get(), std::logic_error);
    
   
    auto future2 = pool.submit_with_timeout([]() -> int {
        throw std::logic_error("test error 2");
    });
    std::this_thread::sleep_for(100ms);  // Ensure task runs
    EXPECT_THROW(future2.get_with_timeout(1s), std::logic_error);
}

TEST(TimeoutTest, InvalidFutureThrows) {
    cortex::TimedFuture<int> empty;
    EXPECT_FALSE(empty.valid());
    EXPECT_THROW(empty.get(), std::future_error);
}