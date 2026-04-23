#include <gtest/gtest.h>
#include <vector>
#include "../include/task.hpp"
#include "../include/thread_pool.hpp"

TEST(ThreadPoolTest, PriorityStrictOrdering) {
    cortex::ThreadPool pool(1);
    std::vector<int> order;
    std::mutex mtx;

    // pause the worker so ALL tasks are queued before any runs
    std::atomic<bool> gate{false};
    pool.submit([&]() {
        while (!gate.load()) std::this_thread::yield();
    }, cortex::Priority::HIGH);  // runs first, blocks the thread

    auto push = [&](int id, cortex::Priority p) {
        pool.submit([&, id]() {
            std::lock_guard<std::mutex> l(mtx);
            order.push_back(id);
        }, p);
    };

    push(1, cortex::Priority::LOW);
    push(2, cortex::Priority::MEDIUM);
    push(3, cortex::Priority::HIGH);

    gate.store(true);        // release the gate
    pool.wait_all();         // wait for everything to finish

    ASSERT_EQ(order.size(), 3);
    EXPECT_EQ(order[0], 3);  // HIGH
    EXPECT_EQ(order[1], 2);  // MEDIUM
    EXPECT_EQ(order[2], 1);  // LOW
}