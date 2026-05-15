# srcEntry → entryPoint 完整转换链

**状态**: 已从源码追踪验证（OHOS + ArkUI-X 双向对比）

---

## 一、转换链总览

```
module.json5: srcEntry = "./ets/entryability/EntryAbility.ets"
        │
        ▼
[JsAbility::GenerateSrcPath]     ← 将 srcEntry 转换为 abc 文件路径
        │
        ▼
[JsAbility::Init → LoadModule]   ← 构造模块名、文件路径、传递 buffer
        │
        ▼
[LoadJsModule → RunScript]       ← 设置 AssetPath、读取 buffer、调用引擎
        │
        ▼
[ExecuteModuleBuffer]            ← 解析路径、生成 entryPoint
        │
        ▼
[ParseAbcPathAndOhmUrl]          ← 三路分支路由，生成 entryPoint
        │
        ▼
[CheckIsRecordWithBundleName]    ← 判断 record 是否含 bundleName
        │
        ▼
[AdaptOldIsaRecord]              ← 条件触发，截断 2 段路径
        │
        ▼
[CheckAndGetRecordInfo]          ← 在 ABC 内精确查找 record
```

---

## 二、OHOS 完整链（逐行追踪）

### Step 1: 源码位置

`/src/ohos/foundation/ability/ability_runtime/frameworks/native/ability/native/ability_runtime/js_ability.cpp`

### Step 2: GenerateSrcPath（行 151-177）

**输入**:
- `abilityInfo->package` = `"entry"`
- `abilityInfo->srcEntrance` = `"./ets/entryability/EntryAbility.ets"`
- `abilityInfo->isModuleJson` = `true` (Stage 模型)

**处理**（行 166-176）:
```cpp
srcPath = abilityInfo->package;       // "entry"
srcPath.append("/");                  // "entry/"
srcPath.append(abilityInfo->srcEntrance); // "entry/./ets/entryability/EntryAbility.ets"
srcPath.erase(srcPath.rfind("."));    // 移除 ".ets" → "entry/./ets/entryability/EntryAbility"
srcPath.append(".abc");               // "entry/./ets/entryability/EntryAbility.abc"
```

**输出**: `srcPath = "entry/./ets/entryability/EntryAbility.abc"`

### Step 3: JsAbility::Init 调用 LoadModule（行 141-142）

```cpp
jsAbilityObj_ = jsRuntime_.LoadModule(
    moduleName,    // "entry::EntryAbility"
    srcPath,       // "entry/./ets/entryability/EntryAbility.abc"
    abilityInfo->hapPath,  // "/data/storage/el1/bundle/entry.hap"
    abilityInfo->compileMode == AppExecFwk::CompileMode::ES_MODULE  // true
);
// 注意: srcEntrance 参数未传递，默认 ""
```

### Step 4: JsRuntime::LoadModule 构造 fileName（行 1055-1058）

`/src/ohos/foundation/ability/ability_runtime/frameworks/native/runtime/js_runtime.cpp`

```cpp
// hapPath 非空:
fileName.append(codePath_).append("/").append(modulePath);
// fileName = codePath_ + "/" + "entry/./ets/entryability/EntryAbility.abc"
std::regex pattern("\\." + "/");  // 移除 "./"
fileName = std::regex_replace(fileName, pattern, "");
// fileName = codePath_/entry/ets/entryability/EntryAbility.abc
```

**输出**: `fileName = "{codePath}/entry/ets/entryability/EntryAbility.abc"`

### Step 5: LoadJsModule → RunScript（行 1001, 1176-1186）

```cpp
// LoadJsModule:
RunScript(path=fileName, hapPath, false, srcEntrance="");

// RunScript, ES module 模式 (isBundle_=false):
path = BUNDLE_INSTALL_PATH + moduleName_ + MERGE_ABC_PATH;
// path = "/data/storage/el1/bundle/" + "entry" + "ets/modules.abc"
panda::JSNApi::SetAssetPath(vm, path);    // ★ 设置 AssetPath 为 modules.abc
panda::JSNApi::SetModuleName(vm, "entry");

func(path, srcPath);  // modulePath=modules.abc路径, abcPath=fileName
// func 读取 modules.abc 的 buffer
// LoadScript(abcPath, buffer, len, isBundle_, srcEntrance="")
// abcPath = "entry/ets/entryability/EntryAbility.abc"
```

**关键**: AssetPath = `modules.abc` 路径；但传入 ExecuteModuleBuffer 的 filename = `abcPath`（即原始 srcPath）

### Step 6: ExecuteModuleBuffer → ParseAbcPathAndOhmUrl

`/src/ohos/arkcompiler/ets_runtime/ecmascript/jspandafile/js_pandafile_executor.cpp:180-233`

```cpp
name = vm->GetAssetPath();  // "/data/storage/el1/bundle/entry/ets/modules.abc"
CString normalName = PathHelper::NormalizePath(filename);
// filename = "entry/ets/entryability/EntryAbility.abc"
// NormalizePath 消除了 "./", 结果不变 (./ 已被 regex 移除)
ModulePathHelper::ParseAbcPathAndOhmUrl(vm, normalName, name, entry);
```

**ParseAbcPathAndOhmUrl**（`module_path_helper.cpp:156-205`）:
- `normalName = "entry/ets/entryability/EntryAbility.abc"`
- 不以 `/data/storage/el1/bundle/` 开头 → **不走 Branch 1**
- 不以 `@bundle:` 开头 → **不走 Branch 2**
- 走 **Branch 3**（相对路径）:
  ```cpp
  #if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS)
      // OHOS 平台:
      outEntryPoint = vm->GetBundleName() + "/" + inputFileName;
      // = "app.hackeris.harmonyexample/entry/ets/entryability/EntryAbility.abc"
  #endif
  ```
- 移除 `.abc` 后缀 → **`entryPoint = "app.hackeris.harmonyexample/entry/ets/entryability/EntryAbility"`**

### Step 7: Bundle/ES Module 分支（行 213-221）

OHOS 代码（行 213-221）:
```cpp
const CString realEntry = entry;
if (vm->IsNormalizedOhmUrlPack()) {
    entry = TransformToNormalizedOhmUrl(...);
} else if (!isBundle) {                          // ES Module 模式
    jsPandaFile->CheckIsRecordWithBundleName(entry);
    if (!jsPandaFile->IsRecordWithBundleName()) {
        PathHelper::AdaptOldIsaRecord(entry);     // ★ 截断 2 段路径
    }
}
```

**ES Module 场景（isBundle=false）**:
- `CheckIsRecordWithBundleName("app.hackeris.harmonyexample/entry/ets/entryability/EntryAbility")`
- first segment = `"app.hackeris.harmonyexample"`
- 遍历 ABC records, 检查是否以此开头
- **ABC records**: `entry/src/main/ets/entryability/EntryAbility` → 不以 bundleName 开头
- → `isRecordWithBundleName_ = false`
- → **AdaptOldIsaRecord 触发**
- `"app.hackeris.harmonyexample/entry/ets/entryability/EntryAbility"`
- 截断前 2 段 `/` → **`"ets/entryability/EntryAbility"`**

### Step 8: CheckAndGetRecordInfo

```cpp
jsPandaFile->CheckAndGetRecordInfo("ets/entryability/EntryAbility", &recordInfo);
// ABC 内部 record: "entry/src/main/ets/entryability/EntryAbility"
// "ets/entryability/EntryAbility" ≠ "entry/src/main/ets/entryability/EntryAbility"
// → 查找失败！
```

### OHOS 为何实际能工作？

**OHOS 默认使用 Bundle 模式**（非 ES Module）。在 Bundle 模式下:
1. `isBundle = true` → AdaptOldIsaRecord **不触发**
2. Bundle 模式的 ABC record 命名格式可能不同于 ES module 格式
3. Bundle 的 `CheckAndGetRecordInfo` 可能有模糊匹配逻辑

**测试 HAP 虽然 compileMode 标记为 "esmodule"，但实际在 OHOS 设备上可能是按 Bundle 模式加载的。**

---

## 三、ArkUI-X 完整链（逐行追踪）

### Step 1: 源码位置

`/src/arkui-x/foundation/appframework/ability/ability_runtime/cross_platform/frameworks/native/ability/js_ability.cpp`

### Step 2: JsAbility::Init（行 68-98）

```cpp
// ES module 分支 (行 86-95):
modulePath = abilityInfo->package;              // "entry"
modulePath.append("/");                          // "entry/"
modulePath.append(abilityInfo->srcEntrance);     // "entry/./ets/entryability/EntryAbility.ets"
modulePath.erase(modulePath.rfind("."));         // "entry/./ets/entryability/EntryAbility"
modulePath.append(".abc");                       // "entry/./ets/entryability/EntryAbility.abc"
```

**输出**: `modulePath = "entry/./ets/entryability/EntryAbility.abc"`

### Step 3: LoadModule（行 97-98）

```cpp
jsAbilityObj_ = jsRuntime_.LoadModule(
    moduleName,     // "entry::EntryAbility"
    modulePath,     // "entry/./ets/entryability/EntryAbility.abc"
    abilityBuffer,  // 预读的 .abc buffer
    abilityInfo->srcEntrance,  // "./ets/entryability/EntryAbility.ets" ← 传递了！
    esmodule        // true
);
```

**与 OHOS 的关键差异**: ArkUI-X 将 `srcEntrance` 作为第 4 参数传递！

### Step 4: LoadModule 构造 fileName（行 550-560）

`/src/arkui-x/foundation/appframework/ability/ability_runtime/cross_platform/frameworks/native/jsruntime/src/js_runtime.cpp`

```cpp
// ES module 分支:
std::string moduleNamePath = moduleName_;  // "entry" (已从 "entry::EntryAbility" 提取)
fileName.append(moduleNamePath).append("/").append(srcEntrance);
// fileName = "entry/./ets/entryability/EntryAbility.ets"
fileName.erase(fileName.rfind("."));    // "entry/./ets/entryability/EntryAbility"
fileName.append(".abc");                // "entry/./ets/entryability/EntryAbility.abc"
std::regex pattern("\\." + "/");
fileName = std::regex_replace(fileName, pattern, "");
// fileName = "entry/ets/entryability/EntryAbility.abc"
```

**输出**: `fileName = "entry/ets/entryability/EntryAbility.abc"`

### Step 5: LoadJsModule → RunScriptBuffer（行 163-186）

```cpp
std::string assetPath = BUNDLE_INSTALL_PATH + moduleName_ + MERGE_ABC_PATH;
// = "/data/storage/el1/bundle/entry/ets/modules.abc"
panda::JSNApi::SetAssetPath(vm_, assetPath);
panda::JSNApi::SetModuleName(vm_, moduleName_);

bool result = engine->RunScriptBuffer(path.c_str(), buffer, false, needUpdate) != nullptr;
// path = "entry/ets/entryability/EntryAbility.abc"
```

### Step 6: ExecuteModuleBuffer → ParseAbcPathAndOhmUrl

`/src/arkui-x/arkcompiler/ets_runtime/ecmascript/jspandafile/js_pandafile_executor.cpp:153-192`

```cpp
name = GetAssetPath(vm);  // "/data/storage/el1/bundle/entry/ets/modules.abc"
CString normalName = PathHelper::NormalizePath(filename);
// filename = "entry/ets/entryability/EntryAbility.abc" (已无 "./")
ModulePathHelper::ParseAbcPathAndOhmUrl(vm, normalName, name, entry);
```

**ParseAbcPathAndOhmUrl**（ArkUI-X 版, `module_path_helper.cpp:163-198`）:
- 不走 Branch 1、Branch 2
- 走 **Branch 3**:
  ```cpp
  outEntryPoint = GetOutEntryPoint(vm, inputFileName);
  ```

**GetOutEntryPoint**（`platform/common/module.cpp:21-24`）:
```cpp
return ConcatToCString(vm->GetBundleName(), "/", inputFileName);
// = "app.hackeris.harmonyexample/entry/ets/entryability/EntryAbility.abc"
```

移除 `.abc` → **`entryPoint = "app.hackeris.harmonyexample/entry/ets/entryability/EntryAbility"`**

### Step 7: Bundle Name Check（行 176-181）

ArkUI-X 代码（行 176-181）:
```cpp
if (!vm->IsNormalizedOhmUrlPack() && !jsPandaFile->IsBundlePack()) {
    jsPandaFile->CheckIsRecordWithBundleName(entry);
    if (!jsPandaFile->IsRecordWithBundleName()) {
        PathHelper::AdaptOldIsaRecord(entry);
    }
}
```

**ArkUI-X 自有 ABC**:
- Record 格式: `app.hackeris.harmonyexample/entry/ets/entryability/EntryAbility`（含 bundleName，无 src/main/）
- `CheckIsRecordWithBundleName` 检查 first segment = `"app.hackeris.harmonyexample"`
- ABC records 以此开头 → `isRecordWithBundleName_ = true`
- **AdaptOldIsaRecord 不触发**
- `CheckAndGetRecordInfo("app.hackeris.harmonyexample/entry/ets/entryability/EntryAbility")` → **找到了！**

**OHOS HAP 的 ABC**:
- Record 格式: `entry/src/main/ets/entryability/EntryAbility`（无 bundleName，有 src/main/）
- `CheckIsRecordWithBundleName` 检查 first segment = `"app.hackeris.harmonyexample"`（来自 entryPoint）
- ABC records 不以此开头 → `isRecordWithBundleName_ = false`
- **AdaptOldIsaRecord 触发**
- `"app.hackeris.harmonyexample/entry/ets/entryability/EntryAbility"`
- 截断 2 段 `/` → `"ets/entryability/EntryAbility"`
- `CheckAndGetRecordInfo("ets/entryability/EntryAbility")` → **找不到！**（实际是 `entry/src/main/ets/entryability/EntryAbility`）

---

## 四、关键发现与结论

### 记录名差异的完整原因

| 差异维度 | OHOS HAP ABC | ArkUI-X ABC |
|---------|-------------|------------|
| bundleName 前缀 | **无** | **有** |
| src/main/ 路径段 | **有** | **无** |
| 示例 | `entry/src/main/ets/entryability/EntryAbility` | `app.hackeris.harmonyexample/entry/ets/entryability/EntryAbility` |

### 两个差异的根因

1. **bundleName 差异**: es2abc 编译时（hvigor 构建系统）在 OHOS 上不添加 bundleName 前缀；ArkUI-X 通过 `GetOutEntryPoint` 运行时添加
2. **src/main/ 差异**: OHOS 的 hvigor/es2abc 编译时自动插入 `src/main/` 路径前缀；ArkUI-X 构建系统不插入此段

### 对技术方案的修正

之前的技术方案认为只需要处理 `GetOutEntryPoint` 和 `AdaptOldIsaRecord`，但**实际情况更复杂**:

1. `GetOutEntryPoint` 添加 bundleName → 导致 `CheckIsRecordWithBundleName` 判定 record 不含 bundleName
2. `AdaptOldIsaRecord` 截断 bundleName + moduleName 两段 → 截断后得到 `ets/entryability/EntryAbility`
3. 但 ABC record 是 `entry/src/main/ets/entryability/EntryAbility` → 仍不匹配！

**正确的处理策略**（三个层面都要解决）:
- **层面 1**: 跳过 `GetOutEntryPoint` 加 bundleName（避免触发错误的 AdaptOldIsaRecord）
- **层面 2**: 跳过 `AdaptOldIsaRecord`（避免截断 OHOS 格式的 entry）
- **层面 3**: entryPoint 中需要补充或匹配 `src/main/` 段，或通过 `CheckAndGetRecordInfo` 的模糊匹配来处理

### Patch 策略调整建议

**方案 A（推荐）**: 在 OHOS HAP 模式下，直接使用 ABC 内部 record 名作为 entryPoint 查找依据——即构造 entryPoint 时保持 `src/main/` 段，且不加 bundleName:

```cpp
// OHOS HAP 模式下的 entryPoint 构造:
// 格式应为: entry/src/main/ets/entryability/EntryAbility
// 匹配 ABC 内部 record
```

**方案 B**: 保持 entryPoint 不变，修改 `CheckAndGetRecordInfo` 支持模糊匹配（忽略 src/main/ 段的差异）。

**方案 A 更可靠**，因为 entryPoint 精确匹配是最安全的查找方式。
