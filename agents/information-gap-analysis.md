# 信息充分性评估与缺口分析

## 交叉验证方法

所有关键声明均通过**双向源码对比**（`/src/ohos/` ↔ `/src/arkui-x/`）进行验证，确保不依赖单一代码源。

---

## 已验证的关键声明

| # | 声明 | OHOS 源码证据 | ArkUI-X 源码证据 | 结果 |
|---|------|-------------|-----------------|:---:|
| 1 | `GetOutEntryPoint` 添加 bundleName 前缀 | **无此函数** — 使用 `ParseAbcPathAndOhmUrl` 内联逻辑 (Branch 3, 5 个子分支) | `platform/common/module.cpp:24`: `ConcatToCString(vm->GetBundleName(), "/", inputFileName)` | ✅ |
| 2 | `AdaptOldIsaRecord` 截断前 2 段路径 | `base/path_helper.h:71-80`: 两次 `find(SLASH_TAG)` 后 `substr(pos+1)` | 同 OHOS | ✅ |
| 3 | `CheckIsRecordWithBundleName` 比对 entry 首段与 ABC record 首段 | `js_pandafile.cpp:68-89`: 取 entry 第一个 `/` 前为 candidate bundleName，遍历 record 比对 | 同 OHOS (行 133-155，使用结构化绑定) | ✅ |
| 4 | Bundle 模式跳过 AdaptOldIsaRecord | `js_pandafile_executor.cpp:217`: `else if (!isBundle)` | 同 OHOS (行 176，简化了条件) | ✅ |
| 5 | `ParseAbcPathAndOhmUrl` Branch 3 的差异 | 5 个子分支：Windows/Mac/OHOS/Preview/.test | **简化为单行调用**: `GetOutEntryPoint(vm, inputFileName)` (行 189) | ✅ |
| 6 | `ExecuteModuleBuffer` 在 OHOS 和 ArkUI-X 中逻辑等价 | 行 180-233: `ParseAbcPathAndOhmUrl` → `LoadJSPandaFile` → `CheckAndGetRecordInfo` → `CommonExecuteBuffer` | 行 153-192: 相同流程，`CheckAndGetRecordInfo` 直接判断 nullptr | ✅ |
| 7 | OHOS HAP record 名含 `src/main/` 前缀 | `entry/src/main/ets/entryability/EntryAbility` (来自 test HAP 分析) | N/A (ArkUI-X record 名格式不同) | ✅ |
| 8 | ArkUI-X 直接复用 OHOS 核心代码 | N/A | `arkui-x-build-system.md` 已确认：指向 OHOS 固定 tag，非 fork | ✅ |

### 关键验证发现

**OHOS vs ArkUI-X 的入口路由差异根因**：不是代码逻辑不同，而是运行时条件不同：

- **OHOS**: 主路径为 Bundle 模式（`isBundle=true`），`ParseAbcPathAndOhmUrl` Branch 1（绝对路径 `/data/storage/el1/bundle/...`），跳过了 AdaptOldIsaRecord
- **ArkUI-X (Android)**: 主路径为 ES Module 模式（`isBundle=false`），Branch 3（相对路径），触发 `GetOutEntryPoint` + `AdaptOldIsaRecord`

这意味着如果我们在构建时**不启用 Bundle 模式**（OHOS HAP 本身也是 esmodule 编译模式），就需要让 Branch 3 的路径处理兼容 OHOS 格式的 record 名。

---

## 信息充分性评估

### ✅ 已具备、可直接用于落地

| 信息 | 状态 | 支撑阶段 |
|------|------|---------|
| 阻塞点精确定位（GetOutEntryPoint / AdaptOldIsaRecord / ParseAbcPathAndOhmUrl） | 完整，有具体行号 | Phase 1-2 |
| **srcEntry → entryPoint 完整转换链** | **✅ 已解决** — 逐行追踪 OHOS + ArkUI-X 源码，识别双维度差异 | Phase 2 |
| 测试 HAP 依赖清单（10 个外部依赖，分层处理） | 完整 | Phase 3 |
| 构建体系（GN 参数、config.gni、依赖树、编译命令） | 完整 | Phase 1 |
| 渲染管线（Rosen + Skia + SurfaceView） | 可复用 ArkUI-X | Phase 5 |
| Stage 模型架构（单引擎共享、生命周期、Context 体系） | 完整 | Phase 4 |
| ABC 格式与跨平台兼容性 | 验证通过 | Phase 2 |
| libarkui_android.so 完整依赖树 | 完整 | Phase 1 |

### ⚠️ 存在缺口、需补充

| # | 缺口 | 影响等级 | 阻塞阶段 |
|---|------|:---:|:---:|
| ~~1~~ | ~~srcEntry → entryPoint 完整转换链~~ | ~~高~~ | **已解决** → `srcEntry-to-entrypoint-chain.md` |
| ~~2~~ | ~~JNI 桥接实现细节~~ | ~~中高~~ | **已解决** → `jni-bridge-analysis.md` |
| ~~3~~ | ~~NAPI 模块注册与注入机制~~ | ~~中~~ | **已解决** → `napi-module-registration.md` |
| ~~4~~ | ~~符号可见性控制方式~~ | ~~中~~ | **已解决** → `symbol-visibility-control.md` |
| ~~5~~ | ~~HAP ZIP 读取库与集成点~~ | ~~中低~~ | **已解决** → `hap-resource-integration.md` |
| ~~6~~ | ~~resource.index 格式与加载流程~~ | ~~低~~ | **已解决** → `hap-resource-integration.md` |

---

## 缺口详细说明

### 缺口 1: ~~srcEntry → entryPoint 完整转换链~~ ✅ 已解决

**结论文档**: `srcEntry-to-entrypoint-chain.md`

通过逐行追踪 OHOS 和 ArkUI-X 两套源码，完整还原了从 `module.json5` 的 `srcEntry` 到 `CheckAndGetRecordInfo` 的每一步参数变换：

**核心发现**:
- record 名差异有**两个维度**，不仅是 bundleName 有无:
  1. **bundleName**: ArkUI-X record 有，OHOS HAP 无
  2. **src/main/**: OHOS HAP record 有（构建系统插入），ArkUI-X 无
- AdaptOldIsaRecord 截断 2 段后仍不匹配 OHOS 格式
- Patch 策略需三个层面同时处理

**对技术方案的修正**: 仅处理 GetOutEntryPoint 和 AdaptOldIsaRecord 不够，还需处理 src/main/ 路径段的差异。

### 缺口 2: ~~JNI 桥接实现细节~~ ✅ 已解决

**结论文档**: `jni-bridge-analysis.md`

通过逐文件追踪两个 JNI 注册表、AppMain、Application、AbilityStage、JsAbility、JsRuntime 的完整调用链，明确了以下关键点：

**核心发现**:
- **两个 JNI 渠道**: StageApplicationDelegate（22 个方法：初始化、资源路径、LoadModule）+ StageActivityDelegate（10 个方法：生命周期分发、WindowView）
- **instanceName 格式**: `bundleName:moduleName:abilityName:instanceId`，通过 `TransformToWant` 按 `:` 分割解析
- **两条 ABC 加载路径**: 
  1. `nativeDispatchOnCreate` → `HandleDispatchOnCreate` → `Application::HandleAbilityStage` → `AbilityStage::LaunchAbility` → `JsAbility::Init` → `JsRuntime::LoadModule`
  2. `nativeLoadModule` → `LoadModuleHelper::LoadModule` → `JsRuntime::LoadModule`
- **ArkUI-X 的 modulePath 构造**: `package + "/" + srcEntrance` → 相对路径（如 `entry/./ets/...`），与 OHOS 的绝对路径不同
- **Asset 通道**: 所有文件（module.json, ABC, resources）通过 Android AssetManager 提供，复制到 `filesDir/arkui-x/`
- **entryFile 格式约束**: Java 层有正则 `^\./ets/([^/]+/)*[^/]+$` 校验

### 缺口 3: ~~NAPI 模块注册与注入机制~~ ✅ 已解决

**结论文档**: `napi-module-registration.md`

通过分析 `NativeModuleManager` 的完整加载链，明确了以下关键点：

**核心发现**:
- **静态注册机制**: NAPI 模块通过 `__attribute__((constructor))` + `napi_module_register()` 在 .so 加载时自动注册
- **三路径搜索**: `FindNativeModuleByDisk` 先尝试 `lib{name}.so`，再 `lib{name}_napi.so`，最后 `.abc` 文件
- **Android 强制 isAppModule**: 所有模块在 Android 上以 app module 方式加载
- **三种 stub 注入方式**: 静态注册（编译进 .so）、运行时 Register()、独立 .so dlopen
- **OHOS 模块命名**: `nm_modname = "app.ability.UIAbility"` — 与 `@ohos:app.ability.UIAbility` 对应
- **Android 路径规则**: 模块名小写、`lib` 前缀、`_napi` 后缀变量

### 缺口 4: ~~符号可见性控制方式~~ ✅ 已解决

**结论文档**: `symbol-visibility-control.md`

**核心发现**:
- ArkUI-X 使用 **linker version script**（`libark_jsruntime.map`）控制符号导出，而非 `-fvisibility=hidden`
- `local: *;` 使所有未列出的符号变为 hidden
- `ExecuteModuleBuffer`、`ParseAbcPathAndOhmUrl` 等函数不在导出列表中 → dlsym 不可见
- OHOS 和 Android 目标共用同一个 `.map` 文件
- 自己构建时可移除 version_script 或定制符号导出列表
- **但实际不需要**: 所有 Patch 代码通过内部调用链触发，不依赖 dlsym

### 缺口 5: ~~HAP ZIP 读取库与集成点~~ ✅ 已解决

**结论文档**: `hap-resource-integration.md`

**核心发现**: ArkUI-X 已内置 `HapParser` + minizip（`third_party/zlib/contrib/minizip/`）→ `ReadRawFileFromHap` 可从 HAP (ZIP) 读取任意文件。无需从零构建 HapReader。

### 缺口 6: ~~resource.index 格式与加载~~ ✅ 已解决

**结论文档**: `hap-resource-integration.md`

**核心发现**: `HapParser::GetIndexDataFromHap` 已实现 resources.index 提取。`resource_management` 模块已完整实现格式解析。此 gap 的工作从"构建新能力"缩减为"路由集成"。

---

## 缺口处理状态

| # | 缺口 | 状态 | 产出文档 |
|:---:|------|:---:|------|
| 1 | srcEntry → entryPoint 转换链 | ✅ 已解决 | `srcEntry-to-entrypoint-chain.md` |
| 2 | JNI 桥接实现细节 | ✅ 已解决 | `jni-bridge-analysis.md` |
| 3 | NAPI 模块注册与注入 | ✅ 已解决 | `napi-module-registration.md` |
| 4 | 符号可见性控制 | ✅ 已解决 | `symbol-visibility-control.md` |
| 5 | HAP ZIP 读取库与集成 | ✅ 已解决 | `hap-resource-integration.md` |
| 6 | resource.index 格式 | ✅ 已解决 | `hap-resource-integration.md` |

**全部 6 个缺口已解决**，可支撑 Phase 1-5 实施。
