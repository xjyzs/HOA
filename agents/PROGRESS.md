# HOA 项目进展

## 当前状态

**里程碑**: OHOS HAP "Hello World" 在 Android 设备上端到端渲染成功（2026-05-15）。

完整用户流程已跑通：APK 安装 → MainActivity → 选择 HAP 文件 → 安装 → 点击启动 → HoaAbilityActivity → Hello World 渲染。

下一个目标：解决 `@ohos:` NAPI 模块依赖，完善 Ability 生命周期。

---

## 已验证的能力

| 能力 | 状态 | 说明 |
|------|------|------|
| ArkUI-X 构建（build.sh） | ✅ | 基于 ArkUI-X 原生 GN 构建系统，产出 libarkui_android.so (79MB) |
| 产物同步（sync_arkui_x.sh） | ✅ | 一键复制所有构建产物到 HOA 项目（~90 文件） |

| HAP 解析与安装 | ✅ | HapBundleLoader 解压 HAP → HapInstaller 写入 filesDir/arkui-x/ |
| ETS VM 创建 | ✅ | `ark::ets::CreateRuntime()` 成功，ES module 模式 |
| OHOS ABC 加载 | ✅ | modules.abc 中的 EntryAbility + Index 均加载成功 |
| ABC record 名匹配 | ✅ | 4 个 git 仓库 12 个文件的定向 Patch 解决双维度差异 |
| ArkUI 渲染 | ✅ | StageActivity + SurfaceView + Skia 管线正常 |
| MainActivity HAP 管理 | ✅ | 安装 / 列表 / 启动 / 卸载 功能完整 |

---

## 关键突破

### 1. ABC Record 名匹配（2026-05-15）

**问题**: OHOS ABC record 名使用 `&` 归一化 URL 格式（`&entry/src/main/ets/EntryAbility&`），
但 ArkUI-X 的 `GetOutEntryPoint` 返回不含 `&` 的 entry point，导致 `CheckAndGetRecordInfo` 精确匹配失败。

**修复**: `GetOutEntryPoint` 在 OHOS 模式下用 `NORMALIZED_OHMURL_TAG`（`&`）包裹输出。
详见 `ARKUI-X_PATCHES.md`。

### 2. MainActivity 启动白屏修复（2026-05-15）

**问题**: 从 MainActivity 点击启动 HoaAbilityActivity 白屏，但 adb am start 直接启动正常。

**根因**: `HapInstaller.getInstalledHaps()` 用 `indexOf('.')` 拆分目录名
`app.hackeris.harmonyexample.entry`，bundleName 含点号导致拆出错误的
bundleName="app" 和 moduleName="hackeris.harmonyexample.entry"。

**修复**:
1. `HapInstaller.kt:122`: `indexOf('.')` → `lastIndexOf('.')` （根因修复）
2. `MainActivity.kt:launchHap()`: 添加 `FLAG_ACTIVITY_NEW_TASK | FLAG_ACTIVITY_CLEAR_TASK`（StageActivity 需要作为 task root）

---

## ArkUI-X 源码修改

共修改 4 个 git 仓库、约 12 个文件（含 JNI/Java 桥接、GN 构建模板），通过 `isOhosHapMode` VM 标志位驱动。
完整清单见 `docs/ARKUI-X_PATCHES.md`。

| 仓库 | 修改内容 |
|------|----------|
| `openharmony/arkcompiler_ets_runtime` | VM 标志位、GetOutEntryPoint（核心）、模块路由、AdaptOldIsaRecord 跳过 |
| `arkui-x/app_framework` | 环境变量 → VM 标志接入 |
| `arkui-x/arkui_for_android` | JNI setenv 桥接、StageApplication Java API、native 声明 |
| `openharmony/build` | GN 构建模板适配（external_deps、arm64e、PAC） |

---

## 构建与工具链

### 构建流程

```bash
# ArkUI-X 原生构建
cd /data/share/hoa2/arkui-x
./build.sh --product-name arkui-x --target-os android

# 产物同步到 HOA 项目
cd /path/to/HOA
./scripts/sync_arkui_x.sh

# APK 打包
./gradlew assembleDebug
```

### 产物同步脚本

`scripts/sync_arkui_x.sh` 一键复制所有 ArkUI-X 构建产物（~90 文件），支持：

### 构建流程

```bash
# ArkUI-X 原生构建
cd /data/share/hoa2/arkui-x
./build.sh --product-name arkui-x --target-os android

# 产物同步到 HOA 项目
cd /path/to/HOA
./scripts/sync_arkui_x.sh

# APK 打包
./gradlew assembleDebug
```

---

## 测试 HAP

当前使用 OHOS 原生格式 HAP：

| 属性 | 值 |
|------|-----|
| 文件 | `assets/hap/entry.hap` (123KB) |
| bundleName | `app.hackeris.harmonyexample` |
| compileMode | `esmodule` |
| modules.abc | 12.8KB，3 个 record（含 backup extensionAbility） |
| srcEntry | `./ets/entryability/EntryAbility.ets` |
| ABC record 格式 | 无 bundleName 前缀，有 `src/main/`，`&` 包裹 |

ArkUI-X 格式 HAP（`/data/share/arkui-x-example/`）曾用于早期基线验证，现已不再需要。

---

## 已完成

### 项目基础设施

- `scripts/sync_arkui_x.sh` — ArkUI-X 产物同步脚本
- `docs/BUILD.md` — 完整构建文档（含 sync 脚本用法、产物清单、故障排查）
- `docs/ARKUI-X_PATCHES.md` — ArkUI-X 源码修改说明（4 个仓库、12 个文件）
- `agents/PLAN.md` — 技术方案与落地路线
- `agents/` — 技术调研文档（ABC 格式、模块路由、HAP 启动流程等）

### HAP 管理（Android 端）

- `HapBundleLoader` — HAP 文件解析（ZIP → module.json + .abc + resources）
- `HapInstaller` — HAP 安装/卸载/列表，写入 `filesDir/arkui-x/`
- `HapExtractor` — 从 `assets/hap/` 解压内嵌 HAP 到安装目录
- `MainActivity` — 完整的 HAP 管理 UI（系统文件选择器安装、列表、启动、长按卸载）

### 启动流程

- `HoaApplication` — 继承 StageApplication，onCreate 中设 OHOS 模式 + 解压 HAP
- `HoaAbilityActivity` — 继承 StageActivity，接收 Intent extras 构造 instanceName

---

## 待完成

### 短期

- 解决 `@ohos:` NAPI 模块依赖（`@ohos:app.ability.UIAbility` 等）
- 完善 Ability 生命周期回调（onCreate → onWindowStageCreate → onForeground）
- 处理 `@ohos:hilog` 之外的其他系统模块

### 中期

- 多 HAP 并存管理
- 资源文件（resources.index）从 HAP 读取集成
- 其他测试 HAP 样本验证

### 已知问题（非阻塞）

- `stage_asset_provider.cpp:663` 报 `read file failed` 但不影响渲染
- `libimonitor.so` 加载警告（OHOS 特有库，Android 不存在）

---

## 文档索引

| 文档 | 说明 |
|------|------|
| `agents/PLAN.md` | 完整技术方案、阻塞点分析、替代方案 |
| `docs/BUILD.md` | 构建文档、sync 脚本用法、产物清单 |
| `docs/ARKUI-X_PATCHES.md` | ArkUI-X 源码修改详细说明 |
| `scripts/sync_arkui_x.sh` | 产物同步脚本 |
