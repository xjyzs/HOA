# ArkUI-X 源码修改说明

> 源码根目录: `/data/share/hoa2/arkui-x/`（repo 管理，含多个独立 git 仓库）
>
> 目标: 使 ArkUI-X 的 Android 运行时能够加载并执行 OHOS 原生格式的 HAP

---

## 涉及仓库与修改

### 1. arkcompiler/ets_runtime（ETS 运行时 VM）

**上游**: `gitcode.com/openharmony/arkcompiler_ets_runtime`

**为什么改** — OHOS 和 ArkUI-X 编译出的 ABC 文件 record 名格式不同：

| 维度 | OHOS HAP ABC | ArkUI-X ABC |
|------|-------------|------------|
| bundleName 前缀 | 无 | 有 |
| src/main/ 路径段 | 有 | 无 |
| 归一化 URL 分隔符 | `&` 包裹 | 无 |

**改了什么** — 新增 `isOhosHapMode` VM 标志位，在模块路由的关键路径上插入判断：

- `ecma_vm.h` — 新增标志位 `isOhosHapMode_` 及访问方法
- `jsnapi_expo.cpp/.h` — 新增 `SetOhosHapMode` / `IsOhosHapMode` 全局 API
- `module.cpp` — `GetOutEntryPoint` 在 OHOS 模式下插入 `src/main/` 并用 `&` 包裹输出（**最关键的修复**）
- `module_path_helper.cpp` — `ParseAbcPathAndOhmUrl` / `ParseUrl` 新增 OHOS 分支
- `js_pandafile_executor.cpp` — 跳过 `AdaptOldIsaRecord`（该函数会将去掉 bundleName 前缀的路径错误截断）

### 2. foundation/appframework（应用框架）

**上游**: `gitcode.com/arkui-x/app_framework`

**为什么改** — 需要在 VM 创建后将标志位注入到 VM 实例

**改了什么**：

- `js_runtime.cpp` — `ArkJsRuntime::Initialize` 中读取 `OHOS_HAP_MODE` 环境变量，调用 `SetOhosHapMode(vm_, true)`

### 3. foundation/arkui/ace_engine/adapter/android（Android 适配器，独立仓库）

**上游**: `gitcode.com/arkui-x/arkui_for_android`

**为什么改** — 需要将 Java 层的配置传递到 C++ 运行时

**改了什么** — 新增 JNI 桥接和 Java API：

- `stage_application_delegate_jni.cpp/.h` — JNI 实现 `SetOhosHapMode`，通过 `setenv` 设置环境变量
- `StageApplication.java` — 公开静态方法 `setOhosHapMode(boolean)`
- `StageApplicationDelegate.java` — `nativeSetOhosHapMode` native 声明

### 4. build（GN 构建系统）

**上游**: `gitcode.com/openharmony/build`

**为什么改** — 使其在不支持 OHOS 原生工具链的 Android NDK 环境中正常编译

**改了什么**：

- `templates/cxx/cxx.gni` — 三项适配：
  1. 当 `is_arkui_x=true` 时清空 `external_deps`（ArkUI-X 不走 OHOS bundles 依赖系统）
  2. 移除 `arm64e` 双架构编译、PAC 分支保护（Android NDK clang 无对应插件）
  3. 移除 test/notice 构建流程（Android 构建不需要）

---

## 标志传递链

```
HoaApplication.kt
  → StageApplication.setOhosHapMode(true)       [仓库 3]
    → JNI nativeSetOhosHapMode                  [仓库 3]
      → setenv("OHOS_HAP_MODE", "1")            [仓库 3]
        → js_runtime.cpp 读取环境变量           [仓库 2]
          → JSNApi::SetOhosHapMode(vm_, true)   [仓库 1]
            → 各模块通过 IsOhosHapMode() 判断   [仓库 1]
```

---

## 调用方 (HOA 项目)

在 `/src/HOA/app/src/main/java/app/hackeris/hoa/HoaApplication.kt` 中：

```kotlin
override fun onCreate() {
    super.onCreate()       // StageApplication.onCreate → 加载 native 库
    setOhosHapMode(true)   // 激活 OHOS 模式标志链
}
```

测试入口已独立到 `DevTestActivity`，与生产主流程 `MainActivity` 分离。
