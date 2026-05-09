/*
 * Copyright (c) 2024 HongEngine Project.
 *
 * Minimal parameters.h stub for Android.
 * Provides stubs for OHOS system parameter functions.
 */

#ifndef OHOS_SYSTEM_PARAMETERS_H
#define OHOS_SYSTEM_PARAMETERS_H

#include <string>

namespace OHOS::system {

inline bool GetBoolParameter(const std::string& key, bool defaultValue)
{
    return defaultValue;
}

inline std::string GetParameter(const std::string& key, const std::string& defaultValue)
{
    return defaultValue;
}

} // namespace OHOS::system

#endif // OHOS_SYSTEM_PARAMETERS_H
