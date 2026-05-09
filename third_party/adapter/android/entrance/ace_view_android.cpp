/*
 * Copyright (c) 2024 HongEngine Project.
 */

#include "adapter/android/entrance/ace_view_android.h"

#include "adapter/android/entrance/platform_window_android.h"
#include "base/log/log.h"
#include "core/common/platform_res_register.h"

namespace OHOS::Ace::Platform {

// Stub PlatformResRegister implementation for Android
class AndroidViewResRegister : public PlatformResRegister {
public:
    bool OnMethodCall(const std::string& method, const std::string& param, std::string& result) override { return false; }
    int64_t CreateResource(const std::string& resourceType, const std::string& param) override { return 0; }
    bool ReleaseResource(const std::string& resourceHash) override { return false; }
};

AceViewAndroid::AceViewAndroid(int32_t instanceId, void* nativeWindow)
    : instanceId_(instanceId), nativeWindow_(nativeWindow)
{
    resRegister_ = Referenced::MakeRefPtr<AndroidViewResRegister>();
    LOGI("AceViewAndroid created: instanceId=%{public}d, nativeWindow=%{public}p",
         instanceId_, nativeWindow_);
}

std::unique_ptr<DrawDelegate> AceViewAndroid::GetDrawDelegate()
{
    // Stub for Step A — Step B will provide Skia-backed DrawDelegate
    return nullptr;
}

std::unique_ptr<PlatformWindow> AceViewAndroid::GetPlatformWindow()
{
    // Step A returns nullptr (like preview).
    // Step B will return a real EGL-backed PlatformWindowAndroid.
    return nullptr;
}

void AceViewAndroid::NotifySurfaceChanged(int32_t width, int32_t height)
{
    width_ = width;
    height_ = height;
    if (viewChangeCallback_) {
        viewChangeCallback_(width, height, WindowSizeChangeReason::UNDEFINED, nullptr);
    }
}

void AceViewAndroid::NotifyDensityChanged(double density)
{
    if (densityChangeCallback_) {
        densityChangeCallback_(density);
    }
}

void AceViewAndroid::NotifySurfaceDestroyed()
{
    if (surfaceDestroyCallback_) {
        surfaceDestroyCallback_();
    }
}

} // namespace OHOS::Ace::Platform
