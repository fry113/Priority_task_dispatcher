#include "thread_pool/thread_pool.hpp"
#include <exception>
#include <iostream>

namespace dispatcher::thread_pool {

ThreadPool::ThreadPool(std::shared_ptr<queue::PriorityQueue> task_queue, size_t num_threads)
    : task_queue_(std::move(task_queue)) {
    // резервируем место для потоков, чтобы избежать аллокаций
    workers_.reserve(num_threads);

    // запускаем рабочие потоки, которые будут выполнять задачи из очереди
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this] { loop(); });
    }
}

ThreadPool::~ThreadPool() { task_queue_->shutdown(); }

void ThreadPool::loop() {
    while (true) {
        auto task = task_queue_->pop();
        if (!task.has_value()) {
            // если pop() вернул std::nullopt, выходим из цикла
            break;
        }
        try {
            // выполняем задачу
            (*task)();
        } catch (const std::exception &e) {
            // обработка исключений, если задача выбросила исключение
            std::cerr << "ThreadPool::loop(): " << e.what() << std::endl;
        }
    }
}

}  // namespace dispatcher::thread_pool