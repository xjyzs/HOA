# ArkUI-X 源码修改说明

> 源码根目录: `/data/share/hoa2/arkui-x/`
>
> 目标: 使 ArkUI-X 的 Android 运行时能够加载并执行 OHOS 原生格式的 HAP（ABC record 名含 `&` 分隔符、无 bundleName 前缀、有 `src/main/` 路径段）。

---

## 修改概览

共修改 **3 个子系统、9 个文件**，通过 `isOhosHapMode` VM 标志位驱动所有行为变化。

### 标志传递链

```
HoaApplication.kt
  └→ StageApplication.setOhosHapMode(true)              [StageApplication.java:103]
       └→ StageApplicationDelegate.nativeSetOhosHapMode() [StageApplicationDelegate.java:1283]
            └→ JNI SetOhosHapMode()                       [stage_application_delegate_jni.cpp:195]
                 └→ setenv("OHOS_HAP_MODE", "1", 1)       [stage_application_delegate_jni.cpp:197]
                      └→ js_runtime.cpp 读取环境变量       [js_runtime.cpp:234]
                           └→ JSNApi::SetOhosHapMode(vm_, true)
                                └→ vm->SetIsOhosHapMode(true)  [ecma_vm.h:732]
                                     └→ 各模块通过 vm->IsOhosHapMode() 判断
```

---

## 修改详情

### 一、arkcompiler/ets_runtime（ETS 运行时 — 核心逻辑）

#### 1. `ecmascript/ecma_vm.h` — VM 标志位

| 位置 | 修改 |
|------|------|
| :727-734 | 新增 `IsOhosHapMode()` / `SetIsOhosHapMode()` 公开方法 |
| :1725 | 新增 `bool isOhosHapMode_ {false}` 成员变量 |

```cpp
bool IsOhosHapMode() const
{
    return isOhosHapMode_;
}
void SetIsOhosHapMode(bool value)
{
    isOhosHapMode_ = value;
}
```

#### 2. `ecmascript/napi/include/jsnapi_expo.h` — 全局 API 声明

| 位置 | 修改 |
|------|------|
| :2009-2010 | 新增 `IsOhosHapMode()` / `SetOhosHapMode()` 静态方法声明 |

```cpp
static bool IsOhosHapMode(EcmaVM *vm);
static void SetOhosHapMode(EcmaVM *vm, bool value);
```

#### 3. `ecmascript/napi/jsnapi_expo.cpp` — 全局 API 实现

| 位置 | 修改 |
|------|------|
| :4648-4655 | 新增 `IsOhosHapMode()` / `SetOhosHapMode()` 实现 |
| :7008 | `GetExportObject` 中 `AdaptOldIsaRecord` 调用加 `!vm->IsOhosHapMode()` 守卫 |

#### 4. `ecmascript/platform/common/module.cpp` — 核心：Entry Point 格式适配

| 位置 | 修改 |
|------|------|
| :22-44 | `GetOutEntryPoint` 新增 OHOS 模式分支 |

OHOS 模式下：
- 将 `/ets/` 替换为 `/src/main/ets/`（插入 `src/main/` 路径段）
- 用 `NORMALIZED_OHMURL_TAG`（`&`）包裹输出
- 不添加 bundleName 前缀

```
输入:  entry/ets/entryability/EntryAbility.abc
输出:  &entry/src/main/ets/entryability/EntryAbility&.abc
       └→ 剥离 .abc 后: &entry/src/main/ets/entryability/EntryAbility&
       └→ 匹配 ABC 内部的 class descriptor record 名
```

这是**最关键的修复** —— 没有 `&` 包裹，`CheckAndGetRecordInfo` 无法精确匹配 ABC record 名。

#### 5. `ecmascript/module/module_path_helper.cpp` — 模块路径路由

| 位置 | 修改 |
|------|------|
| :188 | `ParseAbcPathAndOhmUrl` Branch 3 中新增 `vm->IsOhosHapMode()` 子分支，走 `GetOutEntryPoint`（已被 Patch 4 修改） |
| :356 | `ParseUrl` 中的 `AdaptOldIsaRecord` 调用加 `!vm->IsOhosHapMode()` 守卫 |

#### 6. `ecmascript/jspandafile/js_pandafile_executor.cpp` — AdaptOldIsaRecord 跳过

| 位置 | 修改 |
|------|------|
| :48 | `Execute` 中加 `!vm->IsOhosHapMode()` 守卫 |
| :178 | `ExecuteModuleBuffer` 中加 `!vm->IsOhosHapMode()` 守卫 |
| :361 | `ExecuteSecureModuleBuffer` 中加 `!vm->IsOhosHapMode()` 守卫 |

`AdaptOldIsaRecord` 将不含 bundleName 前缀的 entryPoint 视为"旧格式"，错误地删除前两段路径（`entry/src/main/...` → `ets/...`）。OHOS 模式下必须跳过此适配。

---

### 二、foundation/appframework（应用框架）

#### 7. `ability/ability_runtime/cross_platform/frameworks/native/jsruntime/src/js_runtime.cpp`

| 位置 | 修改 |
|------|------|
| :234-237 | `ArkJsRuntime::Initialize` 中新增环境变量检查 |

```cpp
if (getenv("OHOS_HAP_MODE") != nullptr) {
    panda::JSNApi::SetOhosHapMode(vm_, true);
}
```

VM 创建后立即检查环境变量，将 OHOS 模式标志注入 VM。这是标志传递链的关键节点 —— 连接 Java 层（setenv）与 C++ 层（VM flag）。

---

### 三、foundation/arkui/ace_engine（ACE 引擎 — Android 适配器）

#### 8. `adapter/android/stage/ability/java/jni/stage_application_delegate_jni.h`

| 位置 | 修改 |
|------|------|
| :35 | 新增 `SetOhosHapMode` 静态方法声明 |

#### 9. `adapter/android/stage/ability/java/jni/stage_application_delegate_jni.cpp`

| 位置 | 修改 |
|------|------|
| :57-59 | JNI 注册表添加 `nativeSetOhosHapMode` 映射 |
| :195-198 | `SetOhosHapMode` JNI 实现：调用 `setenv("OHOS_HAP_MODE", ...)` |

#### 10. `adapter/android/stage/ability/java/src/StageApplication.java`

| 位置 | 修改 |
|------|------|
| :103-104 | 新增 `setOhosHapMode(boolean)` 公开静态方法 |

```java
public static void setOhosHapMode(boolean enable) {
    StageApplicationDelegate.nativeSetOhosHapMode(enable);
}
```

#### 11. `adapter/android/stage/ability/java/src/StageApplicationDelegate.java`

| 位置 | 修改 |
|------|------|
| :1283 | 新增 `nativeSetOhosHapMode` native 方法声明 |

---

## 为什么需要这些修改

OHOS 和 ArkUI-X 编译出的 ABC 文件在 record 名格式上有两个维度的差异：

| 维度 | OHOS HAP ABC | ArkUI-X ABC |
|------|-------------|------------|
| bundleName 前缀 | **无** | **有** |
| src/main/ 路径段 | **有** | **无** |
| 归一化 URL 分隔符 | `&` 包裹 | 无 `&` 包裹 |
| 示例 | `&entry/src/main/ets/EntryAbility&` | `app.hackeris.example/entry/ets/EntryAbility` |

ArkUI-X 的代码在两个平台上**完全相同**，但运行时条件不同（Android 上没有 OHOS 的文件系统布局 `el1/bundle/`），导致传入相对路径走不同的代码分支，命中了不适用的 record 名转换逻辑。

以上修改通过在关键路由点（`GetOutEntryPoint`、`ParseAbcPathAndOhmUrl`、`AdaptOldIsaRecord`）插入 `IsOhosHapMode()` 判断，使同一份代码在 OHOS HAP 模式下产出 OHOS 兼容的 record 名。

---

## 调用方 (HOA 项目)

在 `/src/HOA/app/src/main/java/app/hackeris/hoa/HoaApplication.kt` 中：

```kotlin
override fun onCreate() {
    super.onCreate()       // StageApplication.onCreate → 加载 native 库
    setOhosHapMode(true)   // 激活 OHOS 模式标志链
}
```

测试用的一键解压 + 启动功能已独立到 `DevTestActivity`，与生产主流程 `MainActivity` 分离。
