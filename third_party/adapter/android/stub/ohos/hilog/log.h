/*
 * Copyright (c) 2024 HongEngine Project.
 *
 * Minimal hilog/log.h stub for Android — maps OHOS HiLog to Android logcat.
 */

#ifndef HIVIEWDFX_HILOG_H
#define HIVIEWDFX_HILOG_H

#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LOG_APP = 0,
    LOG_CORE = 1,
} LogType;

typedef enum {
    LOG_DEBUG = 3,
    LOG_INFO = 4,
    LOG_WARN = 5,
    LOG_ERROR = 6,
    LOG_FATAL = 7,
} LogLevel;

// Map OHOS log macros to Android __android_log_print
// HILOG_IMPL(type, level, domain, tag, fmt, ...)
#define HILOG_IMPL(type, level, domain, tag, fmt, ...)                   \
    do {                                                                  \
        int _prio;                                                       \
        switch (level) {                                                 \
            case LOG_DEBUG: _prio = ANDROID_LOG_DEBUG; break;            \
            case LOG_INFO:  _prio = ANDROID_LOG_INFO; break;             \
            case LOG_WARN:  _prio = ANDROID_LOG_WARN; break;             \
            case LOG_ERROR: _prio = ANDROID_LOG_ERROR; break;            \
            case LOG_FATAL: _prio = ANDROID_LOG_FATAL; break;            \
            default:        _prio = ANDROID_LOG_INFO; break;             \
        }                                                                \
        __android_log_print(_prio, tag, fmt, ##__VA_ARGS__);             \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif // HIVIEWDFX_HILOG_H
