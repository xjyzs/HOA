# ArkUI-X HAP 加载流程分析

## 概述

ArkUI-X 是 OpenHarmony 的跨平台移植版本，其 Android APK 包含完整的 NAPI/EcmaVM 运行时，
打包为单个巨型共享库 `libarkui_android.so`（~95MB arm64-v8a）。

源码位置: `/src/arkui-x`

## 1. 完整调用链 (6 层)

```
StageActivity.java (Android)
  ↓ setInstanceName("bundleName:moduleName:abilityName:")
  ↓ 解析 modulePath = "arkui-x/{bundleName}.{moduleName}"
  ↓ dispatchOnCreate(instanceName, params)
  ↓ JNI → libarkui_android.so
AppMain (C++)
  ↓ Application::HandleAbilityStage
JsAbility::Init()
  ↓ 读取 modules.abc 为 buffer
  ↓ 从 module.json 构造 modulePath (见第 2 节)
JsRuntime::LoadModule(moduleName, modulePath, buffer, srcEntrance, esmodule)
  ↓
ArkJsRuntime::LoadJsModule(fileName, hapPath, srcEntrance)
  ↓
ArkNativeEngine::RunScriptBuffer(path, buffer, isBundle=false)
  ↓ [关键: isBundle=false 分支]
JSNApi::ExecuteModuleBuffer(vm, data, size, path)
  │ jsnapi_expo.cpp:5645-5659
  ↓
JSPandaFileExecutor::ExecuteModuleBuffer(thread, data, size, filename, ...)
  │ js_pandafile_executor.cpp:150-195
  ↓
  ├── GetAssetPath(vm) → baseFileName
  ├── NormalizePath(filename) → normalName
  ├── ParseAbcPathAndOhmUrl(vm, normalName, baseFileName, entry)  ← 核心路由
  ├── LoadJSPandaFile(thread, baseFileName, entry, buffer, size)
  ├── CheckIsRecordWithBundleName(entry)
  ├── AdaptOldIsaRecord(entry) — 如果不含 bundleName
  ├── CheckAndGetRecordInfo(entry) → JSRecordInfo*
  └── CommonExecuteBuffer(thread, name, entry, buffer, size)
```

## 2. 模块路径构造 (js_ability_stage.cpp)

ArkUI-X 从 `module.json` 的 `srcEntry` 构造查找用的 filename：

```cpp
// js_ability_stage.cpp:96-101
modulePath = moduleName;                    // "entry"
modulePath.append("/");
modulePath.append(srcEntrance);             // "entry/./ets/entryability/EntryAbility.ets"
modulePath.erase(modulePath.rfind("."));    // 去后缀 → "entry/./ets/entryability/EntryAbility"
modulePath.append(".abc");                  // "entry/./ets/entryability/EntryAbility.abc"
```

> **说明**: 路径中的 `./` 来自 module.json5 的 `srcEntry` 字段（如 `"./ets/entryability/EntryAbility.ets"`），在后续 `NormalizePath` 阶段会被规范化去除。

**关键**: 此路径**不是** `@bundle:` 格式，必然是相对路径进入 `ParseAbcPathAndOhmUrl` 的 Branch 3。

## 3. Asset 管理

### Android AssetHelper

```cpp
// js_worker.cpp:155-255
struct AssetHelper {
    void operator()(const std::string &fpath, uint8_t **buff, size_t *buffSize,
                    std::vector<uint8_t> &content, std::string &ami, ...) {
        // Android: ami = filePath (纯相对路径)
        // 其他平台: ami = codePath_ + filePath (绝对路径)
        // 从 AssetManager 读取 APK assets
    }
};
```

### SetGetAssetFunc

`NativeEngine::SetGetAssetFunc` 在 `native_engine.cpp:596` 设置回调。
`NativeEngine::CallGetAssetFunc` 在 `native_engine.cpp:631-635` 调用它。
`GetAbcBuffer` 在 `native_engine.cpp:1086-1099` 调用 `CallGetAssetFunc`。

### StageApplication 初始化

```
HoaApplication.onCreate()
  └→ StageApplication.onCreate()
       └→ StageApplicationDelegate.initApplication()
            ├→ copyAllModuleResources()
            │    ├→ 复制 assets/arkui-x/systemres/ → filesDir
            │    └→ 复制 assets/arkui-x/{module}/resources/ → filesDir
            ├→ nativeSetAssetManager(assets)
            ├→ nativeSetFileDir(filesDir)
            └→ setResourcesFilePrefixPath(filesDir/arkui-x/)
```

**注意**: 只复制 `resources/` 和 `resources.index`，`.abc` 文件通过 AssetManager 直接从 APK assets 读取。

## 4. ArkUI-X 示例 APK 结构

```
app-release.apk (ArkUI-X example)
├── lib/arm64-v8a/
│   ├── libarkui_android.so          95MB   (ACE + NAPI + ETS运行时 + Skia + ICU)
│   ├── libarkui_componentsnapshot.so  540KB
│   ├── libarkui_focuscontroller.so    520KB
│   └── libhilog.so                    155KB
├── assets/arkui-x/
│   ├── {bundleName}.{moduleName}/    (如 app.hackeris.arkuixexample.entry/)
│   │   ├── ets/modules.abc           (7664 bytes, 60 classes)
│   │   ├── module.json
│   │   ├── ark_module.json
│   │   ├── resources.index
│   │   └── resources/
│   └── systemres/
│       ├── abc/                      (7 个系统 ABC)
│       ├── icudt72l.dat              (33.5MB)
│       ├── fonts/                    (9.7MB)
│       └── resources.index          (1.2MB)
└── classes.dex                       (StageActivity, StageApplication 等)
```

## 5. ABC 文件结构

### 模块记录命名约定

**注意**: 记录名格式由**构建系统**（es2abc/hvigor）在编译阶段决定，非运行时行为。

ArkUI-X compilation pipeline 使用:
- 源文件路径: `ets/entryability/EntryAbility.ets` (ArkUI-X 构建系统去掉了 `src/main/` 前缀)
- 编译后记录名: `{bundleName}/entry/ets/entryability/EntryAbility`

OHOS compilation pipeline (hvigor/es2abc) 使用:
- 源文件路径: `src/main/ets/entryability/EntryAbility.ets` (保留 `src/main/` 前缀)
- 编译后记录名: `entry/src/main/ets/entryability/EntryAbility` (不含 bundleName)

两者差异是导致 OHOS HAP 无法直接在 ArkUI-X Android 运行时执行的根本原因。

### IsBundlePack 判定

`CheckIsBundlePack()` (`js_pandafile.cpp:40-61`):
- 遍历所有非外部类
- 检查每个类的字段名
- 如果任何字段为 `isCommonjs` 或 `moduleRecordIdx` → `isBundlePack_ = false`
- ArkUI-X 和 OHOS HAP 都是 ES 模块格式 → `isBundlePack_` **总是 false**

### InitializeMergedPF 的过滤逻辑

`InitializeMergedPF()` (`js_pandafile.cpp:171-227`):
- 遍历所有非外部类的 class
- 对每个 class，解析字段得到 `recordName`
- 检查 class 中是否含有 `isCommonjs` 或 `jsonFileContent` 字段
- **只有**含这些字段的 record 才会被插入 `jsRecordInfo_` map
- 不含这些字段的 record → `delete info` (被丢弃)

### CheckAndGetRecordInfo 查找

`CheckAndGetRecordInfo(recordName)` (`js_pandafile.h:334-346`):
```cpp
if (IsBundlePack()) {
    return jsRecordInfo_.begin()->second;  // Bundle: 返回第一个记录
}
// 非 Bundle: 精确匹配 find
auto info = jsRecordInfo_.find(recordName);
return (info != jsRecordInfo_.end()) ? info->second : nullptr;
```

## 6. 为什么 ArkUI-X 自己的示例可以运行

1. ArkUI-X 示例 APK 的 modules.abc 内部记录名使用 `{bundleName}/entry/ets/...` 格式
2. `GetOutEntryPoint` 产生的 entryPoint (`bundleName/` + 相对路径) 与 record 名匹配
3. `ParseAbcPathAndOhmUrl` Branch 3 的输出刚好匹配
4. `InitializeMergedPF` 保留了含 `isCommonjs` 字段的 record

## 7. 符号导出

`nm -D libarkui_android.so` 的关键结果:

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

## 8. RunScriptForAbc 的问题

`NativeEngine::RunScriptForAbc` (`native_engine.cpp:993-1022`) 是唯一可直接通过 dlsym 调用的执行函数，但它:

1. 使用 `ExecuteTypes::NAPI` → `IsStaticImport(NAPI)` = false
2. `ParseAbcEntryPoint` 被跳过
3. `CheckIsRecordWithBundleName` / `AdaptOldIsaRecord` 被跳过
4. Entry point 原封不动传给 `CheckAndGetRecordInfo`，做精确匹配
5. 精确匹配对 OHOS HAP 的 record 名必然失败

### ExecuteFromAbcFile 的 NAPI 短路

```cpp
// js_pandafile_executor.cpp:92-109
if (!vm->IsBundlePack() && IsStaticImport(executeType)) {
    std::tie(name, entry) = ParseAbcEntryPoint(thread, filename, entryPoint);
} else {
    // ← NAPI 类型走这里: 不做任何转换
    name = filename;
    entry = entryPoint.data();
}
```

## 9. ArkUI-X 内部加载机制 (RunScriptBuffer 路径)

ArkUI-X 自己的 `StageActivity` 走 `RunScriptBuffer` 路径（而非 `RunScriptForAbc`）:

```
ArkNativeEngine::RunScriptBuffer(path, buffer, isBundle=false)
  ↓
JSNApi::ExecuteModuleBuffer(vm_, buffer.data(), buffer.size(), path, needUpdate)
  ↓
JSPandaFileExecutor::ExecuteModuleBuffer(...)
  ↓ 内部调用 ParseAbcPathAndOhmUrl
  ↓ CheckIsRecordWithBundleName + AdaptOldIsaRecord
  ↓ CheckAndGetRecordInfo(entry) → 成功匹配
```

关键: `ExecuteModuleBuffer` 是 `libarkui_android.so` **内部链接**的，ArkUI-X 内部使用直接链接调用，不需要动态符号导出。

## 10. Plan A 的具体失败点

1. **dlsym 可用的 API 错误**: `RunScriptForAbc` 设计用于 restricted worker, 不是主模块入口
2. **NAPI 类型跳过所有路由**: 入口点不做任何转换
3. **入口点格式不匹配**: OHOS HAP record 名 `entry/src/main/ets/...` vs 传入的简单字符串
4. **ExecuteModuleBuffer 不可见**: 正确的执行函数被隐藏
5. **GetOutEntryPoint 添加 bundleName 前缀**: 即使进入 Branch 3，产生的 entryPoint 也是 `bundleName/arkui-x/...`，与 OHOS record 名不匹配
