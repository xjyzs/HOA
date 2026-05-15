# ArkUI-X Android 渲染管线

## 概述

ArkUI-X 在 Android 上通过 Virtual Rosen Window 和 Skia 实现渲染，使用 Android SurfaceView 作为渲染表面。

## 渲染管线架构

```
JS 页面代码 (ETS)
  → DeclarativeFrontendNG
    → PipelineContext (NG)
      → RSSurfaceNode
        → Skia Canvas
          → ANativeWindow (SurfaceView)
            → Android SurfaceFlinger
```

## 关键组件

### SurfaceView 创建

**文件**: `StageActivity.java`

```java
// 默认使用 SurfaceView
boolean isUseSurfaceView() { return true; }

windowView = WindowViewBuilder.makeWindowViewAosp(this, instanceId, isUseSurfaceView());
// → WindowViewAospSurface (继承 SurfaceView)
// 或 WindowViewAospTexture (继承 TextureView)

setContentView(windowView.getView());
```

### Virtual Rosen Window

**文件**: `virtual_rs_window.h` / `virtual_rs_window.cpp`

`Rosen::Window` 的虚拟实现，替代 OHOS 的 WindowManager:
- 不依赖 OHOS WindowManager Service
- 由 Android SurfaceView 提供 surface
- VSync 由 `RSInterfaces::CreateVSyncReceiver` 虚拟实现提供

### Window::SetUIContent

**文件**: `virtual_rs_window.cpp:1156-1182`

```cpp
WMError Window::SetUIContent(...) {
    uiContent = UIContent::Create(context_.get(), engine);
    uiContent->Initialize(this, contentInfo, storage);
    // → UIContentImpl::Initialize
    //   → AceViewSG::CreateView(instanceId)
    //   → AceContainerSG 创建 + 注册到 AceEngine
    //   → AceContainerSG::SetView(aceView, density, width, height, rsWindow)
}
```

### AceContainerSG 初始化

**文件**: `ace_container_sg.cpp`

```cpp
AceContainerSG::SetView(aceView, density, w, h, rsWindow):
  1. 初始化 frontend (DeclarativeFrontendNG for ETS)
  2. 创建 NG::PipelineContext (声明式渲染管线)
  3. 设置 root element
  4. aceView->Launch() — 启动视图
  5. Frontend 附加 pipeline context
```

### RSSurfaceNode 创建

```cpp
// window_view_jni.cpp
SurfaceCreated(jobject surface):
  nativeWindow = ANativeWindow_fromSurface(surface)
  Window::CreateSurfaceNode(nativeWindow)
    → RSSurfaceNode::Create(nativeWindow)
```

## Surface 生命周期

### SurfaceCreated

```
SurfaceView.surfaceCreated (Java)
  → JNI: WindowViewJni::SurfaceCreated
    → ANativeWindow_fromSurface(surface)
    → window->CreateSurfaceNode(nativeWindow)
      → RSSurfaceNode 创建
```

### SurfaceChanged

```
SurfaceView.surfaceChanged (Java)
  → JNI: WindowViewJni::SurfaceChanged
    → window->NotifySurfaceChanged(width, height, density)
      → ViewportConfig 更新
      → uiContent->UpdateViewportConfig(config, RESIZE)
        → AceContainerSG 更新 PipelineContext root size
        → 触发重绘
```

### SurfaceDestroyed

```
SurfaceView.surfaceDestroyed (Java)
  → JNI: WindowViewJni::SurfaceDestroyed
    → window->NotifySurfaceDestroyed()
      → RSSurfaceNode 释放
```

## 渲染后端配置

### Skia

- Skia 静态链接到 `libarkui_android.so` (~95MB 的一部分)
- 使用 `SkFontMgr` 管理字体 (app_main.cpp 引用)
- 字体从 `assets/arkui-x/systemres/fonts/` (9.7MB) 加载

### 平台定义

```cpp
// ace_container_sg.cpp:651
#ifdef ENABLE_ROSEN_BACKEND
    // 使用 RosenWindow (RSSurfaceNode)
#else
    // 使用替代后端
#endif
```

Android 上 `ENABLE_ROSEN_BACKEND` 被定义，使用 Virtual Rosen。

## UIContent 实现

### UIContentImpl

**文件**: `ui_content_impl.cpp`

```cpp
UIContentImpl::Initialize(window, contentInfo, storage):
  1. 创建 AceViewSG::CreateView(instanceId) — 平台视图
  2. 创建 AceContainerSG — 容器
  3. 关联 View 和 Container
  4. 设置 ContentEventCallback (页面生命周期)
```

## 事件分发

### 触摸事件

```
Android MotionEvent
  → WindowViewAospSurface.dispatchTouchEvent()
    → JNI: nativeDispatchPointerDataPacket
      → PipelineContext::DispatchPointerDataPacket
```

### 键盘事件

```
Android KeyEvent
  → WindowViewAospSurface.dispatchKeyEvent()
    → JNI: nativeDispatchKeyEvent
      → PipelineContext::DispatchKeyEvent
```

### 焦点事件

```
onWindowFocusChanged
  → JNI: nativeOnWindowFocusChanged
    → Window::NotifyFocusChanged
```

## 渲染管线对比

| 方面 | OHOS 原生 | ArkUI-X Android |
|------|----------|-----------------|
| 窗口管理 | Rosen WindowManager Service | Virtual Rosen (本地) |
| Surface | OHOS Surface (BufferQueue) | Android SurfaceView (ANativeWindow) |
| VSync | Rosen VSync | Virtual RSInterfaces VSync |
| 渲染引擎 | Skia | Skia (同) |
| 声明式管道 | DeclarativeFrontendNG | DeclarativeFrontendNG (同) |
| PipelineContext | NG::PipelineContext | NG::PipelineContext (同) |
