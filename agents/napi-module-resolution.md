# NAPI 模块解析机制

## 模块前缀约定

**文件**: `module_path_helper.h:73-83` (ArkCompiler / ets_runtime / ecmascript / module)

| 前缀 | 用途 | 解析方式 |
|------|------|---------|
| `@bundle:` | 应用内/跨应用 OHOS 模块 | `ParsePrefixBundle` |
| `@package:` | Ohpm 第三方包 | 返回包路径 |
| `@ohos:` | OHOS NAPI 系统模块 | `CheckNativeModule` → OHOS_MODULE |
| `@native:` | 系统内建模块 | NATIVE_MODULE |
| `@app:` | 应用级 NAPI 原生模块 (.so) | APP_MODULE |

## ConcatFileNameWithMerge — 模块前缀分派

**文件**: `module_path_helper.cpp:29-34`

```cpp
ConcatFileNameWithMerge:
  @bundle: → ParsePrefixBundle (跨模块引用)
  @package: → 返回包路径
  ets/     → MakeNewRecord (ETS 相对引用)
  ./       → MakeNewRecord
  其他     → ParseThirdPartyPackage
```

## CheckNativeModule — 系统模块识别

**文件**: `js_module_source_text.cpp:296-321`

```cpp
CheckNativeModule(moduleName):
  @ohos:xxx     → OHOS_MODULE (如 @ohos:hilog, @ohos:router)
  @app:xxx      → APP_MODULE (应用原生 .so)
  @native:xxx.xxx → NATIVE_MODULE (系统内建)
```

## NAPI 模块加载流程

### NativeModuleManager::LoadNativeModule

```cpp
// 1. 根据 system_kits_config.json 映射模块名到 .so 名称
// 2. dlopen("lib{module}*.z.so" 或 "lib{module}.so")
// 3. 调用模块的 napi_module_register()
// 4. 注入到 JS 全局对象
```

### system_kits_config.json

**OHOS 设备路径**: `/system/etc/system_kits_config.json`

```json
{
  "system_kits_config": [
    {
      "name": "hilog",
      "so_name": "libhilog.z.so"
    }
  ]
}
```

**ArkUI-X Android**: 模块 `.so` 文件打包在 APK `lib/` 目录中。

## requireNapi 桥接

`GetRequireNativeModuleFunc` 从 JS 全局对象获取 `requireNapi` 函数：
- 该函数解析所有 `@ohos:` 和 `@system:` 导入
- 是 JS 代码调用系统 NAPI 模块的入口桥接
- 在 `JsRuntime::Initialize` 中保存引用 (js_runtime.cpp:733-736)

## Register 回调机制

### HostResolveBufferTracker

`JSNApi::SetHostResolveBufferTracker(vm, JsModuleReader(...))`:
- 注册 HSP 跨模块解析回调
- 当模块引用跨模块的 ABC 时被调用
- JsModuleReader 负责从 HAP 中读取对应的 modules.abc

### SearchHapPathTracker

`SetSearchHapPathTracker(vm, ...)`:
- 注册 HSP 路径查找回调
- 用于定位 HSP 包的路径

## Android 上的 NAPI 模块

### ArkUI-X 打包的 NAPI 模块

APK `lib/` 目录中的独立模块:
- `libhilog.so` (155KB) — 日志模块
- `libarkui_componentsnapshot.so` (540KB)
- `libarkui_focuscontroller.so` (520KB)

### 模块加载路径

Android 上注册两个 lib 路径给 NativeModuleManager:
1. `appLibPath` — 系统 lib 目录 (`nativeLibraryDir`)
2. `appDataLibPath` — 应用数据 lib 目录 (支持动态更新)

## OHOS HAP 的外部类依赖

OHOS HAP 中的外部类引用:
```
L@ohos.app;
L@ohos.curves;
L@ohos.matrix4;
L@system.app;
@ohos:app.ability.UIAbility
@ohos:hilog
@ohos:application.BackupExtensionAbility
```

这些在 OHOS 设备上由系统 ABC 和 NAPI 模块提供，在非 OHOS 平台上需要 stub/mock 实现。
