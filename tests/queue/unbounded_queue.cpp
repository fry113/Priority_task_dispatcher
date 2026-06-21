#include <gtest/gtest.h>
#include <thread>

#include "queue/unbounded_queue.hpp"

using namespace dispatcher::queue;

TEST(UnboundedQueueTest, ConstructorCheck) { EXPECT_NO_THROW(UnboundedQueue()); }

TEST(UnboundedQueueTest, TryPopEmptyCheck) {
    UnboundedQueue queue;
    auto check = queue.try_pop();

    EXPECT_FALSE(check.has_value());
}

TEST(UnboundedQueueTest, PushAndPopCheck) {
    UnboundedQueue queue;
    std::atomic<int> value{0};
    auto task = [&]() { value = 10; };

    queue.push(task);
    auto res = queue.try_pop();
    ASSERT_TRUE(res.has_value());
    (*res)();
    EXPECT_EQ(value.load(), 10);
    auto task2 = queue.try_pop();
    EXPECT_FALSE(task2.has_value());
}

TEST(UnboundedQueueTest, OrderCheck) {
    UnboundedQueue queue;
    std::vector<int> value{};
    std::vector<int> check{0, 1, 2, 3, 4};

    for (int i = 0; i < 5; ++i) {
        queue.push([&value, i]() { value.push_back(i); });
    }
    for (int i = 0; i < 5; ++i) {
        auto task = queue.try_pop();
        ASSERT_TRUE(task.has_value());
        (*task)();
    }
    EXPECT_EQ(value, check);
}

TEST(UnboundedQueueTest, ConcurrentCheck) {
    UnboundedQueue queue;
    std::atomic<size_t> value{0};
    size_t check = 100;

    // тестовый поток, который будет извлекать задачи из очереди и выполнять их
    std::jthread t([&]() {
        int count{0};
        while (count < check) {
            auto task = queue.try_pop();
            if (task.has_value()) {
                (*task)();
                ++count;
            }
        }
    });

    for (int i = 0; i < check; ++i) {
        queue.push([&] { value.fetch_add(1, std::memory_order_relaxed); });
    }

    // ждем, пока тестовый поток не выполнит все задачи
    t.join();
    // проверяем, что все задачи были выполнены
    EXPECT_EQ(value.load(std::memory_order_relaxed), check);
}

// --------------------------

// deprecated
// TEST(BoundedQueueTest, ConstructorThrows) {
//     EXPECT_THROW((void)BoundedQueue(0), std::invalid_argument);
//     EXPECT_THROW((void)BoundedQueue(-1), std::invalid_argument);
// }
