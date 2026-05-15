# 平台抽象层分析

## 概述

ArkUI-X 在 OpenHarmony 和 Android 之间通过多层抽象实现跨平台。分析这些抽象点是"在 Android 上运行 OHOS HAP"项目的关键。

## OHOS vs ArkUI-X Android 关键差异总表

| 方面 | OHOS 原生 | ArkUI-X (Android) |
|------|----------|-------------------|
| VM 创建 | `JSNApi::CreateJSVM` + `SetBundle` | 同 (共享核心) |
| 入口路由 | Path A (OhmUrl) 或 Path B | 仅 Path B |
| OhmUrl 处理 | `ExecuteSecureWithOhmUrl` + `CheckAndGetRecordName` | **不支持** |
| Asset 读取 | 文件系统 (`fopen`) | AssetManager (APK assets) |
| 模块路径 | `/data/storage/el1/bundle/{moduleName}/ets/modules.abc` | 相对路径 `arkui-x/{...}/ets/modules.abc` |
| `ParseAbcPathAndOhmUrl` | **Branch 1** (绝对路径) | **Branch 3** (GetOutEntryPoint) |
| 模块记录名 | 源文件路径 (`entry/src/main/ets/...`) | OhmUrl 风格 (`bundleName/entry/ets/...`) |
| entryPoint 产生 | 从绝对路径提取 | `bundleName/` + 相对路径 |
| @ohos: 模块 | 系统提供 NAPI .so | 打包子集 |
| etsstdlib | AOT 预编译 (.an/.ai) | 合并到 etsstdlib.abc |
| 进程模型 | AppSpawn fork | Android Application/Activity |

## 抽象层 1: Asset 管理

### 结构差异

| 平台 | 存储位置 | 访问方式 |
|------|---------|---------|
| OHOS | `/data/storage/el1/bundle/` (文件系统) | `fopen` 直接读写 |
| ArkUI-X Android | APK assets + filesDir | AssetManager + 文件系统混合 |

### AssetHelper (ArkUI-X)

**文件**: `js_worker.cpp:155-255`

```cpp
struct AssetHelper {
    void operator()(const std::string &fpath, uint8_t **buff, size_t *buffSize,
                    std::vector<uint8_t> &content, std::string &ami, ...) {
        // Android: ami = filePath (纯相对路径)
        // 其他平台: ami = codePath_ + filePath (绝对路径)
        // 从 AssetManager 读取 APK assets
    }
};
```

### StageAssetProvider (Android)

**文件**: `stage_asset_provider.cpp`

双路径 asset 解析:
1. **APK assets**: 通过 `AssetManager` 创建 `PackAssetProvider` (行 486)
2. **文件系统** (动态下载模块): 通过 `std::fopen` / `GetBufferByAppDataPath()` (行 632)

### SetGetAssetFunc 回调

```
NativeEngine::SetGetAssetFunc    (native_engine.cpp:596)   — 设置回调
NativeEngine::CallGetAssetFunc   (native_engine.cpp:631-635) — 调用回调
GetAbcBuffer                      (native_engine.cpp:1086-1099) — 读取 ABC
```

### 资源复制策略

`StageApplication.onCreate()` 中:
- 复制 `assets/arkui-x/systemres/` → `filesDir`
- 复制 `assets/arkui-x/{module}/resources/` → `filesDir`
- 复制 `resources.index` → `filesDir`
- **不复制** `.abc` 文件 — 通过 AssetManager 直接从 APK assets 读取

## 抽象层 2: GetOutEntryPoint 实现

### Common (非 OHOS)

**文件**: `platform/common/module.cpp:21-25`

```cpp
CString GetOutEntryPoint(EcmaVM *vm, const CString &inputFileName) {
    return ConcatToCString(vm->GetBundleName(), "/", inputFileName);
    // 输入: "entry/ets/modules.abc"
    // 输出: "app.hackeris.example/entry/ets/modules.abc"
}
```

### OHOS 设备

**文件**: `platform/unix/ohos/module.cpp`

Branch 1 处理 OHOS 路径，Branch 3 触发极少。

## 抽象层 3: 窗口/渲染

### 渲染后端

| 平台 | 后端 | Surface 提供 |
|------|------|-------------|
| OHOS | Rosen (RSSurfaceNode) | OHOS WindowManager |
| ArkUI-X Android | Virtual Rosen (RSSurfaceNode) | Android SurfaceView |

### Virtual Rosen Window

**文件**: `virtual_rs_window.cpp` / `virtual_rs_window.h`

- `Rosen::Window` 的虚拟实现，由 Android `SurfaceView` 支持
- `ANativeWindow_fromSurface()` 获取 native window 句柄 (window_view_jni.cpp)
- VSync 由 `RSInterfaces::CreateVSyncReceiver` 虚拟实现提供

### Surface 集成

```
SurfaceView.surfaceCreated
  → WindowViewJni::SurfaceCreated
    → ANativeWindow_fromSurface(surface)
    → Window::CreateSurfaceNode(nativeWindow)
      → RSSurfaceNode

SurfaceView.surfaceChanged
  → WindowViewJni::SurfaceChanged
    → Window::NotifySurfaceChanged(w, h, density)
      → UIContent::UpdateViewportConfig
```

### 渲染选项

`StageActivity.isUseSurfaceView()`:
- `true` (默认) → `WindowViewAospSurface` (SurfaceView)
- `false` → `WindowViewAospTexture` (TextureView)

## 抽象层 4: 文件系统

| OHOS 路径 | Android 等效 |
|-----------|-------------|
| `/data/storage/el1/bundle/` | `filesDir/` |
| `/data/storage/ark-cache/` | `cacheDir/` |
| temp | `filesDir/temp/` |
| preference | `filesDir/preference/` |
| database | `filesDir/database/` |

设置在 `StageApplicationDelegate.createStagePath()` (Java 端行 454)。

## 抽象层 5: 进程模型

### OHOS: AppSpawn

```
appspawn (系统守护进程)
  → fork() → 子进程
  → execv() → 加载 app 二进制
  → MainThread::HandleLaunchApplication
  → IPC 与 AMS 通信
```

### ArkUI-X Android

```
Android Activity 启动
  → StageApplication.onCreate()
  → StageActivity.onCreate()
  → JNI → AppMain (C++ 单例)
  → Application::HandleAbilityStage
  → Ability::OnCreate
```

## 抽象层 6: Module Path Resolution

`ParseAbcPathAndOhmUrl` 三路分支已在 `abc-path-routing.md` 中详述。
关键点: OHOS 走 Branch 1 (绝对路径), ArkUI-X Android 走 Branch 3 (相对路径 + GetOutEntryPoint)。

## 抽象层 7: 系统模块

| 方面 | OHOS | ArkUI-X Android |
|------|------|----------------|
| 系统 ABC | 系统分区预装 | APK assets/arkui-x/systemres/abc/ |
| NAPI .so | `/system/lib64/` | APK lib/ 目录 |
| 模块映射 | `/system/etc/system_kits_config.json` | 内建于 NativeModuleManager |
| etsstdlib | AOT .an/.ai 文件 | etsstdlib.abc (合并格式) |
