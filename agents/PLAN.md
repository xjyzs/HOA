# HOA 项目计划 — 在 Android 上运行 OHOS 原生 HAP

## 总体策略

**核心思路**: 以 ArkUI-X 的 Android 构建体系为基础（它已经解决了 arkcompiler/ACE/Skia 的 Android 交叉编译问题），对运行时中**模块路由和 record 名解析的关键路径**做定向适配，使 Android 设备能加载并运行 OHOS 原生格式的 HAP。

**关键认知**:
- ArkUI-X 和 OHOS 共享同一个 arkcompiler_ets_runtime 核心（EcmaVM、PandaFile 解析、SourceTextModule），ArkCompiler 四个子系统均为平台无关的 C++ 代码
- ArkUI-X 已经实现完整的 Android 渲染管线（SurfaceView + Virtual Rosen + Skia）
- Stage 模型单引擎共享设计 — 多个应用组件共享一个 ArkTS 引擎实例，Android 端只需一个 EcmaVM
- **阻塞点集中在模块入口路由和 record 名格式适配两个窄点**，不需要重写整个运行时
- ArkUI-X 已内置 HapParser + minizip，HAP ZIP 读取和 resources.index 解析能力已具备

**参考源码**: `/src/ohos/` (OHOS 源码)、`/src/arkui-x/` (ArkUI-X 源码)
**技术调研文档**: `/data/share/hoa2/` (完整分析文档集)

---

## 测试 HAP

### 阶段 1 验证 HAP: ArkUI-X 格式

**文件**: `/data/share/arkui-x-example/entry/build/default/outputs/default/entry-default-unsigned.hap` (113KB)

先用 ArkUI-X 格式 HAP 验证基线流程跑通（构建 → 加载 .so → EcmaVM 创建 → ABC 加载 → 页面渲染），无需处理 record 名差异。

- bundleName: `app.hackeris.arkuixexample`
- compileMode: `esmodule`
- srcEntry: `./ets/entryability/EntryAbility.ets`
- modules.abc: 7.6KB（仅 2 个 record，无 extensionAbilities）
- ABC record 格式: `{bundleName}/entry/ets/...`（含 bundleName，无 src/main/）
- 与 ArkUI-X 运行时原生兼容，理论上无需 Patch 即可加载

### 阶段 2 验证 HAP: OHOS 原生格式

**文件**: `/data/share/entry-default-unsigned.hap` (123KB)

基线跑通后，切换到 OHOS 格式 HAP，验证 3 个 Patch 的效果。

- bundleName: `app.hackeris.harmonyexample`
- compileMode: `esmodule`
- srcEntry: `./ets/entryability/EntryAbility.ets`
- modules.abc: 12.8KB（3 个 record，含 backup extensionAbility）
- ABC record 格式: `entry/src/main/ets/...`（无 bundleName，有 src/main/）
- 源码工程: `/data/share/harmony-example/`

---

## 核心阻塞点分析

### 阻塞点 1: Record 名格式不匹配（双维度差异）

**根因**: OHOS 构建系统（hvigor/es2abc）和 ArkUI-X 构建系统产生不同格式的 ABC record 名。

| 维度 | OHOS HAP ABC | ArkUI-X ABC |
|------|-------------|------------|
| bundleName 前缀 | **无** | **有** (`app.hackeris.example/`) |
| src/main/ 路径段 | **有** (`entry/src/main/ets/...`) | **无** (`entry/ets/...`) |
| 示例 | `entry/src/main/ets/entryability/EntryAbility` | `{bundleName}/entry/ets/entryability/EntryAbility` |

两个维度根因:
1. **bundleName**: es2abc 编译时 OHOS 不加 bundleName 前缀；ArkUI-X 的 `GetOutEntryPoint` 运行时添加
2. **src/main/**: OHOS 的 hvigor 构建系统编译时插入 `src/main/` 路径前缀；ArkUI-X 构建不插入

### 阻塞点 2: AdaptOldIsaRecord 错误触发

即使去掉 bundleName 前缀，`AdaptOldIsaRecord` 将不含 bundleName 的 entryPoint 视为"旧格式"，删除前两段路径:
```
entry/src/main/ets/entryability/EntryAbility
→ ets/entryability/EntryAbility  (删除了 entry 和 src)
```
进一步破坏了与 ABC 内部 record 名的匹配。

### 阻塞点 3: ExecuteModuleBuffer 符号不可见

ArkUI-X 的 `libarkui_android.so` 中 `ExecuteModuleBuffer` 是 hidden 符号（linker version script 控制），外部无法通过 dlsym 调用。**但自己构建 .so 时此限制不存在。**

### 阻塞点 4: @ohos: 系统模块依赖

OHOS HAP 引用的外部类（`@ohos:hilog`、`@ohos:app.ability.UIAbility` 等）在 Android 平台需要对应实现。ArkUI-X 已打包了部分模块（如 libhilog.so），但不完整。

### 阻塞点 5: HAP 文件读取

**已有方案** — ArkUI-X 已内置 `HapParser` + minizip（`third_party/zlib/contrib/minizip/`），`ReadRawFileFromHap` 可从 HAP (ZIP) 读取任意文件。核心工作从"构建新能力"缩减为"路由集成"。

---

## 为什么需要改代码：运行时条件差异

ArkCompiler 的 C++ 代码在 OHOS 和 ArkUI-X 中**完全相同**（指向同一份源码，不是 fork）。但**代码相同，运行时条件不同** — 同一段 C++ 函数内部有多条分支，OHOS 和 Android 命中不同分支:

### ParseAbcPathAndOhmUrl 三路路由

| 条件 | OHOS | Android (ArkUI-X) |
|------|------|-------------------|
| HAP 文件系统布局 | `/data/storage/el1/bundle/{module}/ets/modules.abc` 真实存在 | **无此布局** |
| 传入 filename 格式 | 绝对路径 `/data/storage/el1/bundle/entry/ets/...` | 相对路径 `entry/ets/...` |
| 命中的分支 | **Branch 1**（绝对路径） | **Branch 3**（相对路径） |

**根因**: Android 上没有 `/data/storage/el1/bundle/` 文件系统布局，传入相对路径必然走 Branch 3 → `GetOutEntryPoint` 加 bundleName → record 名不匹配 → `AdaptOldIsaRecord` 错误截断 → `CheckAndGetRecordInfo` 查找失败。

---

## 已完成的 Phase 成果

Phase 1-2 已验证:

| 成果 | 状态 |
|------|------|
| 版本升级 (0.0.0.5 → 13.0.1.0) | 已验证 — libpandafile.so 可打开现代字节码 |
| HAP 解压 | 已验证 — ZipFile 正确提取 modules.abc (9228 bytes, 60 classes) |
| 所有 .so 加载 | 已验证 — 13 个 native 库全部加载成功 |
| ETS VM 创建 | 已验证 — `ark::ets::CreateRuntime()` 成功 |
| 入口点调用 | **不可行** — `ExecutePandaFile(func_main_0)` 不是正确的模块加载方式 |

Phase 4 研究（`/data/share/hoa2/` 文档集）已完成:
- srcEntry → entryPoint 完整转换链（OHOS + ArkUI-X 双向逐行追踪）
- JNI 桥接层完整分析（两个 JNI 渠道、LoadModule/DispatchOnCreate 双路径）
- NAPI 模块注册与注入机制
- 符号可见性控制方式
- HAP 读取与 resources.index 集成
- Bundle vs ES Module 双模式
- ArkUI-X 构建系统详解

---

## 技术方案

### 总体架构

```
                    ┌──────────────────────────────┐
                    │     Android APK               │
                    │                               │
                    │  StageActivity (Java)         │
                    │  StageApplication (Java)       │
                    │       ↓ JNI                   │
                    │  ┌─────────────────────────┐  │
                    │  │  libohos_android.so      │  │
                    │  │                          │  │
                    │  │  ┌────────────────────┐  │  │
                    │  │  │ ACE/ArkUI (Android) │  │  │
                    │  │  │ Virtual Rosen       │  │  │
                    │  │  │ SurfaceView+Skia    │  │  │
                    │  │  └────────────────────┘  │  │
                    │  │                          │  │
                    │  │  ┌────────────────────┐  │  │
                    │  │  │ AppFramework (适配) │  │  │
                    │  │  │ AppMain → Ability   │  │  │
                    │  │  │ → JsAbility         │  │  │
                    │  │  └────────────────────┘  │  │
                    │  │                          │  │
                    │  │  ┌────────────────────┐  │  │
                    │  │  │ ArkCompiler (修补)  │  │  │
                    │  │  │ EcmaVM              │  │  │
                    │  │  │ ParseAbcPath ★      │  │  │
                    │  │  │ GetOutEntryPoint ★   │  │  │
                    │  │  │ JSPandaFileExecutor  │  │  │
                    │  │  └────────────────────┘  │  │
                    │  │                          │  │
                    │  │  ┌────────────────────┐  │  │
                    │  │  │ HAP 读取 (已有)      │  │  │
                    │  │  │ HapParser + minizip  │  │  │
                    │  │  │ modules.abc 提取     │  │  │
                    │  │  │ resources.index 读取 │  │  │
                    │  │  └────────────────────┘  │  │
                    │  │                          │  │
                    │  │  ┌────────────────────┐  │  │
                    │  │  │ NAPI Modules        │  │  │
                    │  │  │ @ohos:* stubs/ports │  │  │
                    │  │  └────────────────────┘  │  │
                    │  └─────────────────────────┘  │
                    └──────────────────────────────┘
```

标注 ★ 的为与 ArkUI-X 的差异点（3 个定向 Patch）。

### 3 个核心 Patch（已实施）

> 源码根目录: `/data/share/hoa2/arkui-x/arkcompiler/ets_runtime/`

#### Patch 0: isOhosHapMode 标志位

| 文件 | 修改 |
|------|------|
| `ecmascript/ecma_vm.h:1713-1714` | 新增 `bool isOhosHapMode_ {false}` 成员变量 |
| `ecmascript/ecma_vm.h:726-735` | 新增 `IsOhosHapMode()` / `SetIsOhosHapMode()` 方法 |
| `ecmascript/napi/include/jsnapi_expo.h:2009-2010` | 新增 `IsOhosHapMode()` / `SetOhosHapMode()` 静态声明 |
| `ecmascript/napi/jsnapi_expo.cpp:4648-4657` | 新增 `IsOhosHapMode()` / `SetOhosHapMode()` 实现 |

#### Patch 1: GetOutEntryPoint 适配 + src/main/ 段保留

**文件**: `ecmascript/platform/common/module.cpp:21-34`

OHOS HAP 模式下不加 bundleName 前缀，而是将 `/ets/` 替换为 `/src/main/ets/`：
- 输入: `entry/ets/entryability/EntryAbility.abc`
- 输出: `entry/src/main/ets/entryability/EntryAbility.abc`

#### Patch 2: ParseAbcPathAndOhmUrl 新增 OHOS HAP 子分支

**文件**: `ecmascript/module/module_path_helper.cpp:188-194`

Branch 3 中新增 OHOS HAP 判断，走 `GetOutEntryPoint`（已被 Patch 1 修改）。

**文件**: `ecmascript/module/module_path_helper.cpp:356-358`

`ParseUrl` 中的 `AdaptOldIsaRecord` 调用增加 `!vm->IsOhosHapMode()` 条件。

#### Patch 3: AdaptOldIsaRecord OHOS HAP 模式下跳过

| 文件 | 行号 | 修改 |
|------|------|------|
| `ecmascript/jspandafile/js_pandafile_executor.cpp:48` | Execute 中增加 `!vm->IsOhosHapMode()` |
| `ecmascript/jspandafile/js_pandafile_executor.cpp:176` | ExecuteModuleBuffer 中增加 `!vm->IsOhosHapMode()` |
| `ecmascript/jspandafile/js_pandafile_executor.cpp:351` | ExecuteSecureModuleBuffer 中增加 `!vm->IsOhosHapMode()` |
| `ecmascript/napi/jsnapi_expo.cpp:6996` | GetExportObject 中增加 `!vm->IsOhosHapMode()` |

### HAP 读取集成（已有基础，路由集成）

ArkUI-X 已内置 `HapParser` + minizip，无需从零构建。已有能力:
- `HapParser::ReadRawFileFromHap(hapPath, patchPath, rawFileName, len, outValue)` — 从 HAP 读取任意文件
- `HapParser::GetIndexDataFromHap(path, buf, bufLen)` — 读取 resources.index

**需要做的**: 将 `HapParser` 集成到 Android 的 `StageAssetProvider` 中，使 ABC/JSON/resource 读取路径在检测到 `.hap` 后缀时走 `HapParser` 而非 `AssetManager`。

### @ohos: 模块依赖处理

**分层次处理**:

#### Level 1: 直接可用的（ArkUI-X 已有）
- `@ohos:hilog` → libhilog.so (ArkUI-X 已打包)
- `@native:*` 引用 → VM 内建解析，无需额外 .so
- ArkUI 组件系统 → ACE 引擎提供

#### Level 2: 需要移植的（从 OHOS 源码构建）

| 依赖 | 类型 | 来源 |
|------|------|------|
| `@ohos:app.ability.UIAbility` | OHOS_MODULE | ability_runtime NAPI 绑定 |
| `@ohos:app.ability.ConfigurationConstant` | OHOS_MODULE | ability_runtime 配置常量 |
| `@ohos:application.BackupExtensionAbility` | OHOS_MODULE | 备份扩展基类 |

#### Level 3: 可以跳过或 stub 的
- 非核心 API → 功能降级 stub，方法体返回空值

#### Stub 注入方式

1. **静态注册** — `__attribute__((constructor))` + `napi_module_register()` 编译进 .so，加载时自动注册
2. **运行时 Register()** — 在 CreateRuntime 之后显式调用模块注册函数
3. **独立 .so dlopen** — 编译为独立 shared library，`FindNativeModuleByDisk` 按路径搜索

---

### 构建方案

**策略**: 基于 ArkUI-X 的 build.sh + GN 构建系统，做最小化修改。

**核心依据**:
- ArkUI-X 不 fork OpenHarmony 核心代码，直接指向 OHOS master 分支固定 tag，按周同步
- `arkcompiler/ets_runtime`、`foundation/arkui/ace_engine`、`arkcompiler/runtime_core` 等核心组件代码与 OHOS 完全相同
- 差异仅在 adapter 层（Android/iOS 适配器）和 AppFramework 跨平台适配层

**构建命令** (Android):
```shell
./build/prebuilts_download.sh --build-arkuix --skip-ssl   # 下载预编译工具链
./build.sh --product-name arkui-x --target-os android      # Android 编译
```

**ArkUI-X 已有构建产物** (`/data/share/hoa2/arkui-x/out/`):
- `libarkui_android.so` (79MB, arm64) — 已编译成功
- 50+ 插件 .so 文件 (hilog, bluetooth, i18n, util 等)

**具体步骤**:
1. 从 ArkUI-X 的 build.sh 提取 Android target 的 GN/CMake 参数
2. 将编译目标指向 `/src/ohos/arkcompiler/` 和 `/src/ohos/foundation/arkui/`（而非 ArkUI-X 的 fork）
3. 应用上述 3 个 Patch 到 ets_runtime 源码
4. 添加 OHOS NAPI stub 模块到构建
5. 编译 arm64-v8a 目标

**与 ArkUI-X 构建的关键差异**:

| 组件 | ArkUI-X 来源 | 本方案来源 | 原因 |
|------|-------------|-----------|------|
| arkcompiler | arkui-x/arkcompiler (指向 OHOS tag) | ohos/arkcompiler + Patch | 代码相同，需打 patch 适配 OHOS HAP record 格式 |
| ACE 核心 | arkui-x/foundation/arkui (指向 OHOS tag) | ohos/foundation/arkui | 代码相同，保证与 OHOS HAP 兼容 |
| Android 适配 | arkui-x/adapter/android | 使用 ArkUI-X 的适配层 | Android 渲染、窗口已成熟 |
| GetOutEntryPoint | arkui-x 实现(加 bundleName) | ohos 实现 + 新增 OHOS HAP 模式 | 解决 record 名不匹配 |

### 初始化流程

```
StageApplication.onCreate()
  ├→ 加载 libohos_android.so
  ├→ 设置 AssetManager (Android)
  ├→ 复制资源: systemres/ → filesDir (复用 ArkUI-X)
  ├→ 设置 OHOS HAP 模式标志 ★
  └→ nativeLaunchApplication(hapPath)

AppMain::LaunchApplication (native)
  ├→ 初始化 ICU 数据
  ├→ 打开 HAP 文件 (HapParser, 已有) ★
  ├→ 解析 module.json (从 HAP ZIP) ★
  ├→ 创建 EcmaVM (同 ArkUI-X)
  ├→ 设置 VM 为 OHOS HAP 模式 ★
  ├→ 预加载 ACE 模块
  └→ 解析 Bundle 完成

StageActivity.onCreate()
  ├→ 创建 WindowView (SurfaceView)
  ├→ dispatchOnCreate(instanceName, params)
  └→ ...

AppMain::HandleAbilityStage (native)
  ├→ 从 HAP 读取 modules.abc buffer ★
  ├→ 从 module.json 构造 srcEntry ★
  ├→ 构造 OHOS 格式的 entryPoint ★
  ├→ JsAbility::Init(srcEntrance)
  ├→ ExecuteModuleBuffer(buffer, entryPoint) ★
  └→ 调用 JS ability.onCreate()

Window::SetUIContent
  ├→ UIContent::Create
  ├→ DeclarativeFrontendNG 初始化
  ├→ JS page 渲染
  └→ 首帧显示
```

标注 ★ 的为与 ArkUI-X 初始化的差异点。

---

## 落地路线

### Phase 1: 构建验证（预期 1-2 周）

**目标**: 使用 OHOS arkcompiler 源码 + ArkUI-X 构建脚本，成功编译出可在 Android 上加载的 libohos_android.so

**关键任务**:
1. 提取 ArkUI-X build.sh 中 Android target 的编译参数
2. 配置 OHOS 源码路径为 arkcompiler 的源
3. 解决编译依赖（third_party 库路径、头文件路径）
4. 应用 Patch 1-3 到 ets_runtime 源码
5. 编译 arm64-v8a 目标，生成 .so
6. 编写最小 Android 测试 APK，加载 .so 并创建 EcmaVM

**验证标准**: 能在 Android 设备上成功 `System.loadLibrary` 并创建 EcmaVM 实例

### Phase 2: ABC 加载验证（预期 1-2 周）

**目标**: 加载 OHOS HAP 的 modules.abc 并成功执行 CommonExecuteBuffer

**关键任务**:
1. 集成 HapParser 实现 modules.abc 从 HAP 提取 (已有 minizip + HapParser，无需从零构建)
2. 验证 Patch 1-3 效果 (GetOutEntryPoint, ParseAbcPathAndOhmUrl, AdaptOldIsaRecord)
3. 编写测试代码: 打开测试 HAP → 读取 modules.abc → 创建 VM → 执行模块
4. 验证 `CheckAndGetRecordInfo` 能够精确匹配 OHOS record 名

**验证标准**: 能在 Logcat 中看到 `SourceTextModule::Evaluate` 成功返回（即使 @ohos: 依赖导致运行时错误）

### Phase 3: @ohos: 模块依赖解决（预期 2-3 周）

**目标**: 使 OHOS HAP 中的模块代码能够完整执行（resolve 所有 @ohos: 导入）

**关键任务**:
1. 分析目标测试 HAP 的完整外部类依赖列表
2. 为 Level 1 模块（已有的）建立映射
3. 从 OHOS 源码移植 Level 2 模块（ability, router 等）
4. 为 Level 3 模块生成 stub
5. 集成到构建系统

**验证标准**: `SourceTextModule::Instantiate` 成功解析所有依赖，Evaluate 不因模块缺失而崩溃

### Phase 4: Ability 生命周期集成（预期 2 周）

**目标**: 实现完整的 ability 创建→启动流程

**关键任务**:
1. 适配 `JsAbility::Init` 以使用 OHOS HAP 格式的 module.json
2. 实现 OHOS 风格的 srcEntrance 路径构造
3. 适配 ability 生命周期回调 (onCreate, onWindowStageCreate 等)
4. 集成到 StageActivity Java 层
5. 适配 `nativeDispatchOnCreate` JNI 渠道
6. 实现 instanceName 解析 (`bundleName:moduleName:abilityName:instanceId`)

**验证标准**: 测试 HAP 的 ability 能从 Java Activity 触发，JS `onCreate()` 被成功调用

### Phase 5: ArkUI 渲染集成（预期 2 周）

**目标**: 实现 HAP 页面在 Android 上的完整渲染

**关键任务**:
1. 测试 DeclarativeFrontendNG 是否能正确解析 OHOS HAP 中的页面 ABC
2. 验证 resources.index 加载（`HapParser::GetIndexDataFromHap`）
3. 验证 SurfaceView 渲染管线对 OHOS 页面的兼容性
4. 适配 `StageAssetProvider` 的 HAP 路径判断
5. 处理可能的 ACE 版本差异

**验证标准**: 测试 HAP 的首页能在 Android SurfaceView 中正确渲染

### Phase 6: 完整集成与测试（预期 2 周）

**目标**: 端到端集成，可运行完整的 OHOS HAP 应用

**关键任务**:
1. 端到端流程贯通: APK 安装 → 启动 → HAP 解包 → ABC 加载 → 页面渲染
2. 测试多个 HAP 样本（简单页面 → 复杂交互）
3. 性能基准测试（与 OHOS 原生和 ArkUI-X 对比）
4. 文档和工具链完善

---

## 风险与缓解

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|---------|
| OHOS 和 ArkUI-X 的 ACE 版本差异过大 | 中 | 高 | 优先使用 OHOS 源码 + ArkUI-X Android 适配层 |
| @ohos: 依赖过多无法全部 stub | 中 | 中 | 分 tier 处理，优先解决高频依赖 |
| ABC 格式差异超出预期 | 低 | 高 | Phase 2 提前验证，如失败考虑 ABC 重编译方案 |
| ArkUI-X 构建脚本与 OHOS 源码不兼容 | 中 | 中 | 可从 ArkUI-X 的 gn 参数出发逐步适配 |
| 性能劣化（翻译执行 vs AOT） | 低 | 中 | ArkUI-X 已启用 AOT stub，可复用 |

## 替代方案（如果上述路径受阻）

### Plan B: 从 ETS 源码重编译 ABC
获取 HAP 的 ETS 源码，用 ArkUI-X 的 es2abc 工具链重新编译成 ArkUI-X 兼容格式的 .abc。不需要修改运行时，但需要 ETS 源码，本质上是"重组 App"而非"直接运行 HAP"。

### Plan C: 基于 ArkUI-X fork 扩展
不修改 OHOS 原版代码，将 OHOS HAP 兼容层作为 ArkUI-X 的扩展，添加 OHOS HAP 格式适配器 + 路径仿真。改动更收敛但可能无法支持所有 OHOS 特性。

---

## 技术调研文档索引（/data/share/hoa2/）

### 核心加载流程
- `arkui-x-script-loading.md` — ArkUI-X 完整的 6 层调用链，RunScriptBuffer 路径，Plan A 失败分析
- `ohos-script-loading.md` — OpenHarmony 原生 8 阶段加载流程，双路径分流
- `ohos-hap-boot-flow.md` — OHOS 完整启动流程

### 构建系统
- `arkui-x-android-build.md` — ArkUI-X APK 结构和 libarkui_android.so 构建分析
- `arkui-x-android-init.md` — Activity.onCreate() 到首帧的 8 阶段初始化序列
- `jni-bridge-analysis.md` — JNI 桥接层完整分析：两个 JNI 渠道、双路径、参数转换链
- `arkui-x-build-system.md` — ArkUI-X GN 构建架构、仓库组织、平台动态发现机制

### 核心不兼容点
- `why-patch-needed.md` — 为什么需要改代码：运行时条件差异分析
- `module-record-mismatch.md` — ABC record 命名差异，Plan A 失败根因
- `abc-path-routing.md` — ParseAbcPathAndOhmUrl 三路分支路由
- `srcEntry-to-entrypoint-chain.md` — srcEntry→entryPoint 完整转换链：双维度差异

### 运行时子系统
- `bundle-vs-esmodule.md` — Bundle vs ES Module 双模式
- `ecmavm-creation.md` — EcmaVM 创建与配置
- `napi-module-resolution.md` — @ohos:/@bundle:/@native: 模块前缀解析
- `napi-module-registration.md` — NAPI 模块注册机制、三种 stub 注入方式
- `symbol-visibility-control.md` — 符号可见性控制：linker version script 机制
- `hap-resource-integration.md` — HAP 读取与 resources.index 集成
- `platform-abstraction-layer.md` — 7 个平台抽象层

### Stage 模型与编译
- `stage-model-architecture.md` — Stage 模型架构、生命周期、单引擎共享设计
- `ohos-build-pipeline.md` — HAP 编译流水线、record 名构造规则
- `abc-bytecode-format.md` — PANDA ABC 二进制格式、跨平台执行条件
- `test-hap-analysis.md` — 测试 HAP 样本完整分析

### 验证与评估
- `information-gap-analysis.md` — 交叉验证、信息充分性评估（6 个缺口全部已解决）
- `technical-plan.md` — 本计划的前版（完整技术方案）

### 渲染与构建产物
- `arkui-rendering-android.md` — Virtual Rosen Window, SurfaceView/Skia 渲染管线
- `libarkui-android-contents.md` — libarkui_android.so 完整构建内容分析
