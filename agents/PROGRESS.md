# HOA 项目进展

## 当前状态

**里程碑**: wan-harmony（玩安卓）HAP 在 HOA 运行时正常渲染，权限流程贯通（2026-05-16）。

完整用户流程已跑通：APK 安装 → MainActivity → 选择 HAP 文件 → 安装 → 点击启动 → HoaAbilityActivity → 首页 Tab 正常显示（含 6 个 Tab 页）。

**上一个里程碑**（2026-05-15）: OHOS HAP "Hello World" 端到端渲染成功。

---

## 已验证的能力

| 能力 | 状态 | 说明 |
|------|------|------|
| ArkUI-X 构建（build.sh） | ✅ | 基于 ArkUI-X 原生 GN 构建系统，产出 libarkui_android.so (79MB) |
| 产物同步（sync_arkui_x.sh） | ✅ | 一键复制所有构建产物到 HOA 项目（~100+ 文件，含 12 插件 + 7 传递依赖） |
| HAP 解析与安装 | ✅ | HapBundleLoader 解压 HAP → HapInstaller 写入 filesDir/hap/ |
| ETS VM 创建 | ✅ | `ark::ets::CreateRuntime()` 成功，ES module 模式 |
| OHOS ABC 加载 | ✅ | modules.abc 中的 EntryAbility + Index 均加载成功 |
| ABC record 名匹配 | ✅ | 4 个 git 仓库 12 个文件的定向 Patch 解决双维度差异 |
| ArkUI 渲染 | ✅ | StageActivity + SurfaceView + Skia 管线正常 |
| MainActivity HAP 管理 | ✅ | 安装 / 列表 / 启动 / 卸载 功能完整 |
| @ohos NAPI 模块全量加载 | ✅ | 12 个插件 .so 正常加载，无 "export undefined" 报错 |
| OHOS → Android 权限映射 | ✅ | INTERNET（普通权限）绕过 JNI 直接授予，危险权限走运行时流程 |
| wan-harmony HAP 端到端运行 | ✅ | 首页入口正常，6 个 Tab 可切换 |
| 最近任务列表显示 HAP 名称和图标 | ✅ | `setTaskDescription()` 动态设置，替换 "HOA" 标签 |
| MainActivity HAP 列表图标 | ✅ | 从 module.json 解析 startWindowIcon → 后台加载 → 缓存 |
| .hap 文件关联 | ✅ | intent filter 注册，文件管理器/分享均可直接安装 |

---

## 关键突破

### 1. 权限流程修复（2026-05-16）

**问题**: wan-harmony HAP 首页显示 "No Permission"，无法进入 Tab 界面。根因是 `EntryAbility` 的 `aboutToAppear()` 中调用 `OHPermission.requestPermission(['ohos.permission.INTERNET'])`，响应式 Promise 永远不 resolve。

**根因分析**:
1. `ohos.permission.INTERNET` 映射为 `android.permission.INTERNET`
2. 预检查通过 JNI `CheckPermission()` 调用 Java 层，但此时 Java 的 `AbilityAccessCtrl` 对象尚未初始化（`g_pluginClass.globalRef` 为 null），JNI 调用静默返回 false
3. 权限进入 `Activity.requestPermissions()` 运行时流程，但 INTERNET 是 Android "普通权限"（normal），安装时自动授予，永不触发 `onRequestPermissionsResult()` 回调
4. 回调不触发 → Promise 不 resolve → `isPermissionGrant` 保持 false → 界面显示 "No Permission"

**修复** (`ability_access_ctrl_impl.cpp`):
- 新增 `g_normalPermissions` 集合，收录已知的 Android 普通权限
- `RequestPermissions()` 中，对命中该集合的权限直接标记 `PERMISSION_GRANTED`，不经过 JNI 检查也不进入运行时流程
- 同时补充 `ohos.permission.INTERNET` 到 `g_permissionMap` 的映射条目

### 2. @ohos NAPI 插件全量补齐（2026-05-16）

**问题**: wan-harmony HAP 启动时报 "export objects of native so is undefined" 错误，多个 @ohos 模块无法加载。

**修复**:
- `sync_arkui_x.sh` A4 段扩展为 12 个插件 .so（hilog、abilityAccessCtrl、data.preferences、net.http、net.connection、net.socket、net.websocket、uri、url、util、util.HashMap、web.webview）
- A5 段新增 7 个传递依赖 .so（net_utils、curl、nghttp2、crypto_openssl、ssl_openssl、libxml2、libshared_libz）
- `build.gradle.kts` 添加 `ace_web_webview_android.jar`

### 3. ABC Record 名匹配（2026-05-15）

（已在上一个里程碑完成，详见 `docs/ARKUI-X_PATCHES.md`）

### 4. MainActivity 启动白屏修复（2026-05-15）

（已在上一个里程碑完成，详见 `docs/ARKUI-X_PATCHES.md`）

---

## ArkUI-X 源码修改

共修改 5 个文件（含本次新增的权限映射修复）：

| 文件 | 修改内容 |
|------|----------|
| `arkcompiler_ets_runtime/...` | VM 标志位、GetOutEntryPoint（核心）、模块路由、AdaptOldIsaRecord 跳过 |
| `arkui-x/app_framework/...` | 环境变量 → VM 标志接入 |
| `arkui-x/arkui_for_android/...` | JNI setenv 桥接、StageApplication Java API、native 声明 |
| `openharmony/build/...` | GN 构建模板适配 |
| `arkui-x/plugins/ability_access_ctrl/.../ability_access_ctrl_impl.cpp` | **新增**：普通权限绕过 JNI 直接授予 |

完整清单见 `docs/ARKUI-X_PATCHES.md`。

---

## 构建与工具链

### 构建流程

```bash
# ArkUI-X 原生构建
cd <arkui-x-source>
./build.sh --product-name arkui-x --target-os android

# 产物同步到 HOA 项目
cd <hoa-project>
./scripts/sync_arkui_x.sh

# APK 打包
./gradlew assembleDebug
```

### 产物同步脚本

`scripts/sync_arkui_x.sh` 一键复制所有 ArkUI-X 构建产物（~100+ 文件），支持：
- `--so-only` — 仅 .so 文件（C++ 修改后快速更新）
- `--abc-only` — 仅 systemres .abc
- `--res-only` — 仅资源文件
- `--dry-run` — 预览模式

---

## 测试 HAP

### wan-harmony（玩安卓）

| 属性 | 值 |
|------|-----|
| 源码 | `/data/share/wan-harmony` |
| bundleName | `top.wangchenyan.wanharmony` |
| moduleName | `entry` |
| mainAbility | `EntryAbility` |
| modules.abc | 748KB |
| Tab 页 | 首页、鸿蒙、广场、体系、公众号、我的（6 个） |
| 依赖的 @ohos 模块 | hilog, abilityAccessCtrl, data.preferences, net.http, uri, url, util.HashMap, web.webview, window, promptAction, bundleManager, app.ability.UIAbility |
| 状态 | ✅ 正常渲染 |

### Hello World（内嵌测试 HAP）

| 属性 | 值 |
|------|-----|
| 文件 | `assets/hap/entry.hap` (123KB) |
| bundleName | `app.hackeris.harmonyexample` |
| 状态 | ✅ 正常渲染（DevTestActivity 一键启动） |

---

## 已完成

### 项目基础设施

- `scripts/sync_arkui_x.sh` — ArkUI-X 产物同步脚本
- `docs/BUILD.md` — 完整构建文档
- `docs/ARKUI-X_PATCHES.md` — ArkUI-X 源码修改说明
- `agents/PLAN.md` — 技术方案与落地路线
- `agents/` — 技术调研文档

### HAP 管理（Android 端）

- `HapBundleLoader` — HAP 文件解析
- `HapInstaller` — HAP 安装/卸载/列表
- `HapExtractor` — 从 assets 解压内嵌 HAP
- `MainActivity` — HAP 管理 UI
- `DevTestActivity` — 一键提取并启动测试 HAP

### 启动流程

- `HoaApplication` — 继承 StageApplication，设置 OHOS 模式
- `HoaAbilityActivity` — 继承 StageActivity，多进程隔离
- `applyHapTaskDescription()` — 从 HAP 的 module.json 解析应用名称和 startWindowIcon，动态设置 Activity 标题和任务描述（最近任务列表显示 HAP 身份）

### UI 体验

- 最近任务列表：动态显示 HAP 的应用名称和图标（替换 "HOA"）
- MainActivity HAP 列表：每个 HAP 显示对应的 startWindowIcon
- .hap 文件关联：从文件管理器直接打开或分享安装 HAP

### NAPI 模块支持

| 模块 | .so |
|------|-----|
| @ohos.hilog | libhilog.so |
| @ohos.abilityAccessCtrl | libabilityaccessctrl.so |
| @ohos.data.preferences | libdata_preferences.so |
| @ohos.net.http | libnet_http.so |
| @ohos.net.connection | libnet_connection.so |
| @ohos.net.socket | libnet_socket.so |
| @ohos.net.websocket | libnet_websocket.so |
| @ohos.uri | liburi.so |
| @ohos.url | liburl.so |
| @ohos.util | libutil.so |
| @ohos.util.HashMap | libutil_hashmap.so |
| @ohos.web.webview | libweb_webview.so |

---

## 待完成

### 短期

- 解决 WebView 插件 init 失败（`NoSuchMethodException: AceWebBase.<init>` — JAR 版本不匹配）
- Vulkan RenderContext 创建失败时明确回退到 OpenGL ES（当前有 fallback 但不透明）
- `@ohos.pulltorefresh` 第三方 ohpm 包支持（wan-harmony 多个页面依赖）
- `@ohos.promptAction` 插件补齐（Mine、UserService 页面依赖）

### 中期

- 完善 Ability 生命周期回调（onCreate → onWindowStageCreate → onForeground）
- 多 HAP 并存管理
- 其他测试 HAP 样本验证

### 已知问题（非阻塞）

| 问题 | 影响 | 说明 |
|------|------|------|
| Pad 窗口模式标题栏显示 "HOA" | 窗口标题栏 | Android `android:label`/`icon` 安装时由 PackageManager 固化，运行时无法动态修改（微信小程序同获此限制） |
| Vulkan RenderContext 返回 nullptr | 首次渲染可能闪烁 | 设备不支持 Vulkan，走 GLES fallback |
| WebView 插件 init 失败 | WebPage 页面无法使用 | `AceWebBase.<init>` 构造函数签名不匹配 |
| `stage_asset_provider.cpp` read file failed | 无 | 日志噪音，不影响渲染 |
| `bundleInfo_ is nullptr` | 无 | HOA 未实现完整 Bundle Manager |

---

## 文档索引

| 文档 | 说明 |
|------|------|
| `agents/PLAN.md` | 完整技术方案、阻塞点分析、替代方案 |
| `docs/BUILD.md` | 构建文档、sync 脚本用法、产物清单 |
| `docs/ARKUI-X_PATCHES.md` | ArkUI-X 源码修改详细说明 |
| `scripts/sync_arkui_x.sh` | 产物同步脚本 |
| `agents/PROGRESS.md` | 本文件，项目进展总览 |
