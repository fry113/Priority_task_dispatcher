#include "queue/bounded_queue.hpp"
#include <gtest/gtest.h>

#include <mutex>
#include <thread>

using namespace dispatcher::queue;

TEST(BoundedQueueTest, ConstructorCheck) {
    EXPECT_NO_THROW(BoundedQueue(1));
    EXPECT_NO_THROW(BoundedQueue(INT_MAX));
}

TEST(BoundedQueueTest, ConstructorThrowsCheck) {
    EXPECT_THROW((void)BoundedQueue(0), std::invalid_argument);
    EXPECT_THROW((void)BoundedQueue(-1), std::invalid_argument);
}

TEST(BoundedQueueTest, TryPopEmptyCheck) {
    BoundedQueue queue(10);
    auto check = queue.try_pop();

    EXPECT_FALSE(check.has_value());
}

TEST(BoundedQueueTest, PushAndPopCheck) {
    BoundedQueue queue(10);
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

TEST(BoundedQueueTest, OrderCheck) {
    BoundedQueue queue(10);
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

TEST(BoundedQueueTest, PushFullQueueCheck) {
    BoundedQueue queue(5);
    std::atomic<int> value{0};
    auto task = [&]() { value.fetch_add(1); };

    for (int i = 0; i < 5; ++i) {
        queue.push([]() {});
    }
    for (int i = 0; i < 5; ++i) {
        auto task = queue.try_pop();
        EXPECT_TRUE(task.has_value());
    }
    EXPECT_FALSE(queue.try_pop().has_value());
}

TEST(BoundedQueueTest, PushOverQueueCheck) {
    BoundedQueue queue(5);
    std::atomic<int> value{0};
    auto task = [&]() { value.fetch_add(1); };

    for (int i = 0; i < 5; ++i) {
        queue.push([]() {});
    }
    for (int i = 0; i < 5; ++i) {
        auto task = queue.try_pop();
        EXPECT_TRUE(task.has_value());
    }
    EXPECT_FALSE(queue.try_pop().has_value());
}

TEST(BoundedQueueTest, ConcurrentCheck) {
    BoundedQueue queue(10);
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
