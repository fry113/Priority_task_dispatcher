#pragma once
#include "queue/queue.hpp"
#include <list>
#include <mutex>

namespace dispatcher::queue {

class UnboundedQueue : public IQueue {
private:
    // хранение задач в очереди
    std::list<std::function<void()>> queue_;

    // mutex для синхронизации доступа к очереди
    mutable std::mutex mtx_;

public:
    explicit UnboundedQueue();

    void push(std::function<void()> task) override;

    std::optional<std::function<void()>> try_pop() override;

    ~UnboundedQueue() override;
};

}  // namespace dispatcher::queue