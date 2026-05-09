/*
 * Copyright (c) 2024 HongEngine Project.
 */

#include "adapter/android/osal/task_executor_android.h"

#include <algorithm>

#include "base/log/log.h"

namespace OHOS::Ace::Platform {

TaskExecutorAndroid::TaskExecutorAndroid()
    : running_(true), mainThreadId_(std::this_thread::get_id())
{
    workerThread_ = std::thread(&TaskExecutorAndroid::ThreadLoop, this);
    threadIds_[TaskType::PLATFORM] = 0;
    threadIds_[TaskType::UI] = 0;
    threadIds_[TaskType::IO] = 0;
    threadIds_[TaskType::GPU] = 0;
    threadIds_[TaskType::JS] = 0;
    threadIds_[TaskType::BACKGROUND] = 0;

    LOGI("TaskExecutorAndroid: created");
}

TaskExecutorAndroid::~TaskExecutorAndroid()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
    }
    cv_.notify_all();
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    LOGI("TaskExecutorAndroid: destroyed");
}

void TaskExecutorAndroid::AddTaskObserver(Task&& callback)
{
    observer_ = std::move(callback);
}

void TaskExecutorAndroid::RemoveTaskObserver()
{
    observer_ = nullptr;
}

bool TaskExecutorAndroid::WillRunOnCurrentThread(TaskType type) const
{
    return std::this_thread::get_id() == workerThread_.get_id();
}

void TaskExecutorAndroid::RemoveTask(TaskType type, const std::string& name)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pendingTasks_.erase(
        std::remove_if(pendingTasks_.begin(), pendingTasks_.end(),
            [&](const TaskEntry& entry) { return entry.name == name && entry.type == type; }),
        pendingTasks_.end());
}

int32_t TaskExecutorAndroid::GetTid(TaskType type)
{
    auto it = threadIds_.find(type);
    if (it != threadIds_.end()) {
        return it->second;
    }
    return 0;
}

bool TaskExecutorAndroid::OnPostTask(
    Task&& task, TaskType type, uint32_t delayTime, const std::string& name,
    PriorityType priorityType) const
{
    if (!running_) {
        return false;
    }

    Task wrappedTask = std::move(task);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingTasks_.push_back({std::move(wrappedTask), name, type, priorityType});
    }
    cv_.notify_one();
    return true;
}

TaskExecutor::Task TaskExecutorAndroid::WrapTaskWithTraceId(Task&& task, int32_t id) const
{
    return std::move(task);
}

bool TaskExecutorAndroid::OnPostTaskWithoutTraceId(
    Task&& task, TaskType type, uint32_t delayTime, const std::string& name,
    PriorityType priorityType) const
{
    return OnPostTask(std::move(task), type, delayTime, name, priorityType);
}

void TaskExecutorAndroid::ThreadLoop()
{
    while (true) {
        TaskEntry entry;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return !pendingTasks_.empty() || !running_; });
            if (!running_ && pendingTasks_.empty()) {
                return;
            }
            if (pendingTasks_.empty()) {
                continue;
            }
            entry = std::move(pendingTasks_.front());
            pendingTasks_.erase(pendingTasks_.begin());
        }

        if (observer_) {
            observer_();
        }
        if (entry.task) {
            entry.task();
        }
    }
}

} // namespace OHOS::Ace::Platform
