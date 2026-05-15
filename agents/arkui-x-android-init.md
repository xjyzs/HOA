# ArkUI-X Android 运行时初始化流程

## 完整初始化序列: Activity.onCreate() → 首帧渲染

### Phase 0: 库加载 (JNI_OnLoad)

**文件**: `/src/arkui-x/foundation/arkui/ace_engine/adapter/android/entrance/java/jni/jni_load.cpp`

`System.loadLibrary("arkui_android")` 触发 JNI_OnLoad:
1. 初始化 `JniEnvironment` (包装 JavaVM 指针)
2. 注册 `JniAppModeConfig`

主动能 JNI 注册（StageActivityDelegateJni, WindowViewJni 等）通过静态初始化块懒惰注册。

### Phase 1: Application.onCreate()

**文件**: `StageApplication.java` → `StageApplicationDelegate.java`

`initApplication()` 执行（按顺序）:

1. **加载 libarkui_android.so** (两种策略):
   - 动态更新: 先尝试从 `filesDir/arkui-x/libs/arm64-v8a/` 加载
   - 正常: `System.loadLibrary("arkui_android")`
2. **设置 JNI Logger**: `nativeSetLogger` / `nativeSetLogLevel`
3. **附加 StageApplicationDelegate**: `nativeAttachStageApplicationDelegate(this)` — 将 Java 委托对象注册到 native `ApplicationContextAdapter`
4. **设置 PID/UID**: `nativeSetPidAndUid(pid, uid)`
5. **设置 HAP 路径**: `nativeSetHapPath(context.getPackageCodePath())` — 传递 APK 路径
6. **设置 AssetManager**: `nativeSetAssetManager(stageApplication.getAssets())` — 传递给 `StageAssetProvider`
7. **设置 native lib 目录**: `nativeSetAppLibDir(context.getApplicationInfo().nativeLibraryDir)`
8. **提取 stub.an**: 从 `assets/arkui-x/stub/{arch}/stub.an` → filesDir (ArkVM AOT stub)
9. **创建目录**: temp/, files/, preference/, database/, arkui-x/
10. **复制模块资源**: 将 `assets/arkui-x/systemres/` 和 `assets/arkui-x/{module}/resources/` + `resources.index` → `filesDir/arkui-x/`
    - **注意**: `.abc` 文件**不复制**，直接从 APK assets 通过 AssetManager 读取
11. **设置 locale**
12. **触发 native 初始化**: `nativeLaunchApplication` 传递 `isCopyNativeLibs` 和 `shouldLoadUI` 标志
13. **注册 ActivityLifecycleCallbacks**: 前后台跟踪

### Phase 2: ScheduleLaunchApplication (C++)

**文件**: `/src/arkui-x/foundation/appframework/ability/ability_runtime/cross_platform/frameworks/native/app/app_main.cpp`

`LaunchApplication` JNI handler 发送到事件处理器 → `ScheduleLaunchApplication`:

1. **加载 ICU 数据**: `SetArkuiXIcuDirectory()` → `filesDir/arkui-x/systemres/`
2. **创建 BundleContainer**: 从 APK assets 读取所有 `module.json` 解析模块/包元数据
3. **复制 native libs** (ANDROID_PLATFORM): 动态模块加载用
4. **创建 Application**: 设置 `ApplicationContext`，调用 `ParseBundleComplete()`:
   - 调用 `CreateRuntime()` 实例化 JsRuntime + EcmaVM
   - 设置 runtime 到 Application 对象

### Phase 3: JsRuntime / EcmaVM 创建

**文件**: `/src/arkui-x/foundation/appframework/ability/ability_runtime/cross_platform/frameworks/native/jsruntime/src/runtime.cpp`

`ArkJsRuntime::Initialize`:

1. **配置 panda::RuntimeOption**:
   - GC: GEN_GC, 256MB pool, 7 线程
   - Android: 加载 stubFilePath AOT stub, 启用 asm interpreter
   - 启用 AOT
2. **创建 EcmaVM**: `panda::JSNApi::CreateJSVM(pandaOption)`
3. **创建 ArkNativeEngine**: 包装 VM 为 napi_env
4. **设置原生库路径**: 注册 `appLibPath` + `appDataLibPath` 给 `NativeModuleManager`
5. **设置 bundle 信息到 VM**:
   - `JSNApi::SetBundle(vm_, false)` — ES Module 模式
   - `JSNApi::SetBundleName(vm_, bundleName)`
   - `JSNApi::SetHostResolveBufferTracker(vm_, JsModuleReader(...))` — 跨模块解析回调
   - 设置 pkgContextInfo, pkgAlias, pkgName 列表
6. **预加载 ACE 模块**: 如果 options.loadAce，调用 `DeclarativeModulePreloader::Preload()` 加载 etsstdlib.abc

### Phase 4: Activity.onCreate() → WindowView

**文件**: `StageActivity.java`

```
StageActivity.onCreate():
  1. activityDelegate.attachStageActivity(instanceName, this)
  2. windowView = WindowViewBuilder.makeWindowViewAosp(this, instanceId, isUseSurfaceView())
  3. initPlatformPlugin(this, instanceId, windowView)
  4. initArkUIXPluginRegistry()
  5. setContentView(windowView.getView())
  6. activityDelegate.dispatchOnCreate(instanceName, params)
```

- `isUseSurfaceView()` 默认返回 `true` → 创建 `WindowViewAospSurface` (继承 SurfaceView)
- `attachStageActivity` 调用 `nativeAttachStageActivity(instanceName, object)` — 存储 Java StageActivity 引用

### Phase 5: dispatchOnCreate → HandleAbilityStage

**文件**: `stage_activity_delegate_jni.cpp` → `app_main.cpp`

`nativeDispatchOnCreate` → `AppMain::DispatchOnCreate` → 事件处理器 → `HandleDispatchOnCreate`:

1. 解析 instanceName (`bundleName:moduleName:abilityName:instanceId`) → Want 对象
2. 加载 ACE (如果未预加载)
3. Android: 复制 HSP 资源，设置 native lib 路径
4. **调用 `application_->HandleAbilityStage(want)`**

### Phase 6: LaunchAbility → Window::SetUIContent

**文件**: `application.cpp` → `ability_stage.cpp`

`HandleAbilityStage`:
1. 从 BundleContainer 获取 hapModuleInfo
2. 创建 AbilityStageContext
3. `AbilityStage::Create(runtime)` → `JsAbilityStage::Create`
4. `abilityStage->OnCreate()` → `abilityStage->LaunchAbility(want, runtime)`

`LaunchAbility`:
1. 获取 abilityInfo，设置 `hapPath = "arkui-x/" + moduleName + "/"`
2. `Ability::Create(runtime)` → `JsAbility::Create`
3. `ability->OnCreate(want)` → 最终调用 `Window::SetUIContent()`

`Window::SetUIContent` (`virtual_rs_window.cpp`):
```cpp
uiContent = UIContent::Create(context_.get(), engine);
uiContent->Initialize(this, contentInfo, storage);
```

### Phase 7: 渲染管线设置

**文件**: `ace_container_sg.cpp`

`AceContainerSG::SetView`:
1. 初始化 frontend (DeclarativeFrontendNG for ETS)
2. 创建 `NG::PipelineContext`
3. 设置 root element
4. `aceView->Launch()`
5. Frontend 附加 pipeline context

### Phase 8: Surface 变化 → 首帧

当 SurfaceView surface 创建:
- `SurfaceCreated` callback → `WindowViewJni::SurfaceCreated` 获取 `ANativeWindow`
- `Window::CreateSurfaceNode(nativeWindow)` → `RSSurfaceNode`
- `surfaceChanged` → `Window::NotifySurfaceChanged(width, height, density)` → `UIContent::UpdateViewportConfig`
- DeclarativeFrontend JS 代码执行 → 首帧渲染

## Instance Name 约定

格式: `bundleName:moduleName:abilityName:instanceId`

解析位于 `app_main.cpp` TransformToWant:
- `bundleName` = nameStrs[0]
- `moduleName` = nameStrs[1]
- `abilityName` = nameStrs[2]
- `ABILITY_ID` = nameStrs[3]
