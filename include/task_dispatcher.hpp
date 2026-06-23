#pragma once

#include <memory>

#include "queue/priority_queue.hpp"
#include "thread_pool/thread_pool.hpp"
#include "types.hpp"

namespace dispatcher {

inline const size_t HIGH_PRIORITY_TASKS_COUNT = 1'000;
// отображение приоритета на конфигурацию представил в виде std::map,
// чтобы задавать конфигурацию как conf[High] = {true, 10} или conf[Low] = queueOption;
using Conf = std::map<TaskPriority, queue::QueueOptions>;
inline const Conf default_conf = {{TaskPriority::High, {true, HIGH_PRIORITY_TASKS_COUNT}},
                                  {TaskPriority::Normal, {false, std::nullopt}}};

class TaskDispatcher {
public:  // types
    using Task = std::function<void()>;

private:
    std::shared_ptr<queue::PriorityQueue> task_queue_;
    std::unique_ptr<thread_pool::ThreadPool> thread_pool_;

public:
    TaskDispatcher(size_t thread_count, const Conf &conf = default_conf);

    void schedule(TaskPriority priority, std::function<void()> task);
    ~TaskDispatcher();
};

}  // namespace dispatcher