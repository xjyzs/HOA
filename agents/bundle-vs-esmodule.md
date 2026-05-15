# Bundle 模式 vs ES Module 模式

## 一、两种模式的定义

ArkCompiler eTS Runtime 在加载 ABC 字节码时，有两种互斥的运行模式：

| 模式 | 对应规范 | 引入版本 | HAP module.json5 配置 |
|------|---------|---------|---------------------|
| **Bundle** | CommonJS | API 8 及以前 | `compileMode: "bundle"` 或不设置 |
| **ES Module** | ES Module 标准 | API 9+ Stage 模型 | `compileMode: "esmodule"` |

---

## 二、源码决策链（两层判断）

### 第 1 层：运行时标志 `isBundle_` — 由 module.json5 的 compileMode 决定

**OHOS** — `/src/ohos/foundation/ability/ability_runtime/frameworks/native/appkit/app/main_thread.cpp:1473`:

```cpp
options.isBundle = (entryHapModuleInfo.compileMode != AppExecFwk::CompileMode::ES_MODULE);
```

**ArkUI-X** — `/src/arkui-x/foundation/appframework/ability/ability_runtime/cross_platform/frameworks/native/app/app_main.cpp:261-262`:

```cpp
CreateRuntime(applicationInfo->bundleName,
    bundleInfo->hapModuleInfos.back().compileMode != AppExecFwk::CompileMode::ES_MODULE);
```

**规则**（两边逻辑相同）:
- `compileMode == "esmodule"` → `isBundle = false`
- 其他情况（`"bundle"`、未设置）→ `isBundle = true`

**传递路径**:
```
main_thread.cpp / app_main.cpp
  → JsRuntime::Options.isBundle
    → js_runtime.cpp:748: isBundle_ = options.isBundle
    → js_runtime.cpp:761: JSNApi::SetBundle(vm, options.isBundle)
```

### 第 2 层：ABC 文件标志 `isBundlePack_` — 从二进制结构自动检测

**源码** — `/src/ohos/arkcompiler/ets_runtime/ecmascript/jspandafile/js_pandafile.cpp:45-66`:

```cpp
void JSPandaFile::CheckIsBundlePack()
{
    Span<const uint32_t> classIndexes = pf_->GetClasses();
    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf_->IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(*pf_, classId);
        cda.EnumerateFields([&](panda_file::FieldDataAccessor &fieldAccessor) -> void {
            panda_file::File::EntityId fieldNameId = fieldAccessor.GetNameId();
            panda_file::File::StringData sd = GetStringData(fieldNameId);
            const char *fieldName = utf::Mutf8AsCString(sd.data);
            if (std::strcmp(IS_COMMON_JS, fieldName) == 0 ||
                std::strcmp(MODULE_RECORD_IDX, fieldName) == 0) {
                isBundlePack_ = false;  // ← 检测到 ES Module 标记
            }
        });
        if (!isBundlePack_) {
            return;
        }
    }
    // 循环结束未检测到 → isBundlePack_ 保持默认值 true (Bundle)
}
```

**检测逻辑**:
- ES Module 格式的 ABC 包含 `"isCommonjs"` 或 `"moduleRecordIdx"` 字段 → `isBundlePack_ = false`
- Bundle 格式的 ABC 不含这些字段 → `isBundlePack_ = true`（默认）

**调用时机** — `js_pandafile.cpp:29-34`:

```cpp
JSPandaFile::JSPandaFile(const panda_file::File *pf, const CString &descriptor)
    : pf_(pf), desc_(descriptor)
{
    CheckIsBundlePack();
    if (isBundlePack_) {
        InitializeUnMergedPF();   // Bundle: 初始化未合并的 PFF
    } else {
        InitializeMergedPF();     // ES Module: 初始化合并后的 PFF
    }
}
```

---

## 三、两个标志驱动的行为差异

### 3.1 加载路径

**源码** — OHOS `js_runtime.cpp:1065-1066`:

```cpp
classValue = esmodule ? LoadJsModule(fileName, hapPath, srcEntrance)
    : LoadJsBundle(fileName, hapPath, useCommonChunk);
```

**源码** — ArkUI-X `js_runtime.cpp:550-562`:

```cpp
if (esmodule) {
    classValue = LoadJsModule(fileName, buffer, isDynamicUpdate);
} else {
    classValue = LoadJsBundle(modulePath, buffer);
}
```

### 3.2 AdaptOldIsaRecord 触发

**源码** — `js_pandafile_executor.cpp:213-222`（OHOS）/ `js_pandafile_executor.cpp:176-181`（ArkUI-X）:

```cpp
if (vm->IsNormalizedOhmUrlPack()) {
    // 标准化 OhmUrl 转换
} else if (!isBundle) {
    // ★ ES Module 模式进入
    jsPandaFile->CheckIsRecordWithBundleName(entry);
    if (!jsPandaFile->IsRecordWithBundleName()) {
        PathHelper::AdaptOldIsaRecord(entry);  // ★ 截断 2 段路径
    }
}
// Bundle 模式 (isBundle=true): 上述逻辑全部跳过
```

### 3.3 模块解析方式

**源码** — `js_pandafile_executor.cpp:237-247`:

```cpp
// CommonExecuteBuffer
if (isBundle) {
    moduleRecord.Update(moduleManager->HostResolveImportedModule(buffer, size, filename));
    // Bundle: 直接从 buffer 解析，无 entry 参数
} else {
    moduleRecord.Update(moduleManager->HostResolveImportedModuleWithMerge(
        filename, entry, executeFromJob));
    // ES Module: 带 entry 参数，支持按 record 名合并查找
}
```

### 3.4 汇总表

| 行为 | Bundle (`true`) | ES Module (`false`) |
|------|:---:|:---:|
| `JSPandaFile` 初始化 | `InitializeUnMergedPF()` | `InitializeMergedPF()` |
| `RunScript` → 加载函数 | `LoadJsBundle()` | `LoadJsModule()` |
| `CheckIsRecordWithBundleName` 调用 | 不调用 | **调用** |
| `AdaptOldIsaRecord` 截断路径 | **跳过** | **触发**（条件性） |
| `CommonExecuteBuffer` 解析方式 | `HostResolveImportedModule` | `HostResolveImportedModuleWithMerge` |
| 依赖文件 | 可能拆分 commons.abc / vendors.abc | 单一合并 modules.abc |
| record 名格式 | 含 bundleName 概率高 | 含 src/main/ 概率高 |

---

## 四、为什么有两个模式？

### 历史原因

OpenHarmony 经历了从 API 8（FA 模型，CommonJS）到 API 9+（Stage 模型，ES Module）的架构升级：

- **API 8 及以前**: 使用 FA (Feature Ability) 模型，JS 运行时基于 CommonJS 规范，ABC 打包为 Bundle 格式
- **API 9+**: 引入 Stage 模型，全面转向 ES Module 标准（`import`/`export` 语法）

为了向后兼容旧的 HAP 包，运行时保留了两种模式的完整支持。

### 本质差异

**Bundle 模式**: 将所有模块打包进单一的"捆绑"ABC，模块间通过 bundleName 进行名称空间隔离。更接近传统的 JS bundle 概念——整个 HAP 是一个大的"捆绑包"。

**ES Module 模式**: 保持每个模块的独立性（每个 .ets 文件编译为独立的 record），通过 merge 机制将多个模块的 ABC 合并到同一个 `modules.abc` 文件中，但保留各自的 record 命名空间（含 `src/main/` 源路径信息）。

### 关键澄清：Bundle/ES 模式不是 OHOS 与 ArkUI-X 的分歧点

**同一个 HAP，OHOS 和 ArkUI-X 会用同一种模式加载。**

两边判断 `isBundle` 的逻辑完全相同（`compileMode != ES_MODULE`）。对于 `compileMode: "esmodule"` 的 HAP，两边都会设 `isBundle = false`，都用 ES Module 模式。**不存在"OHOS 用 Bundle 模式，ArkUI-X 用 ES Module 模式"这种情况。**

真正的不匹配原因与运行模式无关：

1. **路径路由差异** — OHOS 传入绝对路径（`/data/storage/el1/bundle/...`）命中 Branch 1；Android 传入相对路径（`entry/ets/...`）命中 Branch 3
2. **ABC record 格式差异** — OHOS 的 hvigor/es2abc 编译时插入 `src/main/` 段；ArkUI-X 不插入

### 对 HAP-on-Android 项目的影响

由于本项目不需要兼容旧 Bundle 格式（API 8 及以前），只处理 ES Module 格式的 Stage 模型 HAP（API 9+），因此 Patch 策略不变：

- 不需要关注 Bundle 模式的 `LoadJsBundle`、`InitializeUnMergedPF`、`HostResolveImportedModule` 等路径
- Patch 聚焦于 ES Module 路径下的 record 名匹配问题（`GetOutEntryPoint` + `AdaptOldIsaRecord` + `src/main/` 段）

强制使用 Bundle 模式加载 ES Module 格式的 ABC 会导致 `JSPandaFile` 初始化失败（`InitializeUnMergedPF` vs `InitializeMergedPF` 不兼容），不能作为解决方案。
