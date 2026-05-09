/*
 * Copyright (c) 2024 HongEngine Project.
 */

#ifndef HONGENGINE_ADAPTER_ANDROID_PLATFORM_WINDOW_ANDROID_H
#define HONGENGINE_ADAPTER_ANDROID_PLATFORM_WINDOW_ANDROID_H

#include <functional>
#include <memory>

#include "core/common/platform_window.h"

// Forward declarations for Android NDK types
struct ANativeWindow;
typedef struct ANativeWindow ANativeWindow;

namespace OHOS::Ace::Platform {

// Step A: stub PlatformWindow (returns nullptr from AceViewAndroid).
// Step B: implements EGL + ANativeWindow rendering for first frame.
class PlatformWindowAndroid : public PlatformWindow {
public:
    PlatformWindowAndroid(ANativeWindow* nativeWindow);
    ~PlatformWindowAndroid() override;

    void Destroy() override;

    // PlatformWindow interface
    void RequestFrame() override;
    void RegisterVsyncCallback(AceVsyncCallback&& callback) override;
    void SetRootRenderNode(const RefPtr<RenderNode>& root) override;

private:
    ANativeWindow* nativeWindow_;
    AceVsyncCallback vsyncCallback_;
    RefPtr<RenderNode> rootNode_;

    // EGL state (Step B)
    void* eglDisplay_;  // EGLDisplay
    void* eglSurface_;  // EGLSurface
    void* eglContext_;  // EGLContext
    bool eglInitialized_;

    bool InitEGL();
    void DestroyEGL();
    void RenderFrame();
};

} // namespace OHOS::Ace::Platform

#endif // HONGENGINE_ADAPTER_ANDROID_PLATFORM_WINDOW_ANDROID_H
