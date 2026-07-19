#include <gtest/gtest.h>

#include "task_dispatcher.hpp"

using namespace dispatcher;

TEST(TaskDispatcherTest, ConstructorCheck) {
    EXPECT_NO_THROW(TaskDispatcher td(4));
    EXPECT_NO_THROW(TaskDispatcher td(4, {{TaskPriority::High, {true, 100}}}));
}

TEST(TaskDispatcherTest, ConfigCheck) {
    std::atomic<int> value{0};
    int check{10};
    Conf config;
    config[TaskPriority::High] = {false, std::nullopt};
    auto task = [&] { value.fetch_add(1, std::memory_order_relaxed); };
    {
        TaskDispatcher td(2, config);
        EXPECT_THROW(td.schedule(TaskPriority::Normal, task), std::logic_error);
        EXPECT_NO_THROW(for (int i = 0; i < check; ++i) { td.schedule(TaskPriority::High, task); });
    }

    EXPECT_EQ(value.load(std::memory_order_relaxed), check);
}

TEST(TaskDispatcherTest, FinalizeDestructorCheck) {
    std::atomic<int> value{0};
    int check{2'000};
    auto task = [&] { value.fetch_add(1, std::memory_order_relaxed); };
    {
        TaskDispatcher td(4);
        for (int i = 0; i < check; ++i) {
            td.schedule(TaskPriority::Normal, task);
        }
    }
    EXPECT_EQ(value.load(std::memory_order_relaxed), check);
}

TEST(TaskDispatcherTest, DifPrioritiesCheck) {
    std::atomic<int> value{0};
    int check{2'000};
    auto task = [&] { value.fetch_add(1, std::memory_order_relaxed); };
    {
        TaskDispatcher td(4);
        for (int i = 0; i < check / 2; ++i) {
            td.schedule(TaskPriority::High, task);
            td.schedule(TaskPriority::Normal, task);
        }
    }
    EXPECT_EQ(value.load(std::memory_order_relaxed), check);
}

TEST(TaskDispatcherTest, ExceptionSafeCheck) {
    std::atomic<int> value{0};
    int check{10};
    auto fail_task = [] { throw std::runtime_error("check"); };
    auto task = [&] { value.fetch_add(1, std::memory_order_relaxed); };
    {
        TaskDispatcher td(4);
        td.schedule(TaskPriority::Normal, fail_task);
        for (int i = 0; i < check; ++i) {
            td.schedule(TaskPriority::Normal, task);
        }
    }
    EXPECT_EQ(value.load(std::memory_order_relaxed), check);
}

TEST(TaskDispatcherTest, SingleThreadOrderCheck) {
    std::vector<int> value{};
    std::vector<int> check{0, 1, 2, 3, 4};
    std::mutex mtx_;
    auto task = [&](int i) {
        std::lock_guard<std::mutex> lock(mtx_);
        value.emplace_back(i);
    };
    {
        TaskDispatcher td(1);
        for (int i = 0; i < 5; ++i) {
            td.schedule(TaskPriority::Normal, std::bind(task, i));
        }
    }
    EXPECT_EQ(value, check);
}

TEST(TaskDispatcherTest, SemiThreadCheck) {
    std::atomic<int> value{0};
    int check{400};
    auto task = [&] { value.fetch_add(1, std::memory_order_relaxed); };
    {
        TaskDispatcher td(4);
        std::vector<std::jthread> producers;
        for (int i = 0; i < 4; ++i) {
            producers.emplace_back([&]() {
                for (int j = 0; j < check / 4; ++j) {
                    td.schedule(TaskPriority::Normal, task);
                }
            });
        }
        for (auto &t : producers)
            t.join();
    }
    EXPECT_EQ(value.load(std::memory_order_relaxed), check);
}

// --------------------------
