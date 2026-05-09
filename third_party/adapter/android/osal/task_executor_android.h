/*
 * Copyright (c) 2024 HongEngine Project.
 */

#ifndef HONGENGINE_ADAPTER_ANDROID_TASK_EXECUTOR_ANDROID_H
#define HONGENGINE_ADAPTER_ANDROID_TASK_EXECUTOR_ANDROID_H

#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#include "base/thread/task_executor.h"

namespace OHOS::Ace::Platform {

// Minimal TaskExecutor for Android using pthread/thread.
// Step A: single-threaded implementation (all tasks run on same thread).
// Step B: proper multi-threaded with Android Looper.
class TaskExecutorAndroid : public TaskExecutor {
    DECLARE_ACE_TYPE(TaskExecutorAndroid, TaskExecutor);

public:
    TaskExecutorAndroid();
    ~TaskExecutorAndroid() override;

    // TaskExecutor pure virtuals
    void AddTaskObserver(Task&& callback) override;
    void RemoveTaskObserver() override;
    bool WillRunOnCurrentThread(TaskType type) const override;
    void RemoveTask(TaskType type, const std::string& name) override;
    int32_t GetTid(TaskType type) override;

protected:
    bool OnPostTask(Task&& task, TaskType type, uint32_t delayTime, const std::string& name,
        PriorityType priorityType = PriorityType::LOW) const override;
    Task WrapTaskWithTraceId(Task&& task, int32_t id) const override;
    bool OnPostTaskWithoutTraceId(Task&& task, TaskType type, uint32_t delayTime, const std::string& name,
        PriorityType priorityType = PriorityType::LOW) const override;

private:
    struct TaskEntry {
        Task task;
        std::string name;
        TaskType type;
        PriorityType priority;
    };

    void ThreadLoop();

    mutable std::mutex mutex_;
    mutable std::condition_variable cv_;
    mutable std::vector<TaskEntry> pendingTasks_;
    std::thread workerThread_;
    bool running_;
    Task observer_;
    std::map<TaskType, int32_t> threadIds_;
    std::thread::id mainThreadId_;
};

} // namespace OHOS::Ace::Platform

#endif // HONGENGINE_ADAPTER_ANDROID_TASK_EXECUTOR_ANDROID_H
