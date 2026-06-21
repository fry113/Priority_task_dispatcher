#include "queue/priority_queue.hpp"
#include "types.hpp"

namespace dispatcher::queue {

PriorityQueue::PriorityQueue(const Conf &conf) {
    // инициализируем очереди в зависимости от конфигурации
    for (const auto &[priority, option] : conf) {
        if (option.bounded) {
            if (!option.capacity.has_value()) {
                throw std::logic_error("Не указан размер для bounded приоритета в конфигурации");
            }
            queues_[priority] = std::make_unique<BoundedQueue>(option.capacity.value());
        } else {
            queues_[priority] = std::make_unique<UnboundedQueue>();
        }
    }
}

PriorityQueue::~PriorityQueue() { shutdown(); }

void PriorityQueue::push(TaskPriority priority, Task task) {
    if (shutdown_) {
        throw std::runtime_error("Попытка добавить задачу в остановленную очередь");
    }

    // проверяем наличие приоритета в конфигурации на случай, если сконфигурировано
    // меньше приоритетов, чем доступно в TaskPriority
    auto it = queues_.find(priority);
    if (it == queues_.end()) {
        throw std::logic_error("Приоритет не найден в конфигурации очереди");
    }
    it->second->push(std::move(task));
    // отпускаем mutex и уведомляем один поток
    cv_.notify_one();
}

std::optional<PriorityQueue::Task> PriorityQueue::pop() {
    std::unique_lock lock(mtx_);
    while (true) {
        // проверяем наличие задач в очередях по приоритету
        for (auto &[priority, queue] : queues_) {
            auto task = queue->try_pop();
            // если удалось извлечь задачу из очереди, возвращаем ее
            if (task.has_value()) {
                return task;
            }
        }

        // если задач нет, проверяем shutdown
        if (shutdown_.load()) {
            return std::nullopt;
        }

        // если задач нет и shutdown не вызван, ждем уведомления о появлении задачи
        // при ложном пробуждении мы просто сделаем еще один безопасный круг проверки
        cv_.wait(lock);
    }
}

void PriorityQueue::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mtx_);
        shutdown_.store(true);
    }
    cv_.notify_all();
}

}  // namespace dispatcher::queue