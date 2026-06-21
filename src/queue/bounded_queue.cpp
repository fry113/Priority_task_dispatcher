#include "queue/bounded_queue.hpp"

namespace dispatcher::queue {

#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>

using namespace std;

BoundedQueue::BoundedQueue(int capacity) {
    // capacity int-овый. проверяем, что он положительный, иначе - исключение
    if (capacity <= 0) {
        throw std::invalid_argument("Ограниченная очередь должна иметь положительную вместимость");
    }
    capacity_ = static_cast<size_t>(capacity);
}

void BoundedQueue::push(std::function<void()> task) {
    // unique_lock можно передать в condition_variable для ожидания, когда очередь не полна
    unique_lock<mutex> lock(mtx_);
    not_full_.wait(lock, [this] { return queue_.size() < capacity_; });

    // если очередь не полна, добавляем задачу
    queue_.push(std::move(task));

    // уведомляем один поток, что очередь не пуста
    not_empty_.notify_one();
}

std::optional<std::function<void()>> BoundedQueue::try_pop() {
    // unique_lock можно передать в condition_variable для ожидания, когда очередь не пуста
    unique_lock<mutex> lock(mtx_);
    if (queue_.empty()) {
        return std::nullopt;
    }

    // извлекаем задачу из очереди и уведомляем один поток, что очередь не полна
    auto task = std::move(queue_.front());
    queue_.pop();

    not_full_.notify_one();
    return task;
}

BoundedQueue::~BoundedQueue() = default;

}  // namespace dispatcher::queue