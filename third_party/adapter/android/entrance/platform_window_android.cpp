/*
 * Copyright (c) 2024 HongEngine Project.
 */

#include "adapter/android/entrance/platform_window_android.h"

#include "base/log/log.h"
#include "core/pipeline/base/render_node.h"

// EGL and GLES headers (from Android NDK)
#include <EGL/egl.h>
#include <GLES2/gl2.h>

namespace OHOS::Ace::Platform {

PlatformWindowAndroid::PlatformWindowAndroid(ANativeWindow* nativeWindow)
    : nativeWindow_(nativeWindow),
      eglDisplay_(EGL_NO_DISPLAY),
      eglSurface_(EGL_NO_SURFACE),
      eglContext_(EGL_NO_CONTEXT),
      eglInitialized_(false)
{
    LOGI("PlatformWindowAndroid created: nativeWindow=%{public}p", nativeWindow_);
}

PlatformWindowAndroid::~PlatformWindowAndroid()
{
    Destroy();
}

void PlatformWindowAndroid::Destroy()
{
    DestroyEGL();
    nativeWindow_ = nullptr;
}

bool PlatformWindowAndroid::InitEGL()
{
    if (eglInitialized_) {
        return true;
    }

    eglDisplay_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (eglDisplay_ == EGL_NO_DISPLAY) {
        LOGE("PlatformWindowAndroid: eglGetDisplay failed");
        return false;
    }

    EGLint major, minor;
    if (!eglInitialize(eglDisplay_, &major, &minor)) {
        LOGE("PlatformWindowAndroid: eglInitialize failed");
        return false;
    }
    LOGI("PlatformWindowAndroid: EGL %{public}d.%{public}d", major, minor);

    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs;
    if (!eglChooseConfig(eglDisplay_, attribs, &config, 1, &numConfigs) || numConfigs < 1) {
        LOGE("PlatformWindowAndroid: eglChooseConfig failed");
        return false;
    }

    const EGLint ctxAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    eglContext_ = eglCreateContext(eglDisplay_, config, EGL_NO_CONTEXT, ctxAttribs);
    if (eglContext_ == EGL_NO_CONTEXT) {
        LOGE("PlatformWindowAndroid: eglCreateContext failed");
        return false;
    }

    eglSurface_ = eglCreateWindowSurface(eglDisplay_, config, nativeWindow_, nullptr);
    if (eglSurface_ == EGL_NO_SURFACE) {
        LOGE("PlatformWindowAndroid: eglCreateWindowSurface failed");
        return false;
    }

    eglInitialized_ = true;
    LOGI("PlatformWindowAndroid: EGL initialized successfully");
    return true;
}

void PlatformWindowAndroid::DestroyEGL()
{
    if (eglDisplay_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(eglDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (eglSurface_ != EGL_NO_SURFACE) {
            eglDestroySurface(eglDisplay_, eglSurface_);
            eglSurface_ = EGL_NO_SURFACE;
        }
        if (eglContext_ != EGL_NO_CONTEXT) {
            eglDestroyContext(eglDisplay_, eglContext_);
            eglContext_ = EGL_NO_CONTEXT;
        }
        eglTerminate(eglDisplay_);
        eglDisplay_ = EGL_NO_DISPLAY;
    }
    eglInitialized_ = false;
}

void PlatformWindowAndroid::RequestFrame()
{
    if (!eglInitialized_ && !InitEGL()) {
        LOGE("PlatformWindowAndroid::RequestFrame: EGL not initialized");
        return;
    }

    RenderFrame();
}

void PlatformWindowAndroid::RegisterVsyncCallback(AceVsyncCallback&& callback)
{
    vsyncCallback_ = std::move(callback);
}

void PlatformWindowAndroid::SetRootRenderNode(const RefPtr<RenderNode>& root)
{
    rootNode_ = root;
}

void PlatformWindowAndroid::RenderFrame()
{
    if (eglDisplay_ == EGL_NO_DISPLAY || eglSurface_ == EGL_NO_SURFACE) {
        return;
    }

    if (!eglMakeCurrent(eglDisplay_, eglSurface_, eglSurface_, eglContext_)) {
        LOGE("PlatformWindowAndroid: eglMakeCurrent failed");
        return;
    }

    // First frame: clear to a solid color (dark blue-gray)
    glClearColor(0.15f, 0.15f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    eglSwapBuffers(eglDisplay_, eglSurface_);

    LOGI("PlatformWindowAndroid: first frame rendered");
}

} // namespace OHOS::Ace::Platform
