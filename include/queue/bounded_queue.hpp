#pragma once
#include "queue/queue.hpp"
#include <condition_variable>
#include <mutex>
#include <queue>

namespace dispatcher::queue {

class BoundedQueue : public IQueue {
private:
    // хранение задач в очереди
    std::queue<std::function<void()>> queue_;

    // mutex для синхронизации доступа к очереди
    std::mutex mtx_;

    // condition_variable для ожидания, когда очередь не пуста или не полна
    std::condition_variable not_empty_;
    std::condition_variable not_full_;

    // максимальная вместимость очереди
    size_t capacity_;

public:
    explicit BoundedQueue(int capacity);

    void push(std::function<void()> task) override;

    std::optional<std::function<void()>> try_pop() override;

    ~BoundedQueue() override;
};

}  // namespace dispatcher::queue