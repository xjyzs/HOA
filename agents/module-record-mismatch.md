# OHOS HAP 与 ArkUI-X: 模块记录名不匹配问题

## 核心问题

OHOS HAP 和 ArkUI-X 的 ABC 文件内部使用**不同的 record 命名格式**，这是导致无法直接复用执行路径的根本原因。

## 命名格式对比

### OHOS HAP 模块记录名

使用**源文件路径**格式（来自 HAP 构建时的源码结构）:

```
entry/src/main/ets/entryability/EntryAbility
entry/src/main/ets/entrybackupability/EntryBackupAbility
entry/src/main/ets/pages/Index
```

### ArkUI-X 模块记录名

使用 **OhmUrl 风格**格式（编译管线添加了 bundleName 前缀）:

```
{bundleName}/entry/ets/entryability/EntryAbility
{bundleName}/entry/ets/pages/Index
```

## 差异根因

| 构建系统 | 源文件路径 | 编译后 record 名 |
|---------|-----------|-----------------|
| OHOS HAP | `src/main/ets/entryability/EntryAbility.ets` | `entry/src/main/ets/entryability/EntryAbility` |
| ArkUI-X | `ets/entryability/EntryAbility.ets`（无 `src/main/`） | `{bundleName}/entry/ets/entryability/EntryAbility` |

## 对执行路径的影响

### ArkUI-X 执行路径（可正常运行）

1. ArkUI-X 的 module.json 中 `srcEntry` 指向相对路径
2. 模块路径构造 (`js_ability_stage.cpp:96-101`):
   ```cpp
   modulePath = moduleName + "/" + srcEntrance
   // → "entry/./ets/entryability/EntryAbility.abc"
   ```
3. `ParseAbcPathAndOhmUrl` Branch 3:
   ```cpp
   entryPoint = GetOutEntryPoint(vm, "entry/./ets/entryability/EntryAbility.abc")
   // → "{bundleName}/entry/./ets/entryability/EntryAbility"
   ```
4. `CheckAndGetRecordInfo` 精确匹配 → 成功（因为 ArkUI-X 编译的 record 名也带 bundleName）

### 使用 OHOS HAP 的执行路径（必然失败）

1. OHOS HAP 的 module.json 中 `srcEntry`:
   ```
   src/main/ets/entryability/EntryAbility.ets
   ```
2. 构造的 modulePath:
   ```
   entry/src/main/ets/entryability/EntryAbility.abc
   ```
3. `ParseAbcPathAndOhmUrl` Branch 3 (Android 环境):
   ```cpp
   entryPoint = GetOutEntryPoint(vm, "entry/src/main/ets/entryability/EntryAbility.abc")
   // → "{bundleName}/entry/src/main/ets/entryability/EntryAbility"
   ```
4. ABC 内部实际 record 名 (OHOS 格式):
   ```
   entry/src/main/ets/entryability/EntryAbility
   ```
5. **不匹配!** entryPoint 有 `{bundleName}/` 前缀，但 ABC 内部 record 没有。

## 更深层的不匹配: OHOS 路径前缀

即使去掉 bundleName 前缀，仍有其他差异:

| 属性 | OHOS HAP | ArkUI-X |
|------|---------|---------|
| 源文件路径前缀 | `src/main/ets/` | `ets/` |
| 路径包含 | `src/main/` | 无此段 |
| 编译上下文 | OHOS SDK (带完整系统模块) | ArkUI-X SDK (子集) |

## 相关 ABC 结构差异

### IsBundlePack 判定

`CheckIsBundlePack()` (`js_pandafile.cpp:40-61`):
- 遍历所有非外部类
- 检查每个类字段名是否有 `isCommonjs` 或 `moduleRecordIdx`
- ArkUI-X 和 OHOS HAP 都是 ES 模块格式 → `isBundlePack_` **总是 false**

### InitializeMergedPF 过滤逻辑

`InitializeMergedPF()` (`js_pandafile.cpp:171-227`):
- 遍历所有非外部类的 class
- 对每个 class，解析字段得到 recordName
- 检查 class 中是否含有 `isCommonjs` 或 `jsonFileContent` 字段
- **只有**含这些字段的 record 才被插入 `jsRecordInfo_` map
- 不含这些字段的 record → `delete info` (被丢弃)

### CheckAndGetRecordInfo 查找

```cpp
// js_pandafile.h:334-346
if (IsBundlePack()) {
    return jsRecordInfo_.begin()->second;  // Bundle: 返回第一个
}
// 非 Bundle: 精确匹配
auto info = jsRecordInfo_.find(recordName);
return (info != jsRecordInfo_.end()) ? info->second : nullptr;
```

## Failed Path Analysis: Plan A 的具体失败点

尝试通过 dlsym 调用 `RunScriptForAbc` 的路径:

1. **入口点格式不匹配**: OHOS HAP record 名 `entry/src/main/ets/...` vs 传入的简单字符串
2. **NAPI 类型跳过所有路由**: `RunScriptForAbc` 使用 `ExecuteTypes::NAPI`:
   ```cpp
   // js_pandafile_executor.cpp:92-109
   if (!vm->IsBundlePack() && IsStaticImport(executeType)) {
       std::tie(name, entry) = ParseAbcEntryPoint(thread, filename, entryPoint);
   } else {
       // ← NAPI 走这里: 不做任何转换
       name = filename;
       entry = entryPoint.data();
   }
   ```
3. **ExecuteModuleBuffer 不可见**: 正确的执行函数被隐藏，dlsym 无法访问
4. **GetOutEntryPoint 添加 bundleName 前缀**: 即使进入 Branch 3，产生的 entryPoint 也是 `bundleName/模块路径`，与 OHOS record 名不匹配
