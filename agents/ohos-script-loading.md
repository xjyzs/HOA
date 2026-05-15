# OpenHarmony HAP 加载流程分析

## 概述

OpenHarmony 原生执行 HAP 中的 ETS 字节码并非简单地调用 `ExecuteModule("_GLOBAL::main")`。
实际流程涉及多个层次：appspawn 进程孵化 → ability_runtime 框架初始化 → JsRuntime 创建 VM
→ arkcompiler_ets_runtime (napi/ecmascript 层) 加载和执行模块。

源码位置: `/src/ohos`

## 1. 完整调用链 (8 阶段)

### 阶段 1: AppSpawn 进程孵化

```
MainThread::HandleLaunchApplication
  ↓ appkit/app/main_thread.cpp:1670
MainThread::PreloadModule
  ↓ appkit/app/main_thread.cpp:1670
MainThread::ProcessMainAbility
  ↓ appkit/app/main_thread.cpp:1828
  └→ 获取 compileMode, srcEntrance
  └→ runtime->PreloadMainAbility(moduleName, srcPath, hapPath, isEsmode, srcEntrance)
```

### 阶段 2: JsRuntime 创建 EcmaVM

```
JsRuntime::Initialize
  ↓ js_runtime.cpp:689
  ├→ CreateJsEnv(options) → 创建 VM
  │   └→ JsEnvironment::Initialize
  │        └→ JSNApi::CreateJSVM(pandaOption)  → 创建 EcmaVM
  │        └→ engine_ = new ArkNativeEngine(vm_, jsEngine)
  │            js_environment.cpp:59
  ├→ JSNApi::SetBundle(vm, false)            // 非 Bundle 模式 (合并 ABC)
  ├→ JSNApi::SetBundleName(vm, bundleName)
  ├→ JSNApi::SetModuleName(vm, moduleName)
  └→ JSNApi::SetHostResolveBufferTracker(vm, JsModuleReader(...))
       └→ 注册 HSP 跨模块解析回调
```

### 阶段 3: 设置模块信息和资源路径

```
JsRuntime::RunScript
  ↓ js_runtime.cpp:1176
  └→ path = "/data/storage/el1/bundle/" + moduleName + "/ets/modules.abc"
  └→ JSNApi::SetAssetPath(vm, path)
  └→ JSNApi::SetModuleName(vm, moduleName)
```

### 阶段 4: JsModuleReader 读取模块

```
JsModuleReader (js_module_reader.cpp)
  ├→ 从 HAP (ZIP) 中读取 ets/modules.abc
  ├→ 支持安全内存 (safe memory) 和普通 buffer
  └→ MERGE_ABC_PATH = "ets/modules.abc"
```

### 阶段 5: 加载模块 — 双路径分流

**使用场景区分**: OhmUrl (Path A) 主要用于声明式引擎内部的页面路由（如 `windowStage.loadContent('pages/Index')` 时，`jsi_declarative_engine.cpp:303` 构造 `@bundle:{bundleName}/{moduleName}/ets/{pagePath}`），而非初始 ability 加载。初始 ability 加载走的是 Path B，因为 module.json5 中的 `srcEntry` 是文件路径格式（如 `./ets/entryability/EntryAbility.ets`），`IsOhmUrl()` 对其返回 false。

```
JsRuntime::LoadModule(moduleName, srcPath, hapPath, isEsMode, ..., srcEntrance)
  ↓ js_runtime.cpp:1025
  ├── 检查 srcEntrance 是否为 OhmUrl ────────────────────┐
  │   是: panda::JSNApi::IsOhmUrl(srcEntrance) == true   否
  │   ↓                                                  ↓
  │ [Path A: OhmUrl]                            [Path B: 文件路径]
  │ (页面路由,                            (ability 初始加载,
  │  @bundle: 前缀)                        模块文件路径)
  │
  ├── Path A ──────────────────────────
  │   LoadScript(srcPath, buffer, len, isBundle, srcEntrance)
  │     ↓ js_runtime.cpp:575-585
  │   panda::JSNApi::ExecuteSecureWithOhmUrl(
  │       vm, buffer, len, srcFilename, srcEntrance)
  │     ↓ jsnapi_expo.cpp:5669-5699
  │   CheckAndGetRecordName(oHmUrl)  →  OhmUrl → record 名
  │     ↓
  │   执行模块
  │
  └── Path B ──────────────────────────
      LoadScript(srcPath, buffer, len, isBundle)
        ↓ js_runtime.cpp:587
      jsEnv_->LoadScript(path, buffer, len, isBundle)
        ↓
      ArkNativeEngine::RunScriptBuffer(path, buffer, isBundle)
        ↓ ark_native_engine.cpp:3323-3345
      [非 bundle 模式 (esmodule):]
        JSNApi::ExecuteModuleBufferSecure(vm_, buffer, size, path)
        ↓ jsnapi_expo.cpp:5722-5738
      [bundle 模式:]
        JSNApi::ExecuteSecure(vm_, buffer, size, "_GLOBAL::func_main_0", path)
```

### 阶段 6: ExecuteModuleBufferSecure → ExecuteModuleBuffer

```
JSNApi::ExecuteModuleBufferSecure(vm, buffer, size, filename)
  ↓ jsnapi_expo.cpp:5722
JSPandaFileExecutor::ExecuteModuleBuffer(thread, data, size, filename, ...)
  ↓ js_pandafile_executor.cpp:150-195
  ├── GetAssetPath(vm) → baseFileName
  │     (在 OHOS: /data/storage/el1/bundle/{moduleName}/ets/modules.abc)
  ├── NormalizePath(filename) → normalName
  ├── ParseAbcPathAndOhmUrl(vm, normalName, baseFileName, entry)
  │     (在 OHOS: 进入 Branch 1 — 绝对路径处理)
  ├── LoadJSPandaFile(thread, baseFileName, entry, buffer, size)
  ├── CheckIsRecordWithBundleName(entry)
  ├── AdaptOldIsaRecord(entry) — 如果 record 不含 bundleName
  ├── CheckAndGetRecordInfo(entry) → JSRecordInfo*
  └── CommonExecuteBuffer → ModuleResolver → Instantiate → Evaluate
```

### 阶段 7: 入口点格式

**Bundle 模式 (旧 CommonJS 格式)**:
- 入口点常量: `_GLOBAL::func_main_0`
- 定义位置: `native_engine.cpp:32`, `ark_native_engine.cpp:99`, `module_path_helper.h:97`
- 用途: 旧版 CommonJS 格式的 ABC

**合并 ABC / ES 模块模式 (当前主流)**:
- 入口点有两种形式，对应不同场景：
  
  **A. 文件路径入口 (ability 初始加载)**:
  - 格式: 模块记录名（如 `entry/src/main/ets/entryability/EntryAbility`）
  - 来源: `ProcessMainAbility` 从 module.json5 的 `srcEntry` 构造，经 `ParseAbcPathAndOhmUrl` 路由转换
  - 使用: `ExecuteModuleBuffer` / `ExecuteModuleBufferSecure`

  **B. OhmUrl 入口 (声明式引擎页面路由)**:
  - 格式: `@bundle:{bundleName}/{moduleName}/ets/{pagePath}`
  - 构建: `jsi_declarative_engine.cpp:303`
    ```cpp
    "@bundle:" + bundleName + "/" + moduleName + "/ets/" + pagePath
    ```
  - 使用: `ExecuteSecureWithOhmUrl` (Path A)，通过 `CheckAndGetRecordName()` 将 OhmUrl 转为 record 名

### 阶段 8: @ohos.* 和 @system.* 模块解析

模块前缀 (`module_path_helper.h:73-83`):

| 前缀 | 用途 | 解析方式 |
|------|------|---------|
| `@bundle:` | 应用内/跨应用 OHOS 模块 | `ParsePrefixBundle` |
| `@package:` | Ohpm 第三方包 | 返回包路径 |
| `@ohos:` | OHOS NAPI 系统模块 | `CheckNativeModule` → OHOS_MODULE |
| `@native:` | 系统内建模块 | NATIVE_MODULE |
| `@app:` | 应用级 NAPI 原生模块 (.so) | APP_MODULE |

`CheckNativeModule` (`js_module_source_text.cpp:296-321`):
- `@ohos:xxx` → OHOS_MODULE (如 `@ohos:hilog`, `@ohos:router`)
- `@app:xxx` → APP_MODULE (应用原生 .so)
- `@native:xxx.xxx` → NATIVE_MODULE (系统内建)

`GetRequireNativeModuleFunc` 从全局对象获取 `requireNapi` 函数，该函数是解析所有 `@ohos:` 和 `@system:` 导入的 NAPI 桥接。

`ConcatFileNameWithMerge` (`module_path_helper.cpp:29-34`):
- `@bundle:` → `ParsePrefixBundle` (解析跨模块引用)
- `@package:` → 返回包路径
- `ets/` → `MakeNewRecord` (ETS 相对引用)
- 其他 → `ParseThirdPartyPackage`

## 2. ParseAbcPathAndOhmUrl 三路分支

```cpp
void ModulePathHelper::ParseAbcPathAndOhmUrl(
    EcmaVM *vm, const CString &inputFileName,
    CString &outBaseFileName, CString &outEntryPoint)
{
    // module_path_helper.cpp:163-198
    
    if (inputFileName 以 "/data/storage/el1/bundle/" 开头) {
        // Branch 1: OHOS 设备绝对路径
        // 示例输入: /data/storage/el1/bundle/com.example.module/ets/pages/Index.abc
        // 提取 moduleName → 构造 baseFileName
        // outBaseFileName = /data/storage/el1/bundle/{moduleName}/ets/modules.abc
        // outEntryPoint = {bundleName}/{moduleName}/ets/pages/Index
    }
    else if (inputFileName 以 "@bundle:" 开头) {
        // Branch 2: OhmUrl 格式
        // 示例输入: @bundle:bundleName/moduleName/ets/pages/Index.abc
        // outEntryPoint = "bundleName/moduleName/ets/pages/Index" (去除前缀)
        // outBaseFileName = ParseUrl(vm, outEntryPoint)
    }
    else {
        // Branch 3: 相对路径 (非 OHOS 平台的默认分支)
        // 示例输入: moduleName/ets/xxx/xxx.abc
        // outEntryPoint = GetOutEntryPoint(vm, inputFileName)
        //   = "{bundleName}/{inputFileName}"
        //   = "bundleName/moduleName/ets/xxx/xxx"
    }
}
```

**OHOS 设备上通常进入 Branch 1**，因为 `inputFileName` 是 `/data/storage/el1/bundle/` 路径。
**OhmUrl 分流入 Branch 2**，通过 `@bundle:` 前缀识别并通过 `ExecuteSecureWithOhmUrl` 单独处理。
**Branch 3 在 OHOS 上极少触发**，主要由非 OHOS 平台（ArkUI-X）使用。

## 3. GetOutEntryPoint 的跨平台差异

### Common (非 OHOS) 实现

```cpp
// platform/common/module.cpp:21-25
CString GetOutEntryPoint(EcmaVM *vm, const CString &inputFileName) {
    return ConcatToCString(vm->GetBundleName(), "/", inputFileName);
    // 输入: "entry/ets/modules.abc"
    // 输出: "app.hackeris.example/entry/ets/modules.abc"
}
```

### OHOS 设备实现

```cpp
// platform/unix/ohos/module.cpp
// Branch 1 已处理 OHOS 路径, Branch 3 极少触发
```

## 4. OHOS 设备上的关键路径

| 路径 | 用途 |
|------|------|
| `/data/storage/el1/bundle/{moduleName}/ets/modules.abc` | 安装后的合并 ABC |
| `/data/storage/ark-cache/` | JIT/AOT 缓存 |
| `/data/service/el1/public/for-all-app/framework_ark_cache/` | 系统框架 AOT 缓存 (含 etsstdlib) |
| `/system/etc/system_kits_config.json` | @ohos: 模块映射配置 |

## 5. OHOS HAP 内部 ABC 结构

从 OHOS 设备生成的 HAP (`app.hackeris.harmonyexample`):

| 属性 | 值 |
|------|-----|
| 格式 | PANDA (合并 ABC) |
| 版本 | 0.1.0.13 |
| compileMode | esmodule (ETS ES 模块) |
| 类总数 | 13 |
| 内部类 (非外部) | ~3-4 |
| 外部类 (@ohos/@system) | ~8-9 |
| 模块记录名 | `entry/src/main/ets/entryability/EntryAbility` |

### 模块记录名格式

OHOS HAP 的模块记录名使用**源文件路径**格式:
```
entry/src/main/ets/entryability/EntryAbility
entry/src/main/ets/entrybackupability/EntryBackupAbility
entry/src/main/ets/pages/Index
```

这与 ArkUI-X 的 `{bundleName}/entry/ets/...` 格式**完全不同**。

### 外部类依赖

HAP 中的外部类引用了 OHOS 系统模块:
```
L@ohos.app;
L@ohos.curves;
L@ohos.matrix4;
L@system.app;
@ohos:app.ability.UIAbility
@ohos:hilog
@ohos:application.BackupExtensionAbility
```

这些在 OHOS 设备上由系统 ABC 和 NAPI 模块提供，在非 OHOS 平台上需要 mock/stub。

## 6. AdaptOldIsaRecord 的旧格式适配

`AdaptOldIsaRecord` (`base/path_helper.h:71-80`):

```cpp
// 旧 ISA 格式: 删除前两段 "/" 分隔的路径
// 输入: "bundleName/moduleName/ets/xxx/xxx"
// 输出: "ets/xxx/xxx"
```

此函数在 `ExecuteModuleBuffer` 中被调用，当 `CheckIsRecordWithBundleName` 发现 record 不含 bundleName 时触发。这意味着:
- 含 bundleName 的 entryPoint (新格式) → 直接查找
- 不含 bundleName 的 entryPoint (旧格式) → 删除前两段后再查找

## 7. OHOS NAPI 模块解析

### 系统模块映射

`system_kits_config.json` 定义 `@ohos:*` 到 `.so` 的映射:
```json
{
  "system_kits_config": [
    {
      "name": "hilog",
      "so_name": "libhilog.z.so"
    }
  ]
}
```

### NAPI 模块加载

```cpp
// NativeModuleManager::LoadNativeModule
// → dlopen("lib{module}*.z.so" 或 "lib{module}.so")
// → 调用模块的 napi_module_register()
// → 注入到 JS 全局对象
```

在 Android (ArkUI-X) 上，模块 `.so` 文件已打包在 APK 的 `lib/` 目录中。

## 8. OHOS vs ArkUI-X 关键差异

| 方面 | OHOS 原生 | ArkUI-X (Android) |
|------|----------|-------------------|
| VM 创建 | `JSNApi::CreateJSVM` + `SetBundle` | 同 (共享核心) |
| Ability 初始加载入口 | Path B (`ParseAbcPathAndOhmUrl`) | 同 Path B (`ParseAbcPathAndOhmUrl`) |
| 页面路由入口 | Path A (`ExecuteSecureWithOhmUrl` + `CheckAndGetRecordName`) | **不支持** (Android 端声明式引擎走 Branch 3) |
| Asset 读取 | 文件系统 (`fopen`) | AssetManager (APK assets) |
| 模块路径 | `/data/storage/el1/bundle/{moduleName}/ets/modules.abc` | 相对路径 `arkui-x/{...}/ets/modules.abc` |
| `ParseAbcPathAndOhmUrl` 分支 | **Branch 1** (绝对路径) | **Branch 3** (`GetOutEntryPoint`) |
| 模块记录名 (ABC 内) | 源文件路径格式 (`entry/src/main/ets/...`) | OhmUrl 风格 (`bundleName/entry/ets/...`) |
| 记录名来源 | hvigor/es2abc (OHOS 构建系统，保留 `src/main/ets/` 前缀) | ArkUI-X 构建系统 (去掉 `src/main/`，添加 bundleName) |
| entryPoint 产生 | Branch 1 从绝对路径提取 moduleName | `bundleName/` + 相对路径 |
| @ohos: 模块 | 系统提供 NAPI .so | ArkUI-X APK 打包部分模块 |
| etsstdlib | AOT 预编译 (.an/.ai) | 合并到 etsstdlib.abc |

## 9. OHOS 路径中的关键函数索引

| 函数 | 文件 | 行号 | 作用 |
|------|------|------|------|
| `PreloadMainAbility` | `appkit/main_thread.cpp` | ~1670 | 入口: 预加载主能力 |
| `JsRuntime::Initialize` | `js_runtime.cpp` | 689 | 创建 EcmaVM, 设置 bundle/asset |
| `CreateJSVM` | `js_environment.cpp` | 59 | 创建 VM 实例 |
| `JsRuntime::LoadModule` | `js_runtime.cpp` | 1025 | 双路径分流 |
| `JsRuntime::LoadScript` | `js_runtime.cpp` | 568/575 | OhmUrl / 文件路径分流 |
| `ExecuteSecureWithOhmUrl` | `jsnapi_expo.cpp` | 5669 | Path A 执行入口 |
| `ExecuteModuleBufferSecure` | `jsnapi_expo.cpp` | 5722 | Path B 执行入口 |
| `ExecuteModuleBuffer` | `js_pandafile_executor.cpp` | 150 | 合并 ABC 执行 |
| `ParseAbcPathAndOhmUrl` | `module_path_helper.cpp` | 163 | 三路入口路由 |
| `ConcatFileNameWithMerge` | `module_path_helper.cpp` | 29 | 模块前缀分派 (@ohos:, @bundle: 等) |
| `CheckNativeModule` | `js_module_source_text.cpp` | 296 | 判断 NATIVE/OHOS/APP 模块类型 |
