/*
 * Copyright (c) 2024 HongEngine Project.
 *
 * Platform linker stubs for Android — provides symbols that are normally
 * implemented in OHOS adapter source files.
 */

#include <cstdint>
#include <string>
#include <unordered_map>

#include <android/log.h>

#include "base/log/log_wrapper.h"
#include "base/log/dump_log.h"
#include "base/log/log.h"
#include "base/utils/system_properties.h"
#include "base/json/json_util.h"
#include "base/subwindow/subwindow_manager.h"
#include "core/common/display_info_utils.h"
#include "core/pipeline/pipeline_base.h"

namespace OHOS::Ace {

// =========================================================================
// LogWrapper
// =========================================================================

const std::unordered_map<AceLogTag, const char*> g_DOMAIN_CONTENTS_MAP = {
    { AceLogTag::ACE_DEFAULT_DOMAIN, "Ace" },
};

LogLevel LogWrapper::level_ = LogLevel::DEBUG;

char LogWrapper::GetSeparatorCharacter()
{
    return '/';
}

void LogWrapper::PrintLog(LogDomain domain, LogLevel level, AceLogTag tag,
                          const char* fmt, va_list args)
{
    int prio;
    switch (level) {
        case LogLevel::DEBUG: prio = ANDROID_LOG_DEBUG; break;
        case LogLevel::INFO:  prio = ANDROID_LOG_INFO; break;
        case LogLevel::WARN:  prio = ANDROID_LOG_WARN; break;
        case LogLevel::ERROR: prio = ANDROID_LOG_ERROR; break;
        case LogLevel::FATAL: prio = ANDROID_LOG_FATAL; break;
        default: prio = ANDROID_LOG_INFO; break;
    }
    __android_log_vprint(prio, "ACE", fmt, args);
}

bool LogBacktrace(size_t maxFrameNums)
{
    return false;
}

// =========================================================================
// DumpLog
// =========================================================================

DumpLog::DumpLog() = default;
DumpLog::~DumpLog() = default;

void DumpLog::Print(int32_t depth, const std::string& className, int32_t childSize)
{
}

void DumpLog::Print(const std::string& content)
{
    LOGD("DumpLog: %{public}s", content.c_str());
}

void DumpLog::Print(int32_t depth, const std::string& content)
{
    LOGD("DumpLog[%{public}d]: %{public}s", depth, content.c_str());
}

// =========================================================================
// DisplayInfoUtils
// =========================================================================

RefPtr<DisplayInfo> DisplayInfoUtils::GetDisplayInfo(int32_t displayId)
{
    return AceType::MakeRefPtr<DisplayInfo>();
}

void DisplayInfoUtils::InitIsFoldable()
{
    hasInitIsFoldable_ = true;
}

bool DisplayInfoUtils::GetIsFoldable()
{
    return false;
}

FoldStatus DisplayInfoUtils::GetCurrentFoldStatus()
{
    return FoldStatus::UNKNOWN;
}

std::vector<Rect> DisplayInfoUtils::GetCurrentFoldCreaseRegion()
{
    return {};
}

// =========================================================================
// SubwindowManager
// =========================================================================

std::shared_ptr<SubwindowManager> SubwindowManager::GetInstance()
{
    static auto instance = std::make_shared<SubwindowManager>();
    return instance;
}

const RefPtr<Subwindow> SubwindowManager::GetToastSubwindow(int32_t instanceId)
{
    return nullptr;
}

const RefPtr<Subwindow> SubwindowManager::GetSystemToastWindow(int32_t instanceId)
{
    return nullptr;
}

const RefPtr<Subwindow> SubwindowManager::GetSelectOverlaySubwindow(int32_t instanceId)
{
    return nullptr;
}

// =========================================================================
// PipelineBase
// =========================================================================

void PipelineBase::SetFontScale(float scale)
{
    fontScale_ = scale;
}

void PipelineBase::SetFontWeightScale(float scale)
{
    fontWeightScale_ = scale;
}

} // namespace OHOS::Ace

// =========================================================================
// JsonUtil (needs cJSON forward decl)
// =========================================================================

#include "json/json_util.h"

namespace OHOS::Ace {

std::unique_ptr<JsonValue> JsonUtil::Create(bool isRoot)
{
    // Minimal stub: return empty JsonValue without cJSON allocation.
    return std::make_unique<JsonValue>();
}

// =========================================================================
// SystemProperties static members and methods
// =========================================================================

bool SystemProperties::isHookModeEnabled_ = false;
bool SystemProperties::debugEnabled_ = false;

bool SystemProperties::GetIsUseMemoryMonitor()
{
    return false;
}

} // namespace OHOS::Ace
