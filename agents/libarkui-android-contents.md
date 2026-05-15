# libarkui_android.so 构建内容分析

**源码路径**: `/src/arkui-x/`

## 构建入口

**GN 目标**: `/src/arkui-x/foundation/arkui/ace_engine/adapter/android/build/BUILD.gn` (line 26)

```gn
ohos_shared_library("libarkui_android") {
```

使用 `ohos_shared_library` 模板（来自 `//build/ohos.gni`），链接方式是"单体 .so"——所有依赖以静态库/source_set 形式打包，最终产物 ~95MB。

---

## 一级直接依赖

libarkui_android 的直接 deps 只有 5+2 项：

| 依赖 | 来源 | 职责 |
|------|------|------|
| `libace_static_android` | ace_engine/build | **主体静态库**，包含所有 ace_engine 源码 |
| `ace_kit` | interfaces/inner_api/ace_kit | UI 框架 API 抽象层 |
| `ace_static_ndk` | interfaces/native | NDK 接口（XComponent, native node） |
| `appframework_napis` | foundation/appframework | App 框架 NAPI 模块（ability, window） |
| `ace_plugin_util_inner_android` | plugins/interfaces/native | 插件工具层 |
| `libarkbase_static` (条件) | arkcompiler/runtime_core | Panda VM 基础库（Ark 引擎启用时） |
| `libarkfile_static` (条件) | arkcompiler/runtime_core | Panda 文件格式库（Ark 引擎启用时） |

---

## 二级：libace_static_android（主体）

通过 `foreach(ace_platforms)` 循环生成，对 Android 展开为 `ohos_source_set`，包含：

### 核心 Framework（平台无关）

| 模块 | 路径 | 内容 |
|------|------|------|
| `ace_base_android` | frameworks/base | 基础工具：string_utils, log, 内存管理, 事件总线 |
| `framework_bridge_ng_android` | frameworks/bridge | 前端桥接层：JS/ArkTS ↔ C++ 通信、声明式前端桥 |
| `ace_core_ng_android` | frameworks/core | **核心渲染引擎**：组件 NG 模型、布局、绘制、动画、事件 |
| `declarative_js_engine_ng_ark_android` | frameworks/bridge/declarative_frontend | ArkTS 声明式 JS 引擎（内建 JS 引擎，非 V8） |

### 平台依赖（Android 适配层，来自 config.gni platform_deps）

| 模块 | 路径 | 源文件数 | 内容 |
|------|------|---------|------|
| `ace_common_jni_android` | adapter/android/entrance/java/jni | 37+ | JNI 桥接：应用信息、Display 管理、触摸事件、剪贴板、字体、存储、Web、折叠屏、UDMF、振动、上报、子窗口 |
| `ace_osal_android` | adapter/android/osal | 80+ | **OS 抽象层**：无障碍、Display、输入法、图形(pixel_map, image_packer, image_source)、资源管理、窗口管理、文件 I/O、WebSocket |
| `stage_android_jni_android` | adapter/android/stage/ability/java/jni | 9 | Stage 模型 JNI：AbilityContext, ApplicationContext, Activity/Fragment Delegate, AssetProvider, WindowViewAdapter |
| `ace_uicontent_android` | adapter/android/stage/uicontent | 4 | UI 内容管理：AceContainerSG, AceViewSG, UIContentImpl, UIEventMonitor |

---

## 三级：关键子系统

### ace_osal_android（OS 抽象层 — 最大模块）

**文件**：`adapter/android/osal/BUILD.gn`

80+ 个 C++ 源文件，提供 Android 平台所有 OS 级能力实现：

| 类别 | 实现 |
|------|------|
| 无障碍 | accessibility_event_manager, js_accessibility, type_convertor |
| Display | display_manager_android, display_info_utils |
| 输入 | input_manager, input_method_manager_android, mouse_style_android |
| 图形 | pixel_map_android, image_packer_android, image_source_android, drawing_color_filter_android |
| 资源 | resource_adapter_impl_v2, resource_convertor, resource_theme_style |
| 窗口 | subwindow_android, drag_window, modal_ui_extension_impl |
| 系统 | system_properties, trace, event_reporting, frame_reporting, websocket |
| 文件 | file_asset_provider, file_uri_helper_android |

**osal 自身依赖链**：
```
ace_osal_android
├── ace_common_jni_android        (JNI 桥)
├── global_resmgr                 (全局资源管理)
├── resmgr_napi_core              (资源 NAPI 绑定)
├── resourcemanager               (资源管理器)
├── rawfile                       (原始文件访问)
├── dm                            (Display Manager, from appframework)
├── hiviewdfx_hilog_base          (日志)
├── librender_service_client_static (Rosen 渲染后端)
└── libmmi-client-crossplatform   (多模输入)
```

### ace_common_jni（JNI 桥接层）

**capability C++ 文件**（来自 `capability.gni`）：
bridge_jni, clipboard, text_editing, environment, font, grant_result, storage, vibrator, plugin_manager, image/texture

### ArkCompiler 运行时

**条件编译**：当 `ark_engine` 启用时（Android 默认启用）：
- `libarkbase_static` — Panda VM 基础（runtime_core）
- `libarkfile_static` — Panda 文件格式（libpandafile）

实际 **VM 核心在 ace_napi 中**：
```
ace_napi (ohos_static_library)
├── libark_jsruntime_static    ← ETs Runtime VM (arkcompiler/ets_runtime)
├── libuv_static               ← 事件循环
├── static_icui18n             ← ICU 国际化
└── static_icuuc               ← ICU Unicode
```

**这意味着 EcmaVM、SourceTextModule、JSPandaFile 等核心 VM 组件都在 ace_napi → libark_jsruntime_static 中，并被链接进了 .so。**

---

## 四级：AppFramework NAPIs

**文件**：`/src/arkui-x/foundation/appframework/BUILD.gn`

`appframework_napis` 是一个 group，包含：

### NAPI 包（napi/BUILD.gn）

| NAPI 模块 | 说明 |
|-----------|------|
| `app_ability_uiability` | UIAbility NAPI 绑定 |
| `app_ability_abilityconstant` | Ability 常量定义 |
| `application_abilitycontext` | AbilityContext NAPI |
| `ability_delegator` | 测试委托器 |
| `app_ability_abilitystage` | AbilityStage NAPI |
| `application_abilitystagecontext` | AbilityStageContext |
| `error_manager` | 错误管理 |
| `application_applicationcontext` | ApplicationContext |
| `applicationstatechangecallback` | 应用状态回调 |
| `bundle_bundlemanager` | Bundle 管理 |
| `app_ability_configurationconstant` | 配置常量 |
| `application_context` | Context 基础 |
| `module_loader` | 模块加载器 |
| `napi_hiviewdfx_hiappevent` | HiAppEvent 事件 |

### Window NAPI
- `windowstage` — 窗口 Stage 管理 NAPI

---

## 五、Ace Kit（UI 框架 API）

**文件**：`interfaces/inner_api/ace_kit/BUILD.gn`

约 30 个源文件，提供 UI 框架的对外 API 抽象：
- 动画系统（animations）
- 属性系统（properties）
- 资源解析（resource_parsing）
- 视图组件（column, stack, tabs, text）
- 修饰器（modifiers）
- 布局算法（layout_algorithms）
- 主题系统（themes）

依赖：
```
ace_kit
├── librender_service_client_static  (Rosen 2D 渲染)
├── libuv_static                     (事件循环)
└── idlize_gen                       (IDL 代码生成)
```

---

## 六、渲染子系统

### Rosen Render Service Client
**路径**：`//foundation/graphic/graphic_2d/rosen/modules/render_service_client`

2D 图形渲染客户端库，ArkUI 在 Android 上的渲染后端。通过 Virtual Rosen Window + Skia 实现。

### Multimodal Input
**路径**：`//foundation/multimodalinput/input/frameworks/proxy`

跨平台输入事件处理（触摸、按键、鼠标）。

### Display Manager
**路径**：`//foundation/appframework/window_manager/dm`

Display 管理（display, display_manager, display_runtime_napi）。

---

## 七、未链接进 .so 的组件（独立 .so）

以下组件**不打包**进 libarkui_android.so，而是作为独立共享库存在：

| 类别 | 组件 |
|------|------|
| **NAPI 模块** | componentutils, component_snapshot, drag_controller, inspector, observer, font, mediaquery, router, animator |
| **通用插件** | ability_access_ctrl, accessibility, file/fs, common_event_manager, data/preferences, web/webview, multimedia/media |
| **UI 组件** | marquee, stepper, slider, gauge, checkbox, rating, waterflow |
| **平台视图** | platformview 组件 |

---

## 八、Android 编译配置（config.gni）

**文件**：`adapter/android/build/config.gni`

关键编译标志：
```gn
defines = [
  "ANDROID_PLATFORM",
  "NG_BUILD",
  "SK_BUILD_FOR_ANDROID",
  "CROSS_PLATFORM",
]

js_engines = []   # 使用 USE_ARK_ENGINE（非 V8）
enable_rosen_backend = true       # Rosen 2D 后端
use_external_icu = "static"       # ICU 静态链接
web_components_support = true     # Web 组件
video_components_support = true   # 视频组件
xcomponent_components_support = true  # XComponent
pixel_map_support = true          # PixelMap
enable_drag_framework = true      # 拖拽框架
build_container_scope_lib = true  # 容器作用域库
```

---

## 九、完整依赖树（简化视图）

```
libarkui_android.so (~95MB)
│
├── libace_static_android ───────────── 主体（所有 ace_engine 源码 + adapter）
│   ├── ace_base_android              基础工具层
│   ├── framework_bridge_ng_android    前端桥接（ArkTS ↔ C++）
│   ├── ace_core_ng_android           核心渲染引擎（NG 组件、布局、绘制、动画）
│   ├── declarative_js_engine_ng_ark  内建 Ark JS 引擎
│   ├── ace_common_jni_android (37+)  JNI 桥接 + capability 模块
│   ├── ace_osal_android (80+)        OS 抽象层（无障碍/输入/图形/资源/窗口/文件）
│   ├── stage_android_jni_android (9)  Stage 模型 JNI
│   └── ace_uicontent_android (4)     UI 内容管理
│
├── ace_kit ─────────────────────────── UI 框架 API
│   ├── librender_service_client_static  Rosen 2D 渲染客户端
│   ├── libuv_static                    事件循环
│   └── idlize_gen                      IDL 生成
│
├── ace_static_ndk ──────────────────── NDK 接口（XComponent, native node）
│
├── appframework_napis ──────────────── App 框架
│   ├── NAPI 包 (14 个)                UIAbility, Context, BundleManager...
│   ├── windowstage                    窗口管理 NAPI
│   ├── dm                             Display Manager
│   └── libmmi-client-crossplatform    多模输入
│
├── ace_plugin_util_inner_android ───── 插件工具
│   └── ace_napi ────────────────────── NAPI 引擎
│       ├── libark_jsruntime_static    ★ ArkCompiler eTS Runtime (EcmaVM)
│       │   ├── ecmascript/            JS/ArkTS 运行时
│       │   ├── jspandafile/           .abc 文件管理
│       │   └── napi/                  NAPI 桥接
│       ├── libuv_static               事件循环
│       ├── static_icui18n             ICU 国际化
│       └── static_icuuc              ICU Unicode
│
├── libarkbase_static ───────────────── Panda VM 基础（条件编译）
└── libarkfile_static ───────────────── Panda 文件格式（条件编译）
```

---

## 十、对 HAP-on-Android 项目的关键启示

1. **ArkCompiler 已在 .so 内** — EcmaVM、JSPandaFile、SourceTextModule、NAPI 引擎全部在 `ace_napi → libark_jsruntime_static` 链中，被静态链接进 libarkui_android.so
2. **ExecuteModuleBuffer 的代码就在 .so 里** — 只是符号为 hidden，外部 dlsym 看不到，但如果自己构建就是可见的
3. **AppFramework NAPIs 已包含** — UIAbility、AbilityContext、ConfigurationConstant 等 NAPI 绑定在 `appframework_napis` 中，这些正是 OHOS HAP 依赖的 @ohos: 模块的 C++ 实现
4. **OS 适配层有 80+ 文件** — ace_osal_android 是最重的适配模块，涵盖无障碍、输入、图形、资源、窗口等所有 OS 级接口
5. **Rosen + Skia 渲染管线已完备** — `librender_service_client_static` 提供完整的 Android 渲染后端
6. **独立 .so 的 NAPI 模块不会阻拦** — 组件级 NAPI（router, animator 等）是独立 .so，不在主 .so 内，但可在运行时 dlopen 加载
