# JNI 桥接层分析

## 概述

ArkUI-X Android 的 Java 到 native 桥接分为**两个 JNI 渠道**，对应两个 Java delegate 类。

| JNI 注册 | Java 类 | C++ 注册文件 | 职责 |
|----------|--------|-------------|------|
| StageApplicationDelegate | `ohos.stage.ability.adapter.StageApplicationDelegate` | `stage_application_delegate_jni.cpp` | 应用初始化、资源路径、模块加载 |
| StageActivityDelegate | `ohos.stage.ability.adapter.StageActivityDelegate` | `stage_activity_delegate_jni.cpp` | 生命周期分发、WindowView 绑定 |

---

## 一、启动与初始化序列

```
Android Application.onCreate()
  └→ StageApplicationDelegate.initApplication(application)
       ├→ System.loadLibrary()  // 加载 libarkui_android.so
       ├→ nativeSetLogger / nativeSetLogLevel
       ├→ nativeAttachStageApplicationDelegate
       ├→ nativeSetPidAndUid
       ├→ nativeSetIsDynamicLoadLibs
       ├→ nativeSetHapPath(apkPath)
       ├→ nativeSetAssetManager          ★ 传递 Android AssetManager
       ├→ nativeSetAppLibDir
       ├→ nativeSetStubFilePath           // 提取 stub.an 用于 AOT
       ├→ copyAllModuleResources()        // 从 assets 拷贝资源到 filesDir
       ├→ nativeSetAssetsFileRelativePath
       ├→ nativeSetResourcesFilePrefixPath
       ├→ nativeLaunchApplication(true, shouldLoadUI)  ★ 核心
       ├→ nativeInitConfiguration
       └→ nativeSetPackageName
```

### nativeLaunchApplication → C++ 链

```
stage_application_delegate_jni.cpp:230
  LaunchApplication → AppMain::LaunchApplication(isCopyNativeLibs, shouldLoadUI)
    → [PostTask to eventHandler]
    → AppMain::ScheduleLaunchApplication()
```

**`ScheduleLaunchApplication`** (`app_main.cpp:93-142`) 的关键步骤:

1. `LoadIcuData()` — 加载 ICU 数据
2. `StageAssetManager::GetInstance()->GetModuleJsonBufferList()` — 从 assets 读取各模块的 module.json
3. `bundleContainer_->LoadBundleInfos(moduleList)` — 解析 module.json 列表
4. `ParseBundleComplete()` — 创建 Runtime

### ParseBundleComplete → CreateRuntime

```cpp
// app_main.cpp:261-262
CreateRuntime(applicationInfo->bundleName,
    bundleInfo->hapModuleInfos.back().compileMode != AppExecFwk::CompileMode::ES_MODULE);
```

传递到 `Runtime::Create(options)` (`js_runtime.cpp:204`) → 创建 `EcmaVM`，设置 `isBundle` 运行时标志。

---

## 二、渠道 A: StageApplication JNI（模块加载）

### 注册表

**文件**: `stage_application_delegate_jni.cpp:40-155`

| Java 方法 | native C++ 函数 | 用途 |
|-----------|----------------|------|
| `nativeSetAssetManager(AssetManager)` | `SetNativeAssetManager` | 传递 Android AssetManager |
| `nativeSetHapPath(String)` | `SetHapPath` | 设置 APK 路径 |
| `nativeLaunchApplication(boolean, boolean)` | `LaunchApplication` | 启动应用初始化 |
| `nativePreloadModule(String, String)` | `PreloadModule` | 预加载模块 |
| `nativeLoadModule(String, String)` | `LoadModule` | 加载模块(核心) |
| `nativeOnConfigurationChanged(String)` | `OnConfigurationChanged` | 配置变更 |
| `nativeDispatchApplicationOnForeground/Background()` | ... | 前后台切换 |

### LoadModule 调用链

```
Java: StageApplicationDelegate.loadModule("entry", "./ets/xxx.ets")
  → nativeLoadModule(moduleName, entryFile)
  → StageApplicationDelegateJni::LoadModule (line 431-455)
    → AppMain::LoadModule(moduleName, entryFile)
      → [PostTask] HandleLoadModule
        → LoadModuleHelper::LoadModule(runtime, hapModuleInfo, entryFile)
```

**`LoadModuleHelper::LoadModule`** (`load_module_helper.cpp:29-75`):

```cpp
// line 55-60 — 构造 modulePath
modulePath = hapModuleInfo->moduleName;   // "entry"
modulePath.append("/");
modulePath.append(entryPath);              // "entry/./ets/xxx.ets"
modulePath.erase(modulePath.rfind("."));   // "entry/./ets/xxx"
modulePath.append(".abc");                 // "entry/./ets/xxx.abc"

// line 42 — 仅支持 esmodule
bool esModule = hapModuleInfo->compileMode == AppExecFwk::CompileMode::ES_MODULE;
```

然后调用 `JsRuntime::LoadModule(moduleName, modulePath, buffer, entryPath, esModule)`。

### entryFile 格式约束

`StageApplicationDelegate.loadModule` 对 `entryFile` 有正则校验（line 118）:
```java
private static final String LOAD_MODULE_PATH_REGEX = "^\\./ets/([^/]+/)*[^/]+$";
```
即必须匹配 `./ets/xxx/xxx` 格式。`loadModule` 方法文档注释（line 1261）明确说明: `Example: "./ets/xxx.ets"`。

---

## 三、渠道 B: StageActivity JNI（生命周期分发）

### 注册表

**文件**: `stage_activity_delegate_jni.cpp:35-88`

| Java 方法 | native C++ 函数 | 触发时机 |
|-----------|----------------|---------|
| `nativeAttachStageActivity(String, StageActivity)` | `AttachStageActivity` | Activity 创建时 |
| `nativeDispatchOnCreate(String, String)` | `DispatchOnCreate` | Activity.onCreate() |
| `nativeDispatchOnDestroy(String)` | `DispatchOnDestroy` | Activity.onDestroy() |
| `nativeDispatchOnForeground(String)` | `DispatchOnForeground` | Activity 回到前台 |
| `nativeDispatchOnBackground(String)` | `DispatchOnBackground` | Activity 进入后台 |
| `nativeDispatchOnNewWant(String, String)` | `DispatchOnNewWant` | 新 Intent |
| `nativeSetWindowView(String, WindowViewInterface)` | `SetWindowView` | 绑定 SurfaceView |

### DispatchOnCreate 调用链（最核心路径）

```
Java: StageActivityDelegate.nativeDispatchOnCreate(instanceName, params)
  → StageActivityDelegateJni::DispatchOnCreate (line 122-136)
    → AppMain::DispatchOnCreate(instanceName, parameters)
      → [PostTask] HandleDispatchOnCreate (line 443-500)
```

**`HandleDispatchOnCreate`** 的关键流程:

```cpp
// line 450 — 解析 instanceName
auto want = TransformToWant(instanceName);
std::string moduleName = want.GetModuleName();
std::string bundleName = want.GetBundleName();

// line 484 — 自动加载未预载的 ACE 模块
if (runtime != nullptr && !static_cast<JsRuntime&>(*runtime).IsPreloadedAce()) {
    static_cast<JsRuntime&>(*runtime).LoadAce();
}

// line 499 — 进入 Ability 生命周期
application_->HandleAbilityStage(TransformToWant(instanceName, params));
```

### Application::HandleAbilityStage

**文件**: `application.cpp:66-121`

```
1. 查找已有 AbilityStage → 如存在直接 LaunchAbility
2. 如不存在 → 创建新的 AbilityStage:
   a. 创建 AbilityStageContext（设置 applicationContext、hapModuleInfo、resourceManager）
   b. AbilityStage::Create(runtime, hapModuleInfo)
   c. abilityStage->Init(context)
   d. abilityStage->OnCreate()
   e. abilityStage->LaunchAbility(want, runtime)
```

### AbilityStage::LaunchAbility → JsAbility

**文件**: `ability_stage.cpp:82-130`

```
1. 从 bundleContainer_ 获取 abilityInfo
2. 设置 hapPath = "arkui-x/" + moduleName + "/"
3. 创建 AbilityContextImpl（设置 stageContext、abilityInfo、instanceName）
4. ability = Ability::Create(runtime) → 返回 JsAbility
5. ability->Init(abilityInfo)   ★ ABC 加载入口
6. ability->OnCreate(want)      ★ JS onCreate() 触发
```

### JsAbility::Init — ABC 加载入口

**文件**: `js_ability.cpp:68-98`

```cpp
// line 86-96 — 构造 modulePath（ArkUI-X 版 GenerateSrcPath）
if (esmodule) {
    modulePath = abilityInfo->package;         // "entry"
    modulePath.append("/");
    modulePath.append(abilityInfo->srcEntrance); // "entry/./ets/entryability/EntryAbility.ets"
    modulePath.erase(modulePath.rfind("."));     // "entry/./ets/entryability/EntryAbility"
    modulePath.append(".abc");                   // "entry/./ets/entryability/EntryAbility.abc"
}
// line 97-98
jsAbilityObj_ = jsRuntime_.LoadModule(moduleName, modulePath, abilityBuffer,
    abilityInfo->srcEntrance, esmodule);
```

**注意**: 这是 ArkUI-X 的 `appframework` 跨平台实现，不是 OHOS 的 `foundation/ability/ability_runtime` 实现。核心区别:
- ArkUI-X: `modulePath` 是相对路径（`entry/./ets/...`），无 `/data/storage/el1/bundle/` 前缀
- OHOS: `modulePath` 是绝对路径（`/data/storage/el1/bundle/entry/ets/...`），命中 Branch 1

### JsRuntime::LoadModule → RunScriptBuffer

**文件**: `js_runtime.cpp:529-583`

```cpp
// 构造 fileName，去掉 "::" 后的部分，去掉 ./ 模式
std::string fileName;
fileName.append(moduleNamePath).append("/").append(srcEntrance); // "entry/./ets/.../xxx.ets"
fileName.erase(fileName.rfind("."));       // 去后缀
fileName.append(".abc");
std::regex pattern(std::string("\\.") + std::string("/"));
fileName = std::regex_replace(fileName, pattern, "");  // 去掉 "./" → "entry/ets/.../xxx.abc"

classValue = LoadJsModule(fileName, buffer, isDynamicUpdate);
```

**`LoadJsModule`** (`js_runtime.cpp:163-195`):

```cpp
std::string assetPath = BUNDLE_INSTALL_PATH + moduleName_ + MERGE_ABC_PATH;
panda::JSNApi::SetAssetPath(vm_, assetPath);
panda::JSNApi::SetModuleName(vm_, moduleName_);
// ★ 最终调用
bool result = engine->RunScriptBuffer(path.c_str(), buffer, false, needUpdate) != nullptr;
```

`RunScriptBuffer` 进入 ArkCompiler 层 → `ExecuteModuleBuffer` → `ParseAbcPathAndOhmUrl` → entryPoint 路由。

---

## 四、instanceName 格式

**格式**: `bundleName:moduleName:abilityName:instanceId`

**解析** (`app_main.cpp:584-602`):

```cpp
Want AppMain::TransformToWant(const std::string& instanceName, const std::string& params)
{
    std::vector<std::string> nameStrs;
    Ace::StringUtils::StringSplitter(instanceName, ':', nameStrs);
    // nameStrs[0] = bundleName
    // nameStrs[1] = moduleName
    // nameStrs[2] = abilityName
    // nameStrs[3] = abilityId (instance index)

    Want want;
    if (nameStrs.size() == 4) {
        want.SetBundleName(nameStrs[0]);
        want.SetModuleName(nameStrs[1]);
        want.SetAbilityName(nameStrs[2]);
        want.SetParam(Want::ABILITY_ID, nameStrs[3]);
        want.SetParam(Want::INSTANCE_NAME, instanceName);
    }
    want.ParseJson(params);
    return want;
}
```

**调用方**: `StageActivityDelegate.java` 在 `onCreate()` 中构造 instanceName 字符串传入。

---

## 五、关键参数转换链汇总

### 路径 1: HandleDispatchOnCreate（主路径）

```
module.json5:
  srcEntry: "./ets/entryability/EntryAbility.ets"
  package: "entry"

StageActivity.onCreate():
  instanceName: "app.hackeris.example:entry:EntryAbility:0"

JsAbility::Init (ArkUI-X):
  modulePath: "entry/./ets/entryability/EntryAbility.abc"

JsRuntime::LoadModule:
  fileName: "entry/ets/entryability/EntryAbility.abc"  (去掉 ./)

LoadJsModule → RunScriptBuffer:
  path = "entry/ets/entryability/EntryAbility.abc"
  → ParseAbcPathAndOhmUrl → Branch 3 (相对路径) ★
```

### 路径 2: LoadModule（直接加载路径）

```
Java:
  loadModule("entry", "./ets/xxx.ets")

LoadModuleHelper::LoadModule:
  modulePath: "entry/./ets/xxx.abc"

JsRuntime::LoadModule:
  fileName: "entry/ets/xxx.abc"
  → LoadJsModule → RunScriptBuffer
```

---

## 六、对于 OHOS HAP on Android 项目的影响

### 6.1 modulePath 构造差异

ArkUI-X 的 `JsAbility::Init` 构造的 modulePath 是相对路径。对于 OHOS HAP:
- 需要确保 `abilityInfo->package` 和 `abilityInfo->srcEntrance` 能从 HAP 的 module.json5 正确解析
- 或者直接绕过 `JsAbility::Init` 的 modulePath 构造，在调用 `LoadModule` 前自行构造 OHOS 格式的路径

### 6.2 两个可用的 JNI 入口

1. **`nativeLoadModule(moduleName, entryFile)`** — 适用于模块预加载场景
   - `entryFile` 格式必须匹配 `./ets/xxx.ets` 正则
   - 直接走到 `LoadModuleHelper::LoadModule` → `JsRuntime::LoadModule`

2. **`nativeDispatchOnCreate(instanceName, params)`** — 适用于完整 Ability 生命周期
   - `instanceName` 格式为 `bundleName:moduleName:abilityName:instanceId`
   - 走完整的 `HandleAbilityStage` → `AbilityStage::LaunchAbility` → `JsAbility::Init` → `LoadModule`

### 6.3 资源路径

ArkUI-X 的所有资源（module.json, ABC, resources.index）通过 Android `AssetManager` 提供:
- `nativeSetAssetManager(assetManager)` — 传递 Java AssetManager
- `copyAllModuleResources()` — 将 assets 文件复制到 `filesDir/arkui-x/`
- `nativeSetAssetsFileRelativePath()` — 设置相对路径用于资源定位

对于 OHOS HAP 场景，需要将 HAP (ZIP) 中的内容通过 AssetManager 或自定义文件路径暴露给 native 层，或者替换 `StageAssetProvider` 实现。

### 6.4 自定义运行时入口

`nativeLaunchApplication` → `ParseBundleComplete` → `CreateRuntime` 创建 EcmaVM 和 JsRuntime。如果 OHOS HAP 需要自定义初始化（如设置 `isOhosHapMode` 标志），可在 `CreateRuntime` 之后、`HandleAbilityStage` 之前的时机插入。

---

## 七、源码索引

| 文件 | 关键行 | 内容 |
|------|--------|------|
| `stage_application_delegate_jni.cpp` | 36-170 | JNI 方法注册表 |
| `stage_application_delegate_jni.cpp` | 230-235 | LaunchApplication JNI |
| `stage_application_delegate_jni.cpp` | 431-455 | LoadModule JNI |
| `stage_activity_delegate_jni.cpp` | 32-102 | JNI 方法注册表 |
| `stage_activity_delegate_jni.cpp` | 122-136 | DispatchOnCreate JNI |
| `app_main.cpp` | 93-142 | ScheduleLaunchApplication |
| `app_main.cpp` | 236-266 | ParseBundleComplete + CreateRuntime |
| `app_main.cpp` | 443-500 | HandleDispatchOnCreate |
| `app_main.cpp` | 584-602 | TransformToWant (instanceName 解析) |
| `app_main.cpp` | 851-872 | LoadModule → HandleLoadModule |
| `application.cpp` | 66-121 | HandleAbilityStage |
| `ability_stage.cpp` | 82-130 | LaunchAbility |
| `js_ability.cpp` | 68-98 | JsAbility::Init (modulePath 构造) |
| `js_ability.cpp` | 188-239 | JsAbility::OnCreate |
| `load_module_helper.cpp` | 29-75 | LoadModuleHelper::LoadModule |
| `js_runtime.cpp` | 529-583 | JsRuntime::LoadModule |
| `js_runtime.cpp` | 163-195 | LoadJsModule → RunScriptBuffer |
| `StageApplicationDelegate.java` | 220-301 | Java initApplication 流程 |
| `StageApplicationDelegate.java` | 1263-1277 | Java loadModule |
| `StageApplicationDelegate.java` | 118 | entryFile 正则约束 |
| `StageActivityDelegate.java` | — | nativeDispatchOnCreate 声明 |
