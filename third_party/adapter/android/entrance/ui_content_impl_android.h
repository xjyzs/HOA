/*
 * Copyright (c) 2024 HongEngine Project.
 */

#ifndef HONGENGINE_ADAPTER_ANDROID_UI_CONTENT_IMPL_ANDROID_H
#define HONGENGINE_ADAPTER_ANDROID_UI_CONTENT_IMPL_ANDROID_H

#include <memory>
#include <string>

#include "interfaces/inner_api/ace/ui_content.h"

namespace OHOS::Ace::Platform {

// Minimal UIContent implementation for Android.
// Follows preview UIContentImpl pattern, stubbed for first frame.
class UIContentImplAndroid : public UIContent {
public:
    UIContentImplAndroid() = default;
    ~UIContentImplAndroid() override = default;

    // UIContent interface
    UIContentErrorCode Initialize(
        const std::string& assetPath, const std::string& params,
        const std::vector<std::string>& hapPathList = {}) override;

    UIContentErrorCode RunPage(
        const std::string& content, const std::string& params,
        const std::string& profile = "") override;

    void Destroy() override;
    void UpdateViewConfig(const ViewportConfig& config) override;

    void UpdateConfiguration(const std::shared_ptr<AbilityBase::Configuration>& configuration,
        const ConfigurationChange& configurationChange) override;

    void Foreground() override {}
    void Background() override {}
    void ReloadFormBundle(const std::string& bundleName) override {}
    void SetIsActive(bool isActive) override {}
    void SetFontScale(float fontScale) override {}
    void SetFontWeightScale(float fontWeightScale) override {}
    bool ProcessPointerEvent(const std::shared_ptr<MMI::PointerEvent>& pointerEvent) override { return false; }
    bool ProcessAxisEvent(const std::shared_ptr<MMI::AxisEvent>& axisEvent) override { return false; }
    bool ProcessPointerEventWithGPU(const std::shared_ptr<MMI::PointerEvent>& pointerEvent) override { return false; }
    bool ProcessKeyEvent(const std::shared_ptr<MMI::KeyEvent>& keyEvent, bool isPreIme) override { return false; }
    bool ProcessBackPressed() override { return false; }
    ColorMode GetLocalColorMode() override { return ColorMode::LIGHT; }
    void SetParentToken(sptr<IRemoteObject> token) override {}

    // Preview-specific extensions (stubbed)
    void SetFrameLayoutCallback(std::function<void()>&& callback) override {}
    void NotifyWindowMode(WindowMode mode) override {}
    void SetFrameRateCalculateCallback(std::function<void(std::vector<uint64_t>&&)>&& callback) override {}
    void SetPartialUpdateCallback(std::function<bool()>&& callback) override {}
    void SetOnErrorCallback(std::function<void(const std::string&, const std::string&)>&& callback) override {}
    void SetOnRouterChangeCallback(std::function<void(const std::string&)>&& callback) override {}
    void SetDynamicBrightnessValue(int32_t value) override {}

private:
    int32_t instanceId_ = 0;
    bool initialized_ = false;
};

} // namespace OHOS::Ace::Platform

#endif // HONGENGINE_ADAPTER_ANDROID_UI_CONTENT_IMPL_ANDROID_H
