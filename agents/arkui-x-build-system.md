# ArkUI-X 构建系统详解

**参考**: `/src/arkui-x/docs/zh-cn/framework-dev/quick-start/start-with-build.md`、`project-structure-guide.md`

## 构建命令

```shell
# 下载/更新预编译工具链
./build/prebuilts_download.sh --build-arkuix --skip-ssl

# Android 平台编译
./build.sh --product-name arkui-x --target-os android
```

## 仓库组织：直接复用 OpenHarmony 核心代码

**关键发现**: ArkUI-X 不是 fork OpenHarmony，而是**直接指向 OHOS master 分支的固定 tag 点**，定期按周同步。核心运行时代码（arkcompiler、ace_engine）与 OHOS **完全一致**。

| 组件 | 来源仓库 | 关系 |
|------|---------|------|
| `arkcompiler/ets_runtime` | `OpenHarmony/arkcompiler_ets_runtime` | **同一代码**，定期同步 |
| `arkcompiler/runtime_core` | `OpenHarmony/arkcompiler_runtime_core` | **同一代码** |
| `foundation/arkui/ace_engine` | `OpenHarmony/arkui_ace_engine` | **同一代码** |
| `foundation/arkui/napi` | `OpenHarmony/arkui_napi` | **同一代码** |
| `foundation/graphic/graphic_2d` | `OpenHarmony/graphic_graphic_2d` | **同一代码** |
| `arkcompiler/ets_frontend` | `OpenHarmony/arkcompiler_ets_frontend` | **同一代码** |
| `developtools/ace_ets2bundle` | `OpenHarmony/ace_ets2bundle` | **同一代码** |
| `base/global/resource_management` | `OpenHarmony/global_resource_management` | **同一代码** |
| **Android 适配层** | `ArkUI-X/arkui_for_android` | ArkUI-X **独立** |
| **AppFramework 适配层** | `ArkUI-X/app_framework` | ArkUI-X **独立** |
| **构建插件** | `ArkUI-X/build_plugins` | ArkUI-X **独立** |

**这意味着**: ArkUI-X 和 OHOS 的 EcmaVM、ArkCompiler、ACE 引擎核心代码**完全相同**。差异仅在于：
1. Android/iOS 适配层（adapter/android、adapter/ios）
2. AppFramework 跨平台适配层
3. 构建系统插件

## GN 构建架构

### 平台动态发现

```gn
# ace_config.gni: 自动扫描 adapter/ 下的所有目录
ace_platforms = []
_ace_adapter_dir = rebase_path("$ace_root/adapter", root_build_dir)
_adapters = exec_script("build/search.py", [_ace_adapter_dir], "list lines")

# 导入每个 adapter 的 platform.gni
foreach(item, _adapters) {
  import("$ace_root/adapter/$item/build/platform.gni")
  # 收集 platform 定义 → ace_platforms
}

# 为每个 platform 生成编译目标
foreach(item, ace_platforms) {
  ace_base_source_set("ace_base_" + item.name) {
    platform = item.name
    defines = config.defines
    cflags_cc = config.cflags_cc
    # ...
  }
}
```

### 平台配置 (adapter/android/build/config.gni)

```gn
defines = [
  "OHOS_PLATFORM",
  "OHOS_STANDARD_SYSTEM",
]

js_engines = []
ark_engine = {
  engine_name = "ark"
  engine_path = "jsi"
  engine_defines = ["USE_ARK_ENGINE"]
}

disable_gpu = true
use_external_icu = "shared"
use_curl_download = true
ohos_standard_fontmgr = true

platform_deps = [
  "//foundation/arkui/ace_engine/adapter/ohos/entrance:ace_ohos_standard_entrance",
  "//foundation/arkui/ace_engine/adapter/ohos/osal:ace_osal_ohos",
]
```

### ACE 引擎目录结构

```
foundation/arkui/ace_engine
├── ace_config.gni     // 全局配置
├── adapter            // 平台适配层
│   ├── android        // Android (独立仓: ArkUI-X/arkui_for_android)
│   ├── ios            // iOS (独立仓: ArkUI-X/arkui_for_ios)
│   ├── ohos           // OpenHarmony
│   └── preview        // 预览器
├── frameworks         // 引擎框架层 (平台无关)
│   ├── base           // base 库
│   ├── bridge         // 前端桥接
│   └── core           // 引擎核心实现
└── interfaces         // 对外接口
    └── napi
```

### 分层设计原则

- `frameworks/` 目录下所有模块**平台无关**，不能依赖 `adapter/` 下的模块
- 平台相关代码必须放到 `adapter/{platform}/`
- 非 OpenHarmony 的 adapter 不能依赖 OHOS 其它子系统模块
- `frameworks/core` 不允许依赖 `frameworks/bridge`
- 修改 OpenHarmony 仓中代码需保证各平台编译通过

## 对 HAP-on-Android 项目的关键启示

1. **不需要重写 arkcompiler**: OHOS 和 ArkUI-X 用的是同一份 arkcompiler 代码，直接复用
2. **适配层可参考**: ArkUI-X 的 Android 适配层 (`adapter/android/`) 已经解决了 Android 上的窗口、渲染、Asset 加载问题
3. **构建系统可参考**: 可以复用 ArkUI-X 的 GN 模板和 `ace_config.gni` 的平台动态发现机制
4. **补丁范围精确**: 只需要修改 adapter 层中的路径路由逻辑（`platform/common/module.cpp` 的 `GetOutEntryPoint` 和 `module_path_helper.cpp` 的 `ParseAbcPathAndOhmUrl`）
