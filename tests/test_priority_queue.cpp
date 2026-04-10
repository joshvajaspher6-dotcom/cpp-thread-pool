
#include <gtest/gtest.h>
#include <vector>

#include "../include/thread_pool.hpp"


TEST(ThreadPoolTest, PriorityStrictOrdering) {
      cortex::ThreadPool pool(4);
      std::vector<cortex::Priority> execution_order;
      std::mutex order_mtx;

     
      
      pool.submit([&]() {
          std::lock_guard<std::mutex> lock(order_mtx);
          execution_order.push_back(cortex::Priority::CRITICAL);
      }, cortex::Priority::CRITICAL);

      pool.submit([&]() {
          std::lock_guard<std::mutex> lock(order_mtx);
          execution_order.push_back(cortex::Priority::CRITICAL);
      }, cortex::Priority::CRITICAL);

      // 1 HIGH task (submitted BEFORE CRITICAL tasks finish)
      pool.submit([&]() {
          std::lock_guard<std::mutex> lock(order_mtx);
          execution_order.push_back(cortex::Priority::HIGH);
      }, cortex::Priority::HIGH);

      // 2 MEDIUM tasks
      pool.submit([&]() {
          std::lock_guard<std::mutex> lock(order_mtx);
          execution_order.push_back(cortex::Priority::MEDIUM);
      }, cortex::Priority::MEDIUM);

      pool.submit([&]() {
          std::lock_guard<std::mutex> lock(order_mtx);
          execution_order.push_back(cortex::Priority::MEDIUM);
      }, cortex::Priority::MEDIUM);

      // 2 LOW tasks
      pool.submit([&]() {
          std::lock_guard<std::mutex> lock(order_mtx);
          execution_order.push_back(cortex::Priority::LOW);
      }, cortex::Priority::LOW);

      pool.submit([&]() {
          std::lock_guard<std::mutex> lock(order_mtx);
          execution_order.push_back(cortex::Priority::LOW);
      }, cortex::Priority::LOW);

      pool.wait_all();

      
      ASSERT_EQ(execution_order.size(), 7);

      
      EXPECT_EQ(execution_order[0], cortex::Priority::CRITICAL);
      EXPECT_EQ(execution_order[1], cortex::Priority::CRITICAL);

    
      EXPECT_EQ(execution_order[2], cortex::Priority::HIGH);

     
      EXPECT_EQ(execution_order[3], cortex::Priority::MEDIUM);
      EXPECT_EQ(execution_order[4], cortex::Priority::MEDIUM);

      EXPECT_EQ(execution_order[5], cortex::Priority::LOW);
      EXPECT_EQ(execution_order[6], cortex::Priority::LOW);
  }