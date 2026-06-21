#pragma once

#include "queue/priority_queue.hpp"
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace dispatcher::thread_pool {

class ThreadPool {
public:  // types
    using Task = std::function<void()>;

private:
    // jthread для автоматического join при уничтожении
    std::vector<std::jthread> workers_;
    std::shared_ptr<queue::PriorityQueue> task_queue_;

    // приватный метод, который запускает все рабочие потоки
    void loop();

public:
    explicit ThreadPool(std::shared_ptr<queue::PriorityQueue> task_queue, size_t num_threads);
    ~ThreadPool();
};

}  // namespace dispatcher::thread_pool
