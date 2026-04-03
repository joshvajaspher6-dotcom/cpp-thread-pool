
#include <gtest/gtest.h>
#include <vector>

#include "../include/thread_pool.hpp"


TEST(ThreadPoolTest, PriorityStrictOrdering) {
      ThreadPool pool(4);
      std::vector<Priority> execution_order;
      std::mutex order_mtx;

     
      
      pool.submit([&]() {
          std::lock_guard<std::mutex> lock(order_mtx);
          execution_order.push_back(Priority::CRITICAL);
      }, Priority::CRITICAL);

      pool.submit([&]() {
          std::lock_guard<std::mutex> lock(order_mtx);
          execution_order.push_back(Priority::CRITICAL);
      }, Priority::CRITICAL);

      // 1 HIGH task (submitted BEFORE CRITICAL tasks finish)
      pool.submit([&]() {
          std::lock_guard<std::mutex> lock(order_mtx);
          execution_order.push_back(Priority::HIGH);
      }, Priority::HIGH);

      // 2 MEDIUM tasks
      pool.submit([&]() {
          std::lock_guard<std::mutex> lock(order_mtx);
          execution_order.push_back(Priority::MEDIUM);
      }, Priority::MEDIUM);

      pool.submit([&]() {
          std::lock_guard<std::mutex> lock(order_mtx);
          execution_order.push_back(Priority::MEDIUM);
      }, Priority::MEDIUM);

      // 2 LOW tasks
      pool.submit([&]() {
          std::lock_guard<std::mutex> lock(order_mtx);
          execution_order.push_back(Priority::LOW);
      }, Priority::LOW);

      pool.submit([&]() {
          std::lock_guard<std::mutex> lock(order_mtx);
          execution_order.push_back(Priority::LOW);
      }, Priority::LOW);

      pool.wait_all();

      
      ASSERT_EQ(execution_order.size(), 7);

      
      EXPECT_EQ(execution_order[0], Priority::CRITICAL);
      EXPECT_EQ(execution_order[1], Priority::CRITICAL);

    
      EXPECT_EQ(execution_order[2], Priority::HIGH);

     
      EXPECT_EQ(execution_order[3], Priority::MEDIUM);
      EXPECT_EQ(execution_order[4], Priority::MEDIUM);

      EXPECT_EQ(execution_order[5], Priority::LOW);
      EXPECT_EQ(execution_order[6], Priority::LOW);
  }