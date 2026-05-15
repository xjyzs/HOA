# 符号可见性控制方式

## 关键发现

ArkUI-X 使用 **linker version script**（`.map` 文件）控制符号导出，而非 `-fvisibility=hidden`。

## 一、控制机制

### 1.1 Version Script

**文件**: `arkcompiler/ets_runtime/libark_jsruntime.map`

```ld
{
  global:
    extern "C++" {
      panda::JSNApi::*;        // ★ 仅显式列出的类/函数被导出
      panda::EcmaVM::*;
      // ... 约 400 个导出项
    };
    extern "C" {
      ark_parse_js_frame_info;
      // ...
    };
  local:
    *;                          // ★ 其余全部隐藏
};
```

### 1.2 GN 配置

**文件**: `arkcompiler/ets_runtime/js_runtime_config.gni:541-543`

```gn
# Android 目标也使用同样的 version script
if (target_os == "android" && !ark_standalone_build) {
    libs = [ "log" ]
    version_script = "libark_jsruntime.map"
}
```

OHOS 标准系统（非 debug）也使用相同的 `libark_jsruntime.map`（line 521）。

### 1.3 影响

以下函数**不在**导出列表中，因此 `dlsym` 不可见：

| 函数 | 所在文件 |
|------|---------|
| `ExecuteModuleBuffer` | `js_pandafile_executor.cpp` |
| `ParseAbcPathAndOhmUrl` | `module_path_helper.cpp` |
| `GetOutEntryPoint` | `module.cpp` (platform 层) |
| `AdaptOldIsaRecord` | `path_helper.h` |
| `CheckIsRecordWithBundleName` | `js_pandafile.cpp` |

**这解释了 Plan A（通过 dlsym 调用 libarkui_android.so）失败的根因。**

## 二、对本项目的影响

### 2.1 自己构建 .so 的方案

既然我们自己构建 `libohos_android.so`，符号可见性不再是问题：

**方案 A**: 移除或修改 `version_script` 行
```gn
# 在 js_runtime_config.gni 中注释掉或修改
if (target_os == "android" && !ark_standalone_build) {
    libs = [ "log" ]
    # version_script = "libark_jsruntime.map"  # 去掉此行即可导出所有符号
}
```

**方案 B**: 定制 `.map` 文件
创建一个 `libohos_android.map`，包含所有需要的导出（包括 OHOS HAP 兼容函数）。

**方案 C**: 直接静态链接（推荐）
不需要通过 `dlsym` 访问任何内部函数 — 所有 Patch 函数直接编译进 .so 中，由内部调用链正常触发。

### 2.2 实际建议

对于 HAP-on-Android 项目，**方案 C（直接静态链接）是最优解**：
- 不需要 dlsym 调用任何内部函数
- Patch 1-3 修改的代码在同一编译单元内，直接调用
- Java 层通过 JNI 只需暴露 `nativeLaunchApplication` / `nativeLoadModule` / `nativeDispatchOnCreate` 这些已有接口

这些 JNI 接口的符号在 Android adapter 层注册时自动可见（`RegisterNatives` 动态注册，不依赖 ELF 导出）。

## 三、源码索引

| 文件 | 关键行 | 内容 |
|------|--------|------|
| `libark_jsruntime.map` | 1-469 | Version script：全局导出 + local:* |
| `js_runtime_config.gni` | 520-544 | Android/OHOS 共用 version_script 配置 |
| `BUILD.gn` (ets_runtime) | 1689 | `-Wl,-Bsymbolic` 标志 |
