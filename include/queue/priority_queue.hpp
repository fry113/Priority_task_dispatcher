#pragma once
#include "queue/bounded_queue.hpp"
#include "queue/unbounded_queue.hpp"
#include "types.hpp"

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>

namespace dispatcher::queue {

// отображение приоритета на конфигурацию представил в виде std::map,
// чтобы задавать конфигурацию как conf[High] = {true, 10} или conf[Low] = queueOption;
using Conf = std::map<TaskPriority, QueueOptions>;

class PriorityQueue {
public:  // types
    using Task = std::function<void()>;

private:
    // std::map для всех приоритетов. ключ - TaskPriority, значение - очередь
    // забираем задачи из начала очереди, если она не пуста, иначе - std::nullopt
    std::map<TaskPriority, std::unique_ptr<IQueue>> queues_;

    // флаг для остановки очереди
    std::atomic<bool> shutdown_{false};

    // std::mutex и std::condition_variable для управления потоком
    std::mutex mtx_;
    std::condition_variable cv_;

public:
    explicit PriorityQueue(const Conf &conf);

    void push(TaskPriority priority, Task task);
    // block on pop until shutdown is called
    // after that return std::nullopt on empty queue
    std::optional<Task> pop();

    void shutdown();

    ~PriorityQueue();
};

}  // namespace dispatcher::queue