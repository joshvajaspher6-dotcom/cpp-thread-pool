
#include <gtest/gtest.h>
#include <vector>

#include "../include/thread_pool.hpp"

TEST(ThreadPoolTest, DISABLED_PriorityStrictOrdering) {
    cortex::ThreadPool pool(1);
    std::vector<int> order;
    std::mutex mtx;

    auto push = [&](int id, cortex::Priority p) {
        pool.submit([&, id]() {
            std::lock_guard<std::mutex> l(mtx);
            order.push_back(id);
        }, p);
    };

    push(1, cortex::Priority::LOW);
    push(2, cortex::Priority::MEDIUM);
    push(3, cortex::Priority::HIGH);

    // Uses your proven, deadlock-free wait_all()
    pool.wait_all();

    ASSERT_EQ(order.size(), 3);
    EXPECT_EQ(order[0], 3); // HIGH runs first
    EXPECT_EQ(order[1], 2); // MEDIUM second
    EXPECT_EQ(order[2], 1); // LOW last
}
