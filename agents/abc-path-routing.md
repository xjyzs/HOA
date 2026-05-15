# ParseAbcPathAndOhmUrl 三路分支路由

## 概述

`ParseAbcPathAndOhmUrl` 是模块加载的核心路由器，根据输入路径格式选择不同的处理分支。OHOS 设备走 Branch 1，ArkUI-X Android 走 Branch 3，这是两个平台的关键差异。

**源文件**: `module_path_helper.cpp:163-198` (ArkCompiler / ets_runtime / ecmascript / module)

## 三路分支逻辑

```cpp
void ModulePathHelper::ParseAbcPathAndOhmUrl(
    EcmaVM *vm, const CString &inputFileName,
    CString &outBaseFileName, CString &outEntryPoint)
{
    if (inputFileName 以 "/data/storage/el1/bundle/" 开头) {
        // Branch 1: OHOS 设备绝对路径
        // 从绝对路径提取 moduleName，构造 baseFileName 和 entryPoint
    }
    else if (inputFileName 以 "@bundle:" 开头) {
        // Branch 2: OhmUrl 格式
        // 去掉前缀，entryPoint = bundleName/moduleName/ets/...
    }
    else {
        // Branch 3: 相对路径 (非 OHOS 平台的默认分支)
        // entryPoint = GetOutEntryPoint(vm, inputFileName)
    }
}
```

## Branch 1: OHOS 设备绝对路径

**触发条件**: inputFileName 以 `/data/storage/el1/bundle/` 开头

**示例输入**: `/data/storage/el1/bundle/com.example.module/ets/pages/Index.abc`

**处理**:
1. 从绝对路径提取 moduleName
2. `outBaseFileName = /data/storage/el1/bundle/{moduleName}/ets/modules.abc`
3. `outEntryPoint = {bundleName}/{moduleName}/ets/pages/Index`

**特点**: BundleName 从 VM context 获取（JSNApi::GetBundleName），与路径无关。

## Branch 2: OhmUrl 格式

**触发条件**: inputFileName 以 `@bundle:` 开头

**示例输入**: `@bundle:bundleName/moduleName/ets/pages/Index.abc`

**处理**:
1. 去掉 `@bundle:` 前缀
2. `outEntryPoint = bundleName/moduleName/ets/pages/Index`
3. `outBaseFileName = ParseUrl(vm, outEntryPoint)` — 解析 bundle URL 到文件路径

**在 OHOS 上的独立路径**: 当 `isOhmUrl_` 为 true 时，不走 ExecuteModuleBuffer，而是走 `ExecuteSecureWithOhmUrl`（jsnapi_expo.cpp:5669），由 `CheckAndGetRecordName(oHmUrl)` 获取 record 名。

**ArkUI-X Android 不支持 OhmUrl 路径**。

## Branch 3: 相对路径 (默认)

**触发条件**: 非 OHOS 平台的默认分支

**示例输入**: `entry/ets/modules.abc` 或 `entry/ets/entryability/EntryAbility.abc`

**处理**:
- `outEntryPoint = GetOutEntryPoint(vm, inputFileName)`
- GetOutEntryPoint 的平台差异见下文

**ArkUI-X 始终进入 Branch 3**，因为使用相对路径而非绝对系统路径。

## GetOutEntryPoint 跨平台实现

### Common (非 OHOS) 实现

**文件**: `platform/common/module.cpp:21-25`

```cpp
CString GetOutEntryPoint(EcmaVM *vm, const CString &inputFileName) {
    return ConcatToCString(vm->GetBundleName(), "/", inputFileName);
    // 输入: "entry/ets/modules.abc"
    // 输出: "app.hackeris.example/entry/ets/modules.abc"
}
```

**结果**: entryPoint 前加了 `{bundleName}/` 前缀。

### OHOS 设备实现

**文件**: `platform/unix/ohos/module.cpp`

Branch 1 已处理完 OHOS 路径，Branch 3 在 OHOS 设备上很少触发。

## AdaptOldIsaRecord 兼容逻辑

**文件**: `base/path_helper.h:71-80`

```cpp
// 旧 ISA 格式: 删除前两段 "/" 分隔的路径
// 输入: "bundleName/moduleName/ets/xxx/xxx"
// 输出: "ets/xxx/xxx"
```

此函数在 `ExecuteModuleBuffer` 中被调用 (行 217-221)，当 `CheckIsRecordWithBundleName` 发现 record 不含 bundleName 时触发：
- 含 bundleName 的 entryPoint (新格式) → 直接精确查找
- 不含 bundleName 的 entryPoint (旧格式) → 删除前两段后再查找

## 平台使用总结

| 平台 | 常用分支 | inputFileName 格式 | entryPoint 格式 |
|------|---------|-------------------|----------------|
| OHOS 原生 | Branch 1 | `/data/storage/el1/bundle/{module}/ets/...` | `{bundle}/{module}/ets/...` |
| OHOS (OhmUrl) | Branch 2 | `@bundle:{bundle}/{module}/ets/...` | `{bundle}/{module}/ets/...` |
| ArkUI-X Android | Branch 3 | `entry/ets/...` (相对路径) | `{bundle}/entry/ets/...` |
