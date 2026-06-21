#include <gtest/gtest.h>

#include "queue/priority_queue.hpp"
#include <optional>
#include <thread>

using namespace dispatcher;
using namespace dispatcher::queue;

#define HIGH_PRIORITY_TASKS_COUNT 100
using Conf = std::map<TaskPriority, queue::QueueOptions>;
static Conf default_conf = {{TaskPriority::High, {true, HIGH_PRIORITY_TASKS_COUNT}},
                            {TaskPriority::Normal, {false, std::nullopt}}};

TEST(PriorityQueueTest, ConstructorCheck) { EXPECT_NO_THROW(PriorityQueue pq(default_conf)); }

TEST(PriorityQueueTest, ConstructorThrowsCheck) {
    Conf invalid_conf;
    invalid_conf[TaskPriority::High] = {true, std::nullopt};
    EXPECT_THROW(PriorityQueue pq(invalid_conf), std::logic_error);
}

TEST(PriorityQueueTest, AbsentPriorityThrowsCheck) {
    Conf config;
    config[TaskPriority::High] = {true, 10};
    PriorityQueue queue(config);
    auto task = [] {};
    EXPECT_THROW(queue.push(TaskPriority::Normal, task), std::logic_error);
}

TEST(PriorityQueueTest, ShutdownCheck) {
    PriorityQueue queue(default_conf);
    std::atomic<bool> value{false};
    queue.push(TaskPriority::High, [&] { value.store(true, std::memory_order_relaxed); });
    queue.shutdown();

    (*queue.pop())();
    EXPECT_TRUE(value.load(std::memory_order_relaxed));

    auto check = queue.pop();
    EXPECT_FALSE(check.has_value());
}

TEST(PriorityQueueTest, ShutdownAllDoneCheck) {
    PriorityQueue queue(default_conf);
    std::atomic<int> count{0};

    for (int i = 0; i < 10; ++i) {
        queue.push(TaskPriority::High, [&] { count.fetch_add(1, std::memory_order_relaxed); });
        queue.push(TaskPriority::Normal, [&] { count.fetch_add(1, std::memory_order_relaxed); });
        queue.push(TaskPriority::Normal, [&] { count.fetch_add(1, std::memory_order_relaxed); });
    }
    queue.shutdown();

    auto task = queue.pop();
    while (task.has_value()) {
        (*task)();
        task = queue.pop();
    }

    EXPECT_FALSE(task.has_value());
    EXPECT_FALSE(queue.pop().has_value());
    EXPECT_EQ(count.load(std::memory_order_relaxed), 30);
}

TEST(PriorityQueueTest, PriorityCheck) {
    PriorityQueue queue(default_conf);
    std::vector<int> value{};
    std::vector<int> check{1, 2, 3, 4, 5};

    queue.push(TaskPriority::Normal, [&] { value.push_back(3); });
    queue.push(TaskPriority::High, [&] { value.push_back(1); });
    queue.push(TaskPriority::Normal, [&] { value.push_back(4); });
    queue.push(TaskPriority::High, [&] { value.push_back(2); });
    queue.push(TaskPriority::Normal, [&] { value.push_back(5); });
    queue.shutdown();

    while (auto task = queue.pop()) {
        (*task)();
    }
    EXPECT_EQ(value, check);
}

TEST(PriorityQueueTest, SPMCCheck) {
    PriorityQueue queue(default_conf);
    std::atomic<int> value{0};
    int check{1'000};

    auto task = [&] { value.fetch_add(1, std::memory_order_relaxed); };
    auto push_tasks = [&] {
        for (int i = 0; i < check / 2; ++i) {
            queue.push(TaskPriority::High, task);
            queue.push(TaskPriority::Normal, task);
        }
        queue.shutdown();
    };
    auto execute_tasks = [&] {
        while (auto task = queue.pop()) {
            (*task)();
        }
    };

    std::jthread producer(push_tasks);

    std::vector<std::jthread> consumers;
    for (int i = 0; i < 5; ++i) {
        consumers.emplace_back(execute_tasks);
    }

    for (auto &t : consumers) {
        if (t.joinable()) {
            t.join();
        }
    }
    if (producer.joinable()) {
        producer.join();
    }

    EXPECT_EQ(value.load(std::memory_order_relaxed), check);
}

// --------------------------
