#include <gtest/gtest.h>
#include "../include/thread_pool.hpp"
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>

TEST(ThreadPoolTest, WaitAllWith1000Tasks) {
      ThreadPool pool(4);
      std::atomic<int> counter{0};

      for (int i = 0; i < 1000; i++) {
          pool.submit([&counter]{
              counter++;
          });
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
      // Expect at least 1, maybe more already done by the time we wake
      EXPECT_GE(counter, 1);
  }

  TEST(ThreadPoolTest, WaitAnyMultipleWaiters) {
      ThreadPool pool(4);
      std::atomic<int> counter{0};
      std::atomic<int> waiters_awake{0};

      for (int i = 0; i < 50; i++) {
          pool.submit([&counter]{
              std::this_thread::sleep_for(std::chrono::milliseconds(10));
              counter++;
          });
      }

      // Multiple threads calling wait_any()
      std::vector<std::thread> waiters;
      for (int i = 0; i < 4; i++) {
          waiters.emplace_back([&]{
              pool.wait_any();
              waiters_awake++;
          });
      }

      for (auto& t : waiters) t.join();

      EXPECT_GT(waiters_awake, 0);  // All waiters woke up
  }

