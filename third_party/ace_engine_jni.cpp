/*
 * Copyright (c) 2024 HongEngine Project.
 *
 * JNI bridge for ACE engine initialization on Android.
 * Phase 3b Step B: creates Container, PipelineContext, renders first frame.
 */

#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <string>

#define LOG_TAG "HongEngine_ACE"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ACE engine headers
#include "adapter/android/entrance/ace_container_android.h"
#include "adapter/android/entrance/ace_view_android.h"
#include "adapter/android/entrance/platform_window_android.h"
#include "base/log/log.h"
#include "core/common/ace_application_info.h"
#include "core/common/container.h"

using namespace OHOS::Ace;
using namespace OHOS::Ace::Platform;

extern "C" {

JNIEXPORT void JNICALL
Java_app_hackeris_hongengine_StageActivity_nativeInitAce(
    JNIEnv* env, jobject thiz, jobject surface, jstring filesDir)
{
    LOGI("nativeInitAce: starting ACE engine initialization");

    // 1. Get ANativeWindow from Surface
    ANativeWindow* nativeWindow = ANativeWindow_fromSurface(env, surface);
    if (!nativeWindow) {
        LOGE("nativeInitAce: failed to get ANativeWindow from Surface");
        return;
    }
    LOGI("nativeInitAce: got ANativeWindow %p", nativeWindow);

    // 2. Get files directory path
    const char* filesPath = env->GetStringUTFChars(filesDir, nullptr);
    std::string assetPath(filesPath);
    env->ReleaseStringUTFChars(filesDir, filesPath);
    LOGI("nativeInitAce: assetPath=%s", assetPath.c_str());

    // 3. Initialize ACE application info
    AceApplicationInfo::GetInstance().SetPackageName("app.hackeris.hongengine");
    AceApplicationInfo::GetInstance().SetDataFileDirPath(assetPath);
    AceApplicationInfo::GetInstance().SetProcessName("hongengine");

    // 4. Create container
    constexpr int32_t instanceId = 0;
    AceContainerAndroid::CreateContainer(instanceId, FrontendType::DECLARATIVE_JS, true);

    auto container = AceContainerAndroid::GetContainerInstance(instanceId);
    if (!container) {
        LOGE("nativeInitAce: failed to create AceContainerAndroid");
        ANativeWindow_release(nativeWindow);
        return;
    }

    // 5. Create AceView
    auto* aceView = new AceViewAndroid(instanceId, nativeWindow);
    int32_t width = ANativeWindow_getWidth(nativeWindow);
    int32_t height = ANativeWindow_getHeight(nativeWindow);
    if (width <= 0) width = 1080;
    if (height <= 0) height = 1920;

    aceView->NotifySurfaceChanged(width, height);

    // 6. Initialize container
    container->SetView(aceView, 2.0, width, height);
    container->Initialize();

    LOGI("nativeInitAce: ACE engine initialized successfully");

    // 7. Create PlatformWindow for first frame
    auto platformWindow = std::make_unique<PlatformWindowAndroid>(nativeWindow);
    platformWindow->RequestFrame();

    LOGI("nativeInitAce: first frame requested");
}

JNIEXPORT void JNICALL
Java_app_hackeris_hongengine_StageActivity_nativeSurfaceChanged(
    JNIEnv* env, jobject thiz, jobject surface, jint width, jint height)
{
    LOGI("nativeSurfaceChanged: %dx%d", width, height);
    // Handle surface changes in future phases
}

JNIEXPORT void JNICALL
Java_app_hackeris_hongengine_StageActivity_nativeDestroyAce(
    JNIEnv* env, jobject thiz)
{
    LOGI("nativeDestroyAce: destroying ACE engine");
    AceContainerAndroid::DestroyContainer(0);
}

} // extern "C"
