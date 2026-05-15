# EcmaVM 创建与配置

## 概述

EcmaVM 是 ETS 运行时的核心——Panda VM 的 EcmaScript 接口层。OHOS 和 ArkUI-X 共享相同的 VM 创建流程，但在平台配置上有差异。

## VM 创建流程

### 入口: JsEnvironment::Initialize

**文件**: `js_environment.cpp` (行 59-70)

```cpp
bool JsEnvironment::Initialize(const panda::RuntimeOption& pandaOption, void* jsEngine) {
    vm_ = panda::JSNApi::CreateJSVM(pandaOption);  // 创建 EcmaVM 实例
    engine_ = new ArkNativeEngine(vm_, jsEngine);    // 包装为 ArkNativeEngine
}
```

### OHOS 路径: JsRuntime::CreateJsEnv

**文件**: `js_runtime.cpp` (行 818-863)

```cpp
// 行 819: 配置 panda::RuntimeOption
panda::RuntimeOption pandaOption;
// GC 配置: GEN_GC, heap size, GC threads
// JIT/AOT 配置
// 日志配置

// 行 863-864:
jsEnv_ = std::make_shared<JsEnvironment>(std::make_unique<OHOSJsEnvironmentImpl>());
jsEnv_->Initialize(pandaOption, this);
```

### ArkUI-X Android 路径: ArkJsRuntime::Initialize

**文件**: `/src/arkui-x/.../jsruntime/src/runtime.cpp` (行 207-279)

```cpp
// 行 207-229: 配置 panda::RuntimeOption
pandaOption.SetGcType("gen-gc");
pandaOption.SetGcPoolSize(256 * 1024 * 1024); // 256MB
pandaOption.SetGcThreadNum(7);
pandaOption.SetEnableAot(true);

// Android 特定 (行 215-226):
pandaOption.SetStubFile(stubFilePath);     // AOT stub.an
pandaOption.SetEnableAsmInterpreter(true);  // ASM 解释器

// 行 230: 创建 VM
auto vm = panda::JSNApi::CreateJSVM(pandaOption);

// 行 234: 创建 ArkNativeEngine
engine_ = new ArkNativeEngine(vm, this);
```

## VM 创建后的初始化序列

### OHOS

**文件**: `js_runtime.cpp:689` — `JsRuntime::Initialize`

1. `CreateJsEnv(options)` — 创建 VM + Engine
2. `InitSyscapModule(env, globalObj)` — 系统能力模块
3. 保存 `global.requireNapi` 引用 — NAPI 桥接函数
4. `PreloadAce(options)` — 加载 ArkUI ace 模块 (dlopen + `OHOS_ACE_PreloadAceModule`)
5. `SetSearchHapPathTracker(vm, ...)` — HSP 路径查找回调
6. `ReInitJsEnvImpl(options)` — 重新初始化 JS 环境
7. `LoadAotFile(options)` — 加载 AOT 预编译文件
8. **`panda::JSNApi::SetBundle(vm, options.isBundle)`** — 行 761
9. **`panda::JSNApi::SetBundleName(vm, options.bundleName)`** — 行 762
10. **`panda::JSNApi::SetHostResolveBufferTracker(vm, JsModuleReader(...))`** — 行 763
11. `SetHmsModuleList` — 系统 kit 模块映射注册
12. `SetpkgContextInfoList` / `SetPkgAliasList` / `SetPkgNameList` — HSP 包信息
13. `InitConsoleModule()` — console.log 支持
14. `InitSourceMap(operatorObj)` — 堆栈跟踪 source map
15. `InitTimerModule()` — setTimeout/setInterval
16. `InitWorkerModule(options)` — Web Worker 支持
17. `SetModuleLoadChecker()`
18. `SetRequestAotCallback()`
19. **`InitLoop(isStageModel)`** — 行 808, JS 事件循环

### ArkUI-X Android

**文件**: `runtime.cpp`

与 OHOS 基本一致，额外:
1. Android 特定: 设置 `stubFilePath` 和 ASM 解释器 (AOT stub)
2. 注册 `appLibPath` + `appDataLibPath` 给 `NativeModuleManager`
3. 如果 `options.loadAce`: 调用 `DeclarativeModulePreloader::Preload()` 加载 etsstdlib.abc

## VM 关键配置项

| 配置 | OHOS | ArkUI-X Android | 说明 |
|------|------|-----------------|------|
| GC 类型 | GEN_GC | GEN_GC | 分代 GC |
| GC Pool | 256MB | 256MB | 堆大小 |
| GC 线程数 | 7 | 7 | |
| AOT | 启用 | 启用 | Ahead-of-Time 编译 |
| ASM 解释器 | 平台决定 | 启用 | 汇编级解释器 |
| Stub file | 系统路径 | filesDir/stub.an | AOT stub 文件 |
| IsBundle | 根据 compileMode | false (ES Module) | Bundle 模式标志 |

## Bundle vs ES Module 模式

`JSNApi::SetBundle(vm, isBundle)`:
- `true` → Bundle 模式 (旧 CommonJS 格式)
  - `CheckAndGetRecordInfo` 返回第一个 record (不精确匹配)
  - 入口: `_GLOBAL::func_main_0`
- `false` → ES Module 模式 (合并 ABC)
  - `CheckAndGetRecordInfo` 精确匹配 recordName
  - 入口: 模块 record 名

**两者都是 `false`**: OHOS HAP (esmodule compileMode) 和 ArkUI-X (ETS ES 模块) 都是 `isBundle_ = false`。

## 预加载机制

### OHOS PreloadAce

**文件**: `js_runtime.cpp:872`

```cpp
JsRuntime::PreloadAce(options) {
    DeclarativeModulePreloader::Preload(nativeEngine);
    // → dlopen("libace*.so")
    // → 调用 OHOS_ACE_PreloadAceModule()
    // → 注册所有 ArkUI 组件 NAPI 模块
}
```

**文件**: `declarative_module_preloader.cpp:42`
(`/src/ohos/foundation/arkui/ace_engine/interfaces/inner_api/ace/`)

### ArkUI-X PreloadAce

**文件**: `runtime.cpp:436-441`

```cpp
if (options.loadAce) {
    DeclarativeModulePreloader::Preload(*engine);
    // 加载 etsstdlib.abc (ETS 标准库)
}
```
