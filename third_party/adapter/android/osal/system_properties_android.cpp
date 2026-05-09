/*
 * Copyright (c) 2024 HongEngine Project.
 *
 * Android device system properties — provides default values for
 * SystemProperties singleton used throughout ace_engine.
 */

#include "base/utils/system_properties.h"

#include <mutex>
#include <unordered_map>

namespace OHOS::Ace {

static std::unordered_map<std::string, std::string> g_androidSystemProps;
static std::mutex g_propsMutex;

void SystemProperties::InitDeviceType(DeviceType type)
{
    // Set by app at startup
}

DeviceType SystemProperties::GetDeviceType()
{
    return DeviceType::PHONE;
}

bool SystemProperties::IsSysSmartTV()
{
    return false;
}

int32_t SystemProperties::GetMcc()
{
    return 460; // China MCC
}

int32_t SystemProperties::GetMnc()
{
    return 0;
}

bool SystemProperties::IsScoringEnabled()
{
    return false;
}

bool SystemProperties::GetDebugEnabled()
{
    return false;
}

bool SystemProperties::GetTraceEnabled()
{
    return false;
}

bool SystemProperties::GetAccessibilityEnabled()
{
    return false;
}

bool SystemProperties::GetIsHookMode()
{
    return false;
}

ColorMode SystemProperties::GetColorMode()
{
    return ColorMode::LIGHT;
}

} // namespace OHOS::Ace
