/*
 * Copyright (c) 2024 HongEngine Project.
 *
 * Minimal event_handler.h stub for Android.
 */

#ifndef OHOS_EVENT_HANDLER_H
#define OHOS_EVENT_HANDLER_H

#include <functional>
#include <memory>
#include <string>

#include "event_runner.h"

namespace OHOS::AppExecFwk {

struct EventQueue {
    enum class Priority {
        VIP = -1,
        IMMEDIATE = 0,
        HIGH = 1,
        LOW = 2,
        IDLE = 3,
    };
};

class EventHandler {
public:
    using Callback = std::function<void()>;

    explicit EventHandler(std::shared_ptr<EventRunner> runner) {}
    virtual ~EventHandler() = default;

    bool PostTask(Callback task, const std::string& name,
                  int64_t delayTime, EventQueue::Priority priority)
    {
        return false;
    }

    bool PostTimingTask(Callback task, int64_t delayTime, const std::string& caller)
    {
        return false;
    }

    void RemoveTask(const std::string& name) {}
};

} // namespace OHOS::AppExecFwk

#endif // OHOS_EVENT_HANDLER_H
