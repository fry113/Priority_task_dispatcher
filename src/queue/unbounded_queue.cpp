#include "queue/unbounded_queue.hpp"

#include <functional>
#include <mutex>

namespace dispatcher::queue {

UnboundedQueue::UnboundedQueue() = default;

UnboundedQueue::~UnboundedQueue() = default;

void UnboundedQueue::push(std::function<void()> task) {
    // блокируем доступ к очереди, добавляем задачу и разблокируем по выходе из функции
    std::lock_guard<std::mutex> lock(mtx_);
    queue_.push_back(std::move(task));
}

std::optional<std::function<void()>> UnboundedQueue::try_pop() {
    // блокируем доступ к очереди
    std::lock_guard<std::mutex> lock(mtx_);
    // если очередь пуста, возвращаем std::nullopt
    if (queue_.empty()) {
        return std::nullopt;
    }
    // извлекаем задачу и разблокируем по выходе из функции
    auto task = std::move(queue_.front());
    queue_.pop_front();
    return task;
}
}  // namespace dispatcher::queue