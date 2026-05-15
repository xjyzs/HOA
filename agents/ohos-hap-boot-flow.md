# OpenHarmony HAP 启动流程（8 阶段）

## 概述

OHOS 原生执行 HAP 中的 ETS 字节码涉及多个层次，从 appspawn 进程孵化到 ability_runtime 框架初始化，再到 JsRuntime 创建 VM 和 arkcompiler 加载模块。

## 完整调用链 (8 阶段)

### 阶段 1: AppSpawn 进程孵化

AppSpawn 是类似 Android Zygote 的 prefork 模型。

**源码**: `/src/ohos/base/startup/appspawn/standard/`

1. `appspawn` 作为系统守护进程在 boot 时通过 init 启动
2. 收到 socket 消息 (包含 app process info, bundle name 等)
3. `appspawn` 调用 `fork()` 创建子进程
4. 子进程执行 `execv()` 加载实际 app 进程二进制
5. App 进程执行 `MainThread`，通过 IPC 与 AMS 通信

### 阶段 2: MainThread::HandleLaunchApplication

**文件**: `/src/ohos/foundation/ability/ability_runtime/frameworks/native/appkit/app/main_thread.cpp`

关键函数:

| 函数 | 行号 | 作用 |
|------|------|------|
| `ScheduleLaunchApplication` | 652 | 从 AMS 接收 IPC，post task |
| `HandleLaunchApplication` | **1300** | 主 app 初始化逻辑 |
| `InitCreate` | 943 | 创建 OHOSApplication, ContextDeal |
| `CreateResourceManager` | 1701 | 创建资源管理器 |

HandleLaunchApplication 流程:
1. 行 1304: 验证启动数据
2. 行 1319: `InitCreate()` — 创建 OHOSApplication, ContextDeal
3. 行 1336-1359: `GetBundleForLaunchApplication()` — 获取 ENTRY 模块 HapModuleInfo
4. 行 1381-1399: 加载 native 库 (.so)
5. 行 1410-1425: 创建 ContextImpl, ApplicationContext
6. 行 1462-1503: **创建 Runtime (EcmaVM)**:
   ```cpp
   options.bundleName = appInfo.bundleName;
   options.hapPath = hapPath;
   options.moduleName = moduleName;
   options.isBundle = (compileMode != ES_MODULE);
   auto runtime = AbilityRuntime::Runtime::Create(options);
   ```
7. 行 1640: `LoadAllExtensions(jsEngine)`
8. 行 1683-1722: 创建 ResourceManager, `InitResourceManager`
9. 行 1734: `PerformAppReady()` — 通知 AMS app 就绪
10. 行 1777-1778: 如果需要 `PreloadModule`，触发 `ProcessMainAbility` 和 `PreloadMainAbility`

### 阶段 3: JsRuntime::Initialize — VM 创建

**文件**: `/src/ohos/foundation/ability/ability_runtime/frameworks/native/runtime/js_runtime.cpp`

| 函数 | 行号 | 作用 |
|------|------|------|
| `JsRuntime::Create` | **168** | 创建 VM 实例 |
| `JsRuntime::Initialize` | **689** | 完整 VM 初始化 |
| `JsRuntime::CreateJsEnv` | **818** | 创建 JsEnvironment + EcmaVM |

Initialize 关键序列:
1. 行 698: `CreateJsEnv(options)` — 创建 EcmaVM
2. 行 722-743: 初始化 syscap module, 保存 `global.requireNapi` 引用, `PreloadAce`
3. 行 747-777:
   - `SetSearchHapPathTracker(vm, ...)` — HSP 路径查找回调
   - `SetBundle(vm, options.isBundle)` — 设置 bundle 模式
   - `SetBundleName(vm, options.bundleName)`
   - `SetHostResolveBufferTracker(vm, JsModuleReader(...))` — HSP 跨模块解析
   - `SetHmsModuleList` — 系统 kit 模块映射
4. 行 780-811: 初始化 ConsoleModule, SourceMap, TimerModule, WorkerModule, Loop

### 阶段 4: JsEnvironment — VM 包装器

**文件**: `/src/ohos/foundation/ability/ability_runtime/js_environment/frameworks/js_environment/src/js_environment.cpp`

```cpp
// 行 59
bool JsEnvironment::Initialize(const panda::RuntimeOption& pandaOption, void* jsEngine) {
    vm_ = panda::JSNApi::CreateJSVM(pandaOption);  // 创建 EcmaVM
    engine_ = new ArkNativeEngine(vm_, jsEngine);    // 包装为 ArkNativeEngine
}
```

### 阶段 5: PreloadModule 和 ProcessMainAbility

**文件**: `main_thread.cpp`

PreloadModule (行 1856-1872):
1. 调用 `application_->AddAbilityStage()` 初始化 AbilityStage
2. 遍历 entryHapModuleInfo.abilityInfos
3. 如果 `info.name == mainAbility`，调用 `ProcessMainAbility(info, runtime)`

ProcessMainAbility (行 1828-1854):
```cpp
srcPath.append("/").append(info.srcEntrance);
srcPath.erase(srcPath.rfind("."));
srcPath.append(".abc");
// 结果: "entry/ets/entryability/EntryAbility.abc"
runtime->PreloadMainAbility(moduleName, srcPath, info.hapPath, isEsmode, info.srcEntrance);
```

### 阶段 6: JsRuntime::LoadModule — 双路径分流

**文件**: `js_runtime.cpp`

LoadModule 流程 (行 1025):
1. 行 1037: `isOhmUrl_ = JSNApi::IsOhmUrl(srcEntrance)` — 检查是否为 OhmUrl
2. 行 1065: 分支选择:
   - ES Module: `LoadJsModule(fileName, hapPath, srcEntrance)`
   - Bundle: `LoadJsBundle(fileName, hapPath, useCommonChunk)`

RunScript (行 1117):
```cpp
path = "/data/storage/el1/bundle/" + moduleName + "/ets/modules.abc";
JSNApi::SetAssetPath(vm, path);
JSNApi::SetModuleName(vm, moduleName);
// 从 HAP (ZIP) 读取 ABC buffer
LoadScript(abcPath, buffer, len, isBundle_, srcEntrance);
```

LoadScript 双路径 (行 575):
```cpp
if (isOhmUrl_ && !moduleName_.empty()) {
    // Path A: OhmUrl
    JSNApi::ExecuteSecureWithOhmUrl(vm, buffer, len, srcFilename, srcEntrance);
} else {
    // Path B: 文件路径
    jsEnv_->LoadScript(path, buffer, len, isBundle);
    // → ArkNativeEngine::RunScriptBuffer
    // → JSNApi::ExecuteModuleBuffer / ExecuteModuleBufferSecure
}
```

### 阶段 7: ArkCompiler 核心 — 模块执行

**文件**: `/src/ohos/arkcompiler/ets_runtime/ecmascript/jspandafile/js_pandafile_executor.cpp`

ExecuteModuleBuffer (行 180):
1. 行 190: `name = vm->GetAssetPath()` — 获取预设 asset 路径
2. 行 199: `ParseAbcPathAndOhmUrl(vm, normalName, name, entry)` — **三路路由**
3. 行 200: `LoadJSPandaFile(thread, name, entry, buffer, size)` — 解析 ABC
4. 行 217-221: Record 名规范化:
   - `CheckIsRecordWithBundleName(entry)`
   - `AdaptOldIsaRecord(entry)` — 剥离旧格式前缀
5. 行 224: `CheckAndGetRecordInfo(entry)` → JSRecordInfo*
6. 行 232: `CommonExecuteBuffer(thread, name, entry, buffer, size)`

CommonExecuteBuffer:
```cpp
if (isBundle) {
    moduleRecord = moduleManager->HostResolveImportedModule(buffer, size, filename);
} else {
    moduleRecord = moduleManager->HostResolveImportedModuleWithMerge(filename, entry);
}
SourceTextModule::Instantiate(thread, moduleRecord);
SourceTextModule::Evaluate(thread, module, buffer, size);
```

### 阶段 8: JsUIAbility → ArkUI 渲染

**文件**: `/src/ohos/foundation/ability/ability_runtime/frameworks/native/ability/native/ability_runtime/js_ui_ability.cpp`

| 函数 | 行号 | 作用 |
|------|------|------|
| `Init` | **170** | 使用 srcEntrance 创建 ability |
| `UpdateAbilityObj` | 217 | 调用 LoadModule 或复用预加载模块 |
| `OnStart` | **300** | 调用 JS `onCreate()` 生命周期 |
| `OnSceneCreated` | 500 | 调用 JS `onWindowStageCreate()` |

## 文件路径约定

| 路径 | 用途 |
|------|------|
| `/data/storage/el1/bundle/{moduleName}/ets/modules.abc` | 安装后的合并 ABC |
| `/data/storage/ark-cache/` | JIT/AOT 缓存 |
| `/data/service/el1/public/for-all-app/framework_ark_cache/` | 系统框架 AOT 缓存 |
| `/system/etc/system_kits_config.json` | @ohos: 模块映射配置 |
