#include "task_dispatcher.hpp"

namespace dispatcher {

TaskDispatcher::TaskDispatcher(size_t thread_count, const Conf &conf)
    : task_queue_(std::make_shared<queue::PriorityQueue>(conf)),
      thread_pool_(std::make_unique<thread_pool::ThreadPool>(task_queue_, thread_count)) {}

TaskDispatcher::~TaskDispatcher() = default;

void TaskDispatcher::schedule(TaskPriority priority, std::function<void()> task) {
    // добавляем задачу в очередь
    task_queue_->push(priority, std::move(task));
}

}  // namespace dispatcher