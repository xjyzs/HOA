# 为什么 OHOS 能直接运行 HAP，Android 上却需要修改代码？

## 问题

arkcompiler（ArkCompiler eTS Runtime）的 C++ 代码在 OHOS 和 ArkUI-X 中**完全相同**（指向同一份源码，不是 fork）。OHOS 本身就能加载和运行 HAP。为什么在 Android 上移植同一份代码，却不能直接复用 OHOS 的运行时逻辑，还需要 Patch？

## 答案概要

**代码相同，但运行时条件不同**。同一段 C++ 函数内部有多条分支，OHOS 和 Android 在各运行条件下命中不同的分支，导致行为差异。

---

## 一、三路分支的不同命中

### ParseAbcPathAndOhmUrl 的三路路由

```cpp
// module_path_helper.cpp:156-205
void ParseAbcPathAndOhmUrl(vm, inputFileName, outBaseFileName, outEntryPoint)
{
    if (inputFileName 以 "/data/storage/el1/bundle/" 开头) {
        // Branch 1: 绝对文件系统路径（OHOS 主路径）
        outEntryPoint = bundleName + "/" + inputFileName.substr(前缀后);
    }
    else if (inputFileName 以 "@bundle:" 开头) {
        // Branch 2: OhmUrl 格式（页面路由专用）
        outEntryPoint = inputFileName.substr("@bundle:" 长度);
    }
    else {
        // Branch 3: 相对路径（ArkUI-X Android 主路径）
        outEntryPoint = GetOutEntryPoint(vm, inputFileName);
        // GetOutEntryPoint = bundleName + "/" + inputFileName  ← 加 bundleName
    }
}
```

### 为什么 OHOS 和 Android 命中不同分支

| 条件 | OHOS | Android (ArkUI-X) |
|------|------|-------------------|
| HAP 文件系统布局 | `/data/storage/el1/bundle/{module}/ets/modules.abc` 真实存在 | **无此布局**，HAP 内容从 APK assets 或目录加载 |
| 传入的 `filename` 格式 | 绝对路径 `/data/storage/el1/bundle/entry/ets/...` | 相对路径 `entry/ets/...` |
| 命中的分支 | **Branch 1**（绝对路径） | **Branch 3**（相对路径） |

**根因**: Android 上没有 `/data/storage/el1/bundle/` 文件系统布局，传入的必然是相对路径，必然走 Branch 3。

---

## 二、AdaptOldIsaRecord 截断路径

entryPoint 生成后，`ExecuteModuleBuffer` 中有一段逻辑会判断是否需要截断路径：

```cpp
// js_pandafile_executor.cpp:176-181 (ArkUI-X)
if (!vm->IsNormalizedOhmUrlPack() && !jsPandaFile->IsBundlePack()) {
    jsPandaFile->CheckIsRecordWithBundleName(entry);
    if (!jsPandaFile->IsRecordWithBundleName()) {
        PathHelper::AdaptOldIsaRecord(entry);     // ★ 截断前 2 段路径
    }
}
```

`CheckIsRecordWithBundleName` 取 entryPoint 第一段 `/` 前的内容作为候选 bundleName，与 ABC 内部 record 比对。OHOS HAP 的 record 名不含 bundleName（以 `entry` 开头），比对失败 → `AdaptOldIsaRecord` 触发，删除前 2 段：

```
bundleName/entry/ets/entryability/EntryAbility
    → ets/entryability/EntryAbility  (截断后)
```

这进一步破坏了与 ABC record 的匹配。

---

## 三、ABC Record 名格式差异（编译期固化）

即使消除了 bundleName 和 AdaptOldIsaRecord 的问题，还有一个隐藏的差异：

| 维度 | OHOS HAP ABC record | ArkUI-X ABC record |
|------|---------------------|---------------------|
| bundleName | `entry/src/main/ets/...` (无) | `{bundleName}/entry/ets/...` (有) |
| src/main/ 路径 | 编译时**插入** `src/main/` | 编译时**不插入** |
| 来源 | hvigor/es2abc 构建系统行为 | ArkUI-X 构建系统行为 |

这意味着即使 Patch 掉 `GetOutEntryPoint` 和 `AdaptOldIsaRecord`，entryPoint `entry/ets/entryability/EntryAbility` 仍无法匹配 ABC 内的 `entry/src/main/ets/entryability/EntryAbility`。

**根因**: es2abc 编译时，OHOS 的 hvigor 构建系统会在 record 名中插入 `src/main/` 段。这是编译期行为，与运行时无关。

---

## 四、整个流程的差异对比

```
OHOS 加载 HAP (有完整文件系统):
  module.json5 srcEntry → filename = /data/storage/el1/bundle/entry/ets/...
  → ParseAbcPathAndOhmUrl → Branch 1 (绝对路径)
  → 匹配成功

ArkUI-X Android 加载 OHOS HAP (无 OHOS 文件系统):
  module.json5 srcEntry → filename = entry/ets/... (相对路径)
  → ParseAbcPathAndOhmUrl → Branch 3 (相对路径)
  → GetOutEntryPoint 加 bundleName ★
  → AdaptOldIsaRecord 截断 2 段 ★
  → entryPoint = "ets/entryability/EntryAbility"
  → ABC record = "entry/src/main/ets/entryability/EntryAbility"
  → 不匹配！★
```

标注 ★ 的就是需要 Patch 的三个关键点。核心差异是**路径格式**（绝对 vs 相对）和 **ABC record 格式**（有无 `src/main/`）。

---

## 五、有没有可能完全不改代码？

### 方向 1: 模拟 OHOS 文件系统路径

在 Android 上构造 OHOS 格式的绝对路径（`/data/storage/el1/bundle/...`），让 `ParseAbcPathAndOhmUrl` 命中 Branch 1，从而走与 OHOS 相同的路由逻辑。

**需满足的条件**:
- Android 上创建 `/data/storage/el1/bundle/` 目录结构并展开 HAP 内容
- 调用前将 filename 转换为绝对路径格式

**代价**: 引入文件系统布局依赖；且 Branch 1 仍会加 bundleName 前缀，无法回避 record 格式差异。

### 方向 2: ABC 重编译（Plan B）

用 ArkUI-X 构建工具从 ETS 源码重新编译，生成 ArkUI-X 兼容格式的 ABC。

**代价**: 需要 ETS 源码；不能直接运行已编译的 HAP；与"直接运行 HAP"目标有差距。

### 方向 3: 路径仿真

在 Android 上构造 OHOS 格式的路径传入，让 `ParseAbcPathAndOhmUrl` 命中 Branch 1。

**代价**: 需要理解 Branch 1 的全部逻辑；Android 上无对应文件系统语义。

### 结论

当前 Patch 方案（3 个关键点定向修改）**改动范围最收敛**，且不引入文件系统依赖。本质上是**新增一个兼容模式**而非改写现有逻辑。

---

## 六、本质

**要做的不是"修改源码让 Android 支持 OHOS HAP"，而是"在 ArkUI-X 已有的 Android 构建产物中，新增对 OHOS 格式 ABC record 的兼容"**。

ArkUI-X 已经解决了"arkcompiler 在 Android 上编译和运行"这个最困难的问题。我们只需在关键路径上加一个 OHOS 格式的识别和处理分支，桥接两种 ABC record 命名格式的差异。
