# NAPI 模块注册与注入机制

## 概述

OHOS HAP 使用 `@ohos:`, `@app:`, `@native:` 三种命名空间导入外部模块。这些模块在运行时由 `NativeModuleManager` 解析和加载。Android 上这些模块的来源和加载方式与 OHOS 不同。

---

## 一、NAPI 模块注册方式

### 1.1 静态注册（OHOS 标准方式）

每个 NAPI 模块通过 `__attribute__((constructor))` 在 .so 加载时自动注册：

```cpp
// 示例: ability_module.cpp
static napi_module _module = {
    .nm_version = 0,
    .nm_filename = "app/ability/libability.so/ability.js",
    .nm_modname = "app.ability.UIAbility",       // ★ 模块名
};

extern "C" __attribute__((constructor))
void NAPI_app_ability_UIAbility_AutoRegister(void)
{
    napi_module_register(&_module);
}
```

`napi_module_register` 将模块插入 `NativeModuleManager` 的链表中，由其统一管理。

### 1.2 模块命名空间前缀

| 前缀 | 含义 | 示例 |
|------|------|------|
| `@ohos:` | 系统模块（.so 方式） | `@ohos:app.ability.UIAbility` → `app.ability.UIAbility` |
| `@app:` | 应用模块 | `@app:...` |
| `@native:` | 原生模块（内建解析） | `@native:system.app` → VM 内建 |

`@ohos:` 前缀在 VM 层面被剥离，剩余部分直接作为模块名传递给 `NativeModuleManager::LoadNativeModule`。

### 1.3 Android 的特殊处理

**`native_module_manager.cpp:1002-1003`**:
```cpp
#ifdef ANDROID_PLATFORM
    isAppModule = true;   // Android 上强制以应用模块方式加载
#endif
```

在 Android 上，`isAppModule` 恒为 `true`，这改变了模块搜索路径和命名方式。

---

## 二、模块搜索三路径

`FindNativeModuleByDisk` (`native_module_manager.cpp:1326-1459`) 按以下顺序搜索：

### Path 1: 主 .so 路径
```
{prefix}/lib{moduleName}.so
```
- `prefix` 来自 `appLibPathMap_["default"]`，如 `/data/app/.../lib/arm64:/data/data/.../files/arkui-x/libs/arm64/`
- `moduleName` 被转为小写（Android）

### Path 2: 备用 _napi .so
```
{prefix}/lib{moduleName}_napi.so
```
- 如果主 .so 不存在，尝试 `_napi` 后缀版本

### Path 3: ABC 文件
```
{abcPrefix}/{moduleName}.abc
```
- `abcPrefix` 通常是 `{filesDir}/arkui-x/systemres/abc`
- 如果 .so 文件都找不到，尝试读取预编译的 .abc 文件

### LoadModuleLibrary（dlopen）

```cpp
// native_module_manager.cpp:1230
lib = dlopen(path, RTLD_LAZY);
// 加载完成后调用 napi_onLoad 回调
Napi_onLoadCallback(lib, moduleName);
```

---

## 三、静态链接 vs 动态加载

### OHOS 模式
- 每个 `@ohos:xxx` 模块对应一个独立的 .so 文件（如 `libability.so`）
- .so 的 `__attribute__((constructor))` 自动注册模块到 NativeModuleManager
- 各 .so 位于 `/system/lib64/module/{namespace}/`

### ArkUI-X Android 模式
- 所有模块**静态链接**进单体 `libarkui_android.so`
- 模块注册通过 `__attribute__((constructor))` 在 .so 加载时完成
- **但**: 并非所有 OHOS 系统模块都被打包进 `libarkui_android.so`

### 哪些模块已内置

ArkUI-X 中已注册的 NAPI 模块位于:
- `foundation/arkui/ace_engine/component_ext/` — UI 组件扩展（`arkui.ArcList`, `arkui.ArcSwiper` 等）
- `foundation/arkui/napi/` — 引擎内建模块和 sample

### 哪些模块缺失（需端口/stub）

以测试 HAP (`entry-default-unsigned.hap`) 为例:

| 模块名 | 来源 | 状态 |
|--------|------|------|
| `app.ability.UIAbility` | `foundation/ability/ability_runtime/` | **缺失**，需端口或 stub |
| `app.ability.ConfigurationConstant` | 同上 | **缺失** |
| `application.BackupExtensionAbility` | 同上 | **缺失**（低优先级） |
| `hilog` | `foundation/hilog/` | ArkUI-X 已有 (`libhilog.so`) |

---

## 四、Stub/Mock 注入点

有三种注入方式：

### 方式 1: 静态注册 Stub 模块

```cpp
// 在 app_main.cpp 或单独的 stub.cpp 中
static napi_module _module = {
    .nm_version = 0,
    .nm_filename = "stub/ability/ability.js",
    .nm_modname = "app.ability.UIAbility",
};
extern "C" __attribute__((constructor))
void NAPI_app_ability_UIAbility_AutoRegister(void)
{
    napi_module_register(&_module);
}
```

**优点**: 无需 .so 文件，直接编译进 libohos_android.so
**缺点**: 需在 `GetJSCode` / `GetABCCode` 中提供有效的 ABC 字节码（即使是空实现）

### 方式 2: 通过 NativeModuleManager::Register() 运行时注入

在 `CreateRuntime` 之后、`HandleAbilityStage` 之前:
```cpp
NativeModule* stub = new NativeModule();
stub->name = strdup("default/app.ability.UIAbility");
stub->moduleName = strdup("default/app.ability.UIAbility");
stub->registerCallback = [](napi_env) { /* 注册空 API */ };
stub->moduleLoaded = true;
NativeModuleManager::GetInstance()->Register(stub);
```

**优点**: 灵活，无需编译 .abc
**缺点**: 需要理解 NativeModule 的完整生命周期

### 方式 3: 提供独立 .so 文件

将 stub 编译为 `libapp_ability_uIAbility.so`（注意 Android 小写），放入 `filesDir/arkui-x/libs/arm64-v8a/`。

**优点**: 不修改现有 .so
**缺点**: 需要额外构建步骤；需匹配 `GetNativeModulePath` 的路径规则（小写、`lib` 前缀、`.so` 后缀）

---

## 五、Android 路径规则细节

`GetNativeModulePath` 对模块名的处理（Android 特有）:

```cpp
// 1. 转小写
for (int32_t i = 0; i < lengthOfModuleName; i++) {
    dupModuleName[i] = tolower(dupModuleName[i]);
}

// 2. 对于不含 '.' 的模块名（如 hilog），构造路径:
//    Path 1: lib{moduleName}.so  →  libhilog.so
//    Path 2: lib{moduleName}_napi.so  →  libhilog_napi.so
//    Path 3: {abcPrefix}/{moduleName}.abc

// 3. 对于含 '.' 的模块名（如 app.ability.UIAbility），构造路径:
//    '.' 替换为 '/' → app/ability/UIAbility
//    Path: {prefix}/{modulePath}/lib{afterDot}.so
//    例: {prefix}/app/ability/libUIAbility.so

// 4. 主路径检查顺序:
//    nativeModulePath[0] = lib{moduleName}.so 或 {prefix}/.../lib{name}.so
//    nativeModulePath[1] = lib{moduleName}_napi.so 或备用路径
//    nativeModulePath[2] = abc 文件路径
```

---

## 六、对 OHOS HAP on Android 项目的影响

### Phase 3 实施策略

1. **Level 1 依赖（已有）**：`hilog`, `@native:*` — 无需处理
2. **Level 2 依赖（端口）**：`UIAbility`, `ConfigurationConstant` — 建议方式 1（静态注册 stub）
3. **Level 3 依赖（可延后）**：`BackupExtensionAbility` — Phase 2 后处理

### 关键集成点

- `NativeModuleManager::LoadNativeModule` — 模块查找入口
- `FindNativeModuleByDisk` — Android 上的磁盘查找逻辑
- `NativeModuleManager::Register` — 运行时注入点
- `napi_module_register` — 静态注册入口

### GetJSCode/GetABCCode 机制

每个注册的模块需要提供 `GetJSCode` 或 `GetABCCode` 回调，用于返回模块的 JS/ABC 字节码。命名规则为 `NAPI_{模块名}_GetABCCode`。对于 stub 模块，可返回空的合法 ABC。

---

## 七、源码索引

| 文件 | 关键行 | 内容 |
|------|--------|------|
| `native_module_manager.cpp` | 282-347 | `Register()` — 模块注册入口 |
| `native_module_manager.cpp` | 805-949 | `LoadNativeModule()` — 模块加载主入口 |
| `native_module_manager.cpp` | 966-1196 | `GetNativeModulePath()` — 路径构造（含 Android 专门逻辑） |
| `native_module_manager.cpp` | 1198-1259 | `LoadModuleLibrary()` — dlopen 加载 |
| `native_module_manager.cpp` | 1317-1324 | `Napi_onLoadCallback()` — napi_onLoad 回调 |
| `native_module_manager.cpp` | 1326-1459 | `FindNativeModuleByDisk()` — 三路径搜索 |
| `native_module_manager.cpp` | 1002-1003 | `isAppModule = true` — Android 强制标志 |
| `ability_module.cpp` (OHOS) | 24-32 | NAPI 模块注册示例 |
