# ArkUI-X Android 构建系统分析

## 概述

ArkUI-X 将 OpenHarmony 核心运行时打包为单个巨型共享库 `libarkui_android.so`（arm64-v8a ~95MB），包含 ACE 引擎 + NAPI 桥接 + ETS 运行时 + Skia + ICU。

## 构建产物：APK 结构

```
app-release.apk
├── lib/arm64-v8a/
│   ├── libarkui_android.so          95MB   (ACE + NAPI + ETS运行时 + Skia + ICU)
│   ├── libarkui_componentsnapshot.so  540KB
│   ├── libarkui_focuscontroller.so    520KB
│   └── libhilog.so                    155KB
├── assets/arkui-x/
│   ├── stub/{arch}/stub.an                    (AOT stub for ArkVM)
│   ├── {bundleName}.{moduleName}/
│   │   ├── ets/modules.abc                   (应用字节码)
│   │   ├── module.json
│   │   ├── ark_module.json
│   │   ├── resources.index
│   │   └── resources/
│   └── systemres/
│       ├── abc/                              (7 个系统 ABC)
│       ├── icudt72l.dat                      (33.5MB ICU 数据)
│       ├── fonts/                            (9.7MB)
│       └── resources.index                   (1.2MB 系统资源)
└── classes.dex                               (StageActivity, StageApplication 等)
```

## libarkui_android.so 符号导出

`nm -D libarkui_android.so` 关键结果:

| 符号 | Visibility | 用途 |
|------|-----------|------|
| `NativeEngine::SetGetAssetFunc` | **exported** | 设置 Asset 回调 |
| `NativeEngine::SetModuleName` | **exported** | 设置模块名 |
| `NativeEngine::SetModuleFileName` | **exported** | 设置模块文件名 |
| `NativeEngine::Init` | **exported** | 初始化引擎 |
| `NativeEngine::RunScript` | **exported** | 执行脚本 |
| `NativeEngine::RunScriptForAbc` | **exported** | 执行 ABC (限制 Worker) |
| `NativeEngine::RunScriptBuffer` | **exported** | 通过 buffer 执行 |
| `JSNApi::Execute` | **exported** | JS API 执行 |
| `JSNApi::ExecuteModuleBuffer` | **hidden** | 模块 buffer 执行 |
| `JSNApi::ExecuteModuleBufferSecure` | **hidden** | 安全模块 buffer 执行 |
| `JSNApi::ExecuteSecureWithOhmUrl` | **hidden** | OhmUrl 安全执行 |

**关键**: `ExecuteModuleBuffer` 是 `libarkui_android.so` **内部链接**的，ArkUI-X 内部使用可直接链接调用，但外部调用者（通过 dlsym）无法访问。

## 关键源码路径

| 组件 | 路径 |
|------|------|
| 构建脚本 | `/src/arkui-x/build.sh` |
| Android 平台适配 | `/src/arkui-x/foundation/arkui/ace_engine/adapter/android/` |
| Stage Java 层 | `.../adapter/android/stage/ability/java/src/` |
| Stage JNI 层 | `.../adapter/android/stage/ability/java/jni/` |
| ACE Android 入口 | `.../adapter/android/entrance/java/` |
| AppFramework (跨平台) | `/src/arkui-x/foundation/appframework/` |
| JsRuntime (跨平台) | `/src/arkui-x/foundation/appframework/ability/ability_runtime/cross_platform/frameworks/native/jsruntime/` |
| OHOS 原始实现 | `/src/ohos/foundation/ability/ability_runtime/` |
| ArkCompiler | `/src/ohos/arkcompiler/ets_runtime/` |

## 编译内容

### ACE 引擎 (ArkUI)
- `/src/arkui-x/foundation/arkui/ace_engine/` — 声明式 UI 框架核心
- Android 适配层：`adapter/android/` 包含 StageActivity、WindowView、SurfaceView 渲染

### NAPI 桥接
- `@ohos:*` 系统模块的 NAPI 实现
- `@app:*` 应用原生模块加载
- `NativeModuleManager::LoadNativeModule` — dlopen + napi_module_register

### ETS 运行时 (arkcompiler_ets_runtime)
- EcmaVM 创建 (`JSNApi::CreateJSVM`)
- Panda 文件解析 (`JSPandaFileExecutor`)
- 模块路由 (`ModulePathHelper`)
- ES 模块加载 (`SourceTextModule::Instantiate/Evaluate`)

### 第三方依赖
- **Skia**: 2D 图形渲染 (静态链接到 libarkui_android.so)
- **ICU**: 国际化 (icudt72l.dat 33.5MB，作为 APK asset)
- **字体**: 9.7MB 系统字体

## 构建特点

1. **单体 .so 设计**: 所有核心功能编译到一个共享库，避免模块加载复杂性
2. **Android NDK 交叉编译**: 使用 GN/CMake 通过 NDK 构建 arm64-v8a 目标
3. **Asset 打包**: 系统资源、ABC 文件、ICU 数据作为 APK assets 分发
4. **动态更新支持**: 可以从 `filesDir` 加载更新版本的 `libarkui_android.so`
