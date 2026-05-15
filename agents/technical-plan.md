# Technical Plan: 在 Android 上运行 OHOS 原生 HAP

## 一、总体策略

**核心思路**: 以 ArkUI-X 的 Android 构建体系为基础（它已经解决了 arkcompiler/ACE/Skia 的 Android 交叉编译问题），对运行时中 **模块路由和记录名解析的关键路径** 进行适配性修改，使其能够加载和运行 OHOS 原生格式的 HAP。

**关键认知**: 
- ArkUI-X 和 OHOS 共享同一个 arkcompiler_ets_runtime 核心（EcmaVM、PandaFile 解析、SourceTextModule），ArkCompiler 四个子系统（Core/Execution/Compiler/Runtime）均为平台无关的 C++ 代码
- ArkUI-X 已经实现完整的 Android 渲染管线（SurfaceView + Virtual Rosen + Skia）
- **Stage 模型关键设计**: 多个应用组件共享**一个** ArkTS 引擎实例（ArkCompiler eTS Runtime），Android 端只需一个 EcmaVM
- 阻塞点集中在 **模块入口路由** 和 **record 名格式适配** 两个窄点，不需要重写整个运行时
- 参考文档: `/src/ohos/docs/zh-cn/` (OHOS Stage模型、模块配置、ArkCompiler)、`/src/arkui-x/docs/zh-cn/` (Android集成指南、平台差异化)

## 二、核心阻塞点分析

### 阻塞点 1: Record 名格式不匹配（双维度差异）

**根因**: OHOS 构建系统（hvigor/es2abc）和 ArkUI-X 构建系统产生不同格式的 ABC record 名。

**差异有两个维度**（详见 `srcEntry-to-entrypoint-chain.md`）:

| 维度 | OHOS HAP ABC | ArkUI-X ABC |
|------|-------------|------------|
| bundleName 前缀 | **无** | **有** (`app.hackeris.example/`) |
| src/main/ 路径段 | **有** (`entry/src/main/ets/...`) | **无** (`entry/ets/...`) |
| 示例 | `entry/src/main/ets/entryability/EntryAbility` | `{bundleName}/entry/ets/entryability/EntryAbility` |

**两个维度的根因**:
1. **bundleName**: es2abc 编译时 OHOS 不加 bundleName 前缀；ArkUI-X 的 `GetOutEntryPoint` 运行时添加
2. **src/main/**: OHOS 的 hvigor 构建系统在编译时插入 `src/main/` 路径前缀；ArkUI-X 构建不插入

**对 entryPoint 匹配的影响**:
- GetOutEntryPoint 添加 bundleName → entryPoint 与 OHOS record 不匹配
- AdaptOldIsaRecord 截断 2 段 → 进一步破坏匹配
- 即使处理了前两个问题，src/main/ 段差异仍导致不匹配

### 阻塞点 2: AdaptOldIsaRecord 错误触发

即使去掉 bundleName 前缀，`AdaptOldIsaRecord` 会将不含 bundleName 的 entryPoint 视为"旧格式"，删除前两段路径:
```
bundleName/entry/src/main/ets/entryability/EntryAbility
→ src/main/ets/entryability/EntryAbility  (删除了 bundleName 和 entry)

或 entry/src/main/ets/entryability/EntryAbility（无 bundleName 版本）
→ ets/entryability/EntryAbility  (删除了 entry 和 src/main)
```

这进一步破坏了与 ABC 内部 record 名的匹配。

### 阻塞点 3: ExecuteModuleBuffer 符号不可见

ArkUI-X 的 `libarkui_android.so` 中 `ExecuteModuleBuffer` 是 hidden 符号，外部无法通过 dlsym 调用。但如果我们**自己构建 .so**，这个限制不存在。

### 阻塞点 4: @ohos: 系统模块依赖

OHOS HAP 引用的外部类（`@ohos:hilog`, `@ohos:app.ability.UIAbility` 等）在 Android 平台需要对应实现。ArkUI-X 已打包了部分模块（如 libhilog.so），但不完整。

### 阻塞点 5: HAP 文件读取

OHOS 从 HAP (ZIP) 文件系统路径读取 modules.abc 和 resources.index。Android 上 HAP 需要从 APK assets 或外部存储读取，需要适配 Asset 加载层。

## 三、技术方案

### 3.1 总体架构

```
                    ┌──────────────────────────────┐
                    │     Android APK               │
                    │                               │
                    │  StageActivity (Java)         │
                    │  StageApplication (Java)      │
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

### 3.2 关键代码修改点

#### Patch 1: GetOutEntryPoint 适配 + src/main/ 段保留

**文件**: `platform/common/module.cpp:21-25` (或 Android 平台对应文件)

**修改策略**: 新增 VM 标志位 `isOhosHapMode`，在加载 OHOS HAP 时设置。在此模式下:
- 不添加 bundleName 前缀（避免触发错误的 CheckIsRecordWithBundleName 和 AdaptOldIsaRecord）
- 保留或补充 `src/main/` 路径段，确保 entryPoint 与 ABC 内部 record 精确匹配

```cpp
CString GetOutEntryPoint(EcmaVM *vm, const CString &inputFileName) {
    if (vm->IsOhosHapMode()) {
        // OHOS HAP record 名格式: entry/src/main/ets/entryability/EntryAbility
        // inputFileName 来自运行时: entry/ets/entryability/EntryAbility.abc
        // 需将 ets/ 替换为 src/main/ets/ 以匹配 ABC record
        return NormalizeOhosEntryPoint(inputFileName);
    }
    return ConcatToCString(vm->GetBundleName(), "/", inputFileName);
}
```

#### Patch 2: ParseAbcPathAndOhmUrl 新增 OHOS HAP 子分支

**文件**: `module_path_helper.cpp:163-198`

**修改策略**: 对 OHOS HAP 模式的路径，在 Branch 3 中增加子分支，使用 OHOS 兼容的 entryPoint 生成逻辑。同时确保 final entryPoint 格式为 `entry/src/main/ets/xxx/xxx`。

```cpp
// Branch 3 增强 (相对路径)
else {
    if (vm->IsOhosHapMode() && IsOhosStylePath(inputFileName)) {
        // OHOS HAP 子分支: entryPoint 保留 src/main/，不加 bundleName
        outEntryPoint = NormalizeOhosEntryPoint(inputFileName);
    } else {
        // 原 ArkUI-X 逻辑
        outEntryPoint = GetOutEntryPoint(vm, inputFileName);
    }
}
```

#### Patch 3: AdaptOldIsaRecord 控制（双重保障）

**文件**: `js_pandafile_executor.cpp:176-181`

**修改策略**: 在 OHOS HAP 模式下跳过 AdaptOldIsaRecord。这是双重保障——Patch 1 已确保 entryPoint 不加 bundleName，即使此处未跳过也可通过 AdaptOldIsaRecord 后的模糊匹配工作，但跳过更安全。

```cpp
// 在 ExecuteModuleBuffer 中
if (!vm->IsOhosHapMode() && !vm->IsNormalizedOhmUrlPack() && !jsPandaFile->IsBundlePack()) {
    jsPandaFile->CheckIsRecordWithBundleName(entry);
    if (!jsPandaFile->IsRecordWithBundleName()) {
        PathHelper::AdaptOldIsaRecord(entry);
    }
}
// OHOS HAP mode: entry 原封不动传递给 CheckAndGetRecordInfo
```

#### Patch 4: HAP 读取集成 (已有基础)

**无需从零构建** — ArkUI-X 已内置 `HapParser` + minizip（`third_party/zlib/contrib/minizip/`），且已链接进 `libarkui_android.so`。

已有能力:
- `HapParser::ReadRawFileFromHap(hapPath, patchPath, rawFileName, len, outValue)` — 从 HAP (ZIP) 读取任意文件
- `HapParser::GetIndexDataFromHap(path, buf, bufLen)` — 读取 resources.index
- `HapParser::GetIndexData(path, buf, bufLen)` — 自动判断 .hap / 独立文件

**需要做的**: 将 `HapParser` 集成到 Android 的 `StageAssetProvider` 中，使 ABC/JSON/resource 读取路径在检测到 `.hap` 后缀时走 `HapParser` 而非 `AssetManager`。

集成方式（详见 `hap-resource-integration.md`）:
- **方式 A**: 在 `StageAssetProvider` 层增加 HAP 路径判断，`.hap` 结尾时调用 `HapParser::ReadRawFileFromHap`
- **方式 B**: 先解压 HAP 到临时目录，走现有文件系统路径
- **方式 C**: 新增 JNI 接口传入 HAP 路径，native 层自动切换

#### Patch 5: Asset 加载路由适配

**修改策略**: 在 `StageAssetProvider` / `StageAssetManager` 中增加路径判断逻辑。ABC 文件读取时，如果检测到 OHOS HAP 模式，走 `HapParser::ReadRawFileFromHap` → ZIP 提取路径；否则走原有的 AssetManager 路径。核心工作是从"构建新能力"缩减为"路由集成"。

### 3.3 构建方案

**策略**: 基于 ArkUI-X 的 build.sh + GN 构建系统，做最小化修改。

**核心依据** (详见 `arkui-x-build-system.md`):
- ArkUI-X **不 fork** OpenHarmony 核心代码，而是直接指向 OHOS master 分支的固定 tag 点，按周同步
- `arkcompiler/ets_runtime`、`foundation/arkui/ace_engine`、`arkcompiler/runtime_core` 等核心组件**代码完全相同**
- 差异仅在 adapter 层（Android/iOS 适配器）和 AppFramework 跨平台适配层
- 构建使用 GN + Ninja，通过 `ace_config.gni` 的平台动态发现机制为每个平台生成编译目标

**构建命令** (Android):
```shell
./build/prebuilts_download.sh --build-arkuix --skip-ssl   # 下载预编译工具链
./build.sh --product-name arkui-x --target-os android      # Android 编译
```

```
构建输入:
├── /src/ohos/arkcompiler/ets_runtime/    (与 ArkUI-X 指向同一份代码 ← 关键)
├── /src/ohos/arkcompiler/runtime_core/   (同一份)
├── /src/ohos/foundation/arkui/ace_engine/ (同一份)
├── /src/arkui-x/foundation/arkui/ace_engine/adapter/android/ (Android 适配层，独立)
├── /src/arkui-x/foundation/appframework/  (AppFramework 跨平台适配层，独立)
├── /src/ohos/base/global/resource_management/ (同一份)
├── /src/ohos/third_party/ (Skia, ICU, libpng, freetype...)
└── 新增: OHOS NAPI stubs

构建输出:
└── libohos_android.so  (单体 .so，类似 libarkui_android.so)
```

**具体步骤**:
1. 从 ArkUI-X 的 build.sh 提取 Android target 的 GN/CMake 参数
2. 将编译目标指向 `/src/ohos/arkcompiler/` 和 `/src/ohos/foundation/arkui/`（而非 ArkUI-X 的 fork）
3. 应用上述 Patch 到对应文件
4. 添加 OHOS NAPI stub 模块到构建
5. 编译 arm64-v8a 目标

**与 ArkUI-X 构建的关键差异**:
| 组件 | ArkUI-X 来源 | 本方案来源 | 原因 |
|------|-------------|-----------|------|
| arkcompiler | arkui-x/arkcompiler (指向 OHOS tag) | ohos/arkcompiler (同一代码) + Patch | 代码相同，需打 patch 适配 OHOS HAP record 格式 |
| ACE 核心 | arkui-x/foundation/arkui (指向 OHOS tag) | ohos/foundation/arkui (同一代码) | 代码相同，保证与 OHOS HAP 兼容 |
| Android 适配 | arkui-x/adapter/android | 使用 ArkUI-X 的适配层 | Android 渲染、窗口已成熟 |
| GetOutEntryPoint | arkui-x 实现(加 bundleName) | ohos 实现 + 新增 OHOS HAP 模式 | 解决 record 名不匹配 |

**注**: ArkUI-X 不是 fork，而是直接指向 OHOS master 分支的固定 tag 并按周同步。arkcompiler、ace_engine 等核心组件代码与 OHOS **完全相同**。差异仅在 Android/iOS adapter 层和 AppFramework 跨平台适配层。

### 3.4 @ohos: 模块依赖处理

**分层次处理**:

#### Level 1: 直接可用的 (ArkUI-X 已有)
- `@ohos:hilog` → libhilog.so ✓ (ArkUI-X 已打包)
- `@native:*` 引用 → NATIVE_MODULE，由 VM 内建解析，无需额外 .so ✓
- ArkUI 组件系统 ✓ (ACE 引擎提供)

#### Level 2: 需要移植的 (从 OHOS 源码构建)

基于测试 HAP（`entry-default-unsigned.hap`）的外部依赖分析：

| 依赖 | 类型 | 来源 |
|------|------|------|
| `@ohos:app.ability.UIAbility` | OHOS_MODULE | ability_runtime NAPI 绑定 |
| `@ohos:app.ability.ConfigurationConstant` | OHOS_MODULE | ability_runtime 配置常量 |
| `@ohos:application.BackupExtensionAbility` | OHOS_MODULE | 备份扩展基类 |

**注意**: `@ohos:curves`、`@ohos:matrix4`、`@ohos:app` 等在此测试 HAP 中以 `@native:` 和 `L-class` 形式引用，由 VM 内建处理，无需独立 .so。

#### Level 3: 可以跳过或 stub 的
- `EntryBackupAbility` — backup 类型 ExtensionAbility，Phase 2 验证时可直接跳过
- 非核心 API → 功能降级 stub，方法体返回空值

**Stub 注入方式**（详见 `napi-module-registration.md`）:
1. **静态注册** — `__attribute__((constructor))` + `napi_module_register()` 编译进 .so，加载时自动注册
2. **运行时 Register()** — 在 `CreateRuntime` 之后显式调用模块注册函数
3. **独立 .so dlopen** — 编译为独立 shared library，`FindNativeModuleByDisk` 按 `lib{name}.so` / `lib{name}_napi.so` 路径搜索

**验证简化** (详见 `test-hap-analysis.md`):
- 测试 HAP 仅 3 个模块记录 + 1 个页面
- modules.abc 仅 12KB
- Phase 2 聚焦 `EntryAbility` → `pages/Index` 路径，外部依赖从 10 个缩减到约 5 个（`hilog` 已有 + `@native:` 内建 + UIAbility + ConfigurationConstant + BackupExtensionAbility 可延后）

### 3.5 初始化流程

参考 OHOS 和 ArkUI-X 的初始化序列，设计 OHOS HAP on Android 的启动流程:

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

## 四、落地路线

### Phase 1: 构建验证（预期 1-2 周）

**目标**: 使用 `/src/ohos/arkcompiler/` 源码 + ArkUI-X 构建脚本，成功编译出可在 Android 上加载的 libohos_android.so

**关键任务**:
1. 提取 ArkUI-X build.sh 中 Android target 的编译参数
2. 配置 OHOS 源码路径为 arkcompiler 的源
3. 解决编译依赖（third_party 库路径、头文件路径）
4. 编译 arm64-v8a 目标，生成 .so
5. 编写最小 Android 测试 APK，加载 .so 并创建 EcmaVM

**验证标准**: 能在 Android 设备上成功 `System.loadLibrary` 并创建 EcmaVM 实例

### Phase 2: ABC 加载验证（预期 1-2 周）

**目标**: 加载 OHOS HAP 的 modules.abc 并成功执行 CommonExecuteBuffer

**关键任务**:
1. 集成 HapParser 实现 modules.abc 从 HAP 提取 (已有 minizip + HapParser，无需从零构建)
2. 应用 Patch 1-3 (GetOutEntryPoint, ParseAbcPathAndOhmUrl, AdaptOldIsaRecord)
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

**验证标准**: 测试 HAP 的 ability 能从 Java Activity 触发，JS onCreate() 被成功调用

### Phase 5: ArkUI 渲染集成（预期 2 周）

**目标**: 实现 HAP 页面在 Android 上的完整渲染

**关键任务**:
1. 测试 DeclarativeFrontendNG 是否能正确解析 OHOS HAP 中的页面 ABC
2. 验证 resources.index 加载（资源管理适配）
3. 验证 SurfaceView 渲染管线对 OHOS 页面的兼容性
4. 处理可能的 ACE 版本差异

**验证标准**: 测试 HAP 的首页能在 Android SurfaceView 中正确渲染

### Phase 6: 完整集成与测试（预期 2 周）

**目标**: 端到端集成，可运行完整的 OHOS HAP 应用

**关键任务**:
1. 端到端流程贯通: APK 安装 → 启动 → HAP 解包 → ABC 加载 → 页面渲染
2. 测试多个 HAP 样本（简单页面 → 复杂交互）
3. 性能基准测试（与 OHOS 原生和 ArkUI-X 对比）
4. 文档和工具链完善

## 五、风险与缓解

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|---------|
| OHOS 和 ArkUI-X 的 ACE 版本差异过大 | 中 | 高 | 优先使用 OHOS 源码 + ArkUI-X Android 适配层 |
| @ohos: 依赖过多无法全部 stub | 中 | 中 | 分 tier 处理，优先解决高频依赖 |
| ABC 格式差异超出预期 | 低 | 高 | Phase 2 提前验证，如失败考虑 ABC 重编译方案 |
| ArkUI-X 构建脚本与 OHOS 源码不兼容 | 中 | 中 | 可从 ArkUI-X 的 gn 参数出发逐步适配 |
| 性能劣化（翻译执行 vs AOT） | 低 | 中 | ArkUI-X 已启用 AOT stub，可复用 |

## 六、替代方案（如果上述路径受阻）

### Plan B: 从 ETS 源码重编译 ABC

- 获取 HAP 的 ETS 源码，用 ArkUI-X 的 es2abc 工具链重新编译成 ArkUI-X 兼容格式的 .abc
- 优点: 不需要修改运行时
- 缺点: 需要 ETS 源码；本质上是"用 ArkUI-X 重组一个 App"而非"直接运行已编译的 HAP"；与项目目标矛盾

### Plan C: 基于 ArkUI-X fork 扩展
- 不修改 OHOS 原版代码，而是将 OHOS HAP 兼容层作为 ArkUI-X 的扩展
- 添加 OHOS HAP 格式适配器 + 路径仿真（模拟 OHOS 文件系统布局）
- 优点: 改动更收敛
- 缺点: 可能无法支持所有 OHOS 特性
