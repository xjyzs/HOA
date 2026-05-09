/*
 * Copyright (c) 2024 HongEngine Project.
 *
 * Minimal event_runner.h stub for Android.
 */

#ifndef OHOS_EVENT_RUNNER_H
#define OHOS_EVENT_RUNNER_H

#include <memory>
#include <string>

namespace OHOS::AppExecFwk {

class EventRunner {
public:
    virtual ~EventRunner() = default;

    static std::shared_ptr<EventRunner> Create(const std::string& name)
    {
        return std::make_shared<EventRunner>();
    }

    static std::shared_ptr<EventRunner> Current()
    {
        return std::make_shared<EventRunner>();
    }

    static std::shared_ptr<EventRunner> GetMainEventRunner()
    {
        return std::make_shared<EventRunner>();
    }

    bool IsCurrentRunnerThread() { return false; }

    void Run() {}
    void Stop() {}
};

} // namespace OHOS::AppExecFwk

#endif // OHOS_EVENT_RUNNER_H
