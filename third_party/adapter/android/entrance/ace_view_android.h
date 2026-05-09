/*
 * Copyright (c) 2024 HongEngine Project.
 */

#ifndef HONGENGINE_ADAPTER_ANDROID_ACE_VIEW_ANDROID_H
#define HONGENGINE_ADAPTER_ANDROID_ACE_VIEW_ANDROID_H

#include <memory>

#include "base/utils/noncopyable.h"
#include "core/common/ace_view.h"
#include "core/common/platform_window.h"
#include "core/event/key_event_recognizer.h"

namespace OHOS::Ace::Platform {

// Minimal AceView for Android — follows preview adapter pattern.
// In Step A this is mostly stubs; Step B adds EGL/Skia rendering.
class AceViewAndroid : public AceView {
    DECLARE_ACE_TYPE(AceViewAndroid, AceView);

public:
    AceViewAndroid(int32_t instanceId, void* nativeWindow);
    ~AceViewAndroid() override = default;

    // AceView interface implementation (all callbacks stubbed for first frame)
    void Launch() override {}
    int32_t GetInstanceId() const override { return instanceId_; }

    void RegisterTouchEventCallback(TouchEventCallback&& callback) override {
        touchEventCallback_ = std::move(callback);
    }
    void RegisterKeyEventCallback(KeyEventCallback&& callback) override {
        keyEventCallback_ = std::move(callback);
    }
    void RegisterNonPointerEventCallback(NonPointerEventCallback&& callback) override {}
    void RegisterMouseEventCallback(MouseEventCallback&& callback) override {
        mouseEventCallback_ = std::move(callback);
    }
    void RegisterAxisEventCallback(AxisEventCallback&& callback) override {
        axisEventCallback_ = std::move(callback);
    }
    void RegisterRotationEventCallback(RotationEventCallBack&& callback) override {
        rotationEventCallBack_ = std::move(callback);
    }
    void RegisterCrownEventCallback(CrownEventCallback&& callback) override {}
    void RegisterDragEventCallback(DragEventCallBack&& callback) override {}
    void RegisterCardViewPositionCallback(CardViewPositionCallBack&& callback) override {}
    void RegisterCardViewAccessibilityParamsCallback(CardViewAccessibilityParamsCallback&& callback) override {}
    void RegisterViewChangeCallback(ViewChangeCallback&& callback) override {
        viewChangeCallback_ = std::move(callback);
    }
    void RegisterViewPositionChangeCallback(ViewPositionChangeCallback&& callback) override {}
    void RegisterDensityChangeCallback(DensityChangeCallback&& callback) override {
        densityChangeCallback_ = std::move(callback);
    }
    void RegisterTransformHintChangeCallback(TransformHintChangeCallback&& callback) override {}
    void RegisterSystemBarHeightChangeCallback(SystemBarHeightChangeCallback&& callback) override {}
    void RegisterSurfaceDestroyCallback(SurfaceDestroyCallback&& callback) override {
        surfaceDestroyCallback_ = std::move(callback);
    }
    void RegisterIdleCallback(IdleCallback&& callback) override {
        idleCallback_ = std::move(callback);
    }

    bool HandleTouchEvent(const TouchEvent& touchEvent) override { return false; }
    bool HandleKeyEvent(const KeyEvent& keyEvent) override {
        return keyEventCallback_ && keyEventCallback_(keyEvent);
    }

    const RefPtr<PlatformResRegister>& GetPlatformResRegister() const override { return resRegister_; }
    bool Dump(const std::vector<std::string>& params) override { return false; }
    ViewType GetViewType() const override { return ViewType::SURFACE_VIEW; }
    const void* GetNativeWindowById(uint64_t textureId) override { return nullptr; }
    std::unique_ptr<DrawDelegate> GetDrawDelegate() override;
    std::unique_ptr<PlatformWindow> GetPlatformWindow() override;

    void* GetNativeWindow() const { return nativeWindow_; }

    void NotifySurfaceChanged(int32_t width, int32_t height);
    void NotifyDensityChanged(double density);
    void NotifySurfaceDestroyed();

private:
    int32_t instanceId_;
    void* nativeWindow_; // ANativeWindow*
    RefPtr<PlatformResRegister> resRegister_;

    TouchEventCallback touchEventCallback_;
    MouseEventCallback mouseEventCallback_;
    AxisEventCallback axisEventCallback_;
    RotationEventCallBack rotationEventCallBack_;
    ViewChangeCallback viewChangeCallback_;
    DensityChangeCallback densityChangeCallback_;
    SurfaceDestroyCallback surfaceDestroyCallback_;
    IdleCallback idleCallback_;
    KeyEventCallback keyEventCallback_;

    ACE_DISALLOW_COPY_AND_MOVE(AceViewAndroid);
};

} // namespace OHOS::Ace::Platform

#endif // HONGENGINE_ADAPTER_ANDROID_ACE_VIEW_ANDROID_H
