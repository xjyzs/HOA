# OHOS HAP 编译流水线与 ABC 字节码结构

## 源代码到字节码

### 开发态 → 编译态

**参考**: `/src/ohos/docs/zh-cn/application-dev/quick-start/application-package-structure-stage.md`

| 阶段 | 文件 | 说明 |
|------|------|------|
| 开发态 | `Module/src/main/ets/entryability/EntryAbility.ets` | ArkTS 源码 |
| 开发态 | `Module/src/main/module.json5` | 模块配置 |
| 开发态 | `AppScope/app.json5` | 应用级配置 |
| 编译态 | `ets/modules.abc` | 编译后的合并字节码文件 |
| 编译态 | `module.json` | app.json5 字段合入后的最终配置 |
| 编译态 | `resources.index` | 资源索引（AppScope 资源合入模块） |

### module.json5 关键字段

**参考**: `module-configuration-file.md`

```json5
{
  "module": {
    "name": "entry",                                    // 模块名 → record 名的第一段
    "type": "entry",
    "srcEntry": "./ets/entryability/EntryAbility.ets",  // 入口 Ability 代码路径
    "mainElement": "EntryAbility",                      // 入口 Ability 名称
    "abilities": [
      {
        "name": "EntryAbility",
        "srcEntry": "./ets/entryability/EntryAbility.ets"  // Ability 级入口
      }
    ]
  }
}
```

### srcEntry 路径格式

注意 module.json5 中的路径**不含** `src/main/`:
```json5
"srcEntry": "./ets/entryability/EntryAbility.ets"
```

而 **ABC 内部的 record 名** 包含 `src/main/`:
```
entry/src/main/ets/entryability/EntryAbility
```

### Record 名的构造规则

OHOS HAP 的 ABC 中 record 名使用完整的**构建时源文件路径**:
```
{moduleName}/src/main/ets/entryability/EntryAbility
{moduleName}/src/main/ets/pages/Index
```

也就是说:
1. `moduleName`（来自 module.json5 的 `name` 字段）作为第一段
2. `src/main/ets/` 是构建系统保留的源路径前缀
3. 后缀去掉 `.ets` 扩展名

## ArkCompiler 架构

**参考**: `/src/ohos/docs/zh-cn/readme/ARK-Runtime-Subsystem-zh.md`

### 四个子系统

```
arkcompiler/
├── ets_runtime       # ArkTS 运行时组件
├── runtime_core      # 运行时公共组件
├── ets_frontend      # ArkTS 语言的前端工具（源码→abc）
└── toolchain         # ArkTS 工具链
```

### 运行时架构

| 子系统 | 职责 |
|--------|------|
| **Core Subsystem** | 语言无关的基础运行库：File 组件（承载字节码）、Tooling 组件（Debugger 支持）、Base 库组件（系统调用适配） |
| **Execution Subsystem** | 字节码解释器、快速路径 Inline Cache、Profiler |
| **Compiler Subsystem** | Stub 编译器、基于 IR 的编译优化框架、代码生成器 |
| **Runtime Subsystem** | 内存管理（CMS-GC/Partial-Compressing-GC）、DFX 工具、Actor 并发模型、ECMAScript 标准库、JSNAPI 接口 |

### ArkCompiler eTS Runtime 设计特点

1. **原生类型支持**: TS 源码编译时分析推导类型信息，传递给运行时。运行时直接使用类型信息预生成 Inline Cache 加速字节码执行。TSAOT Compiler 可利用类型信息直接编译生成优化机器码。

2. **并发模型**: Actor 模型 — 执行体之间不共享数据对象，通过消息机制通信。支持 Worker API 和 TaskPool（优先级调度、工作线程自动扩缩容）。

3. **安全**: 预先静态编译为方舟字节码（不支持 eval，仅 strict 模式），提供多重混淆能力。

## 发布态结构

### APP 打包

```
应用发布: Bundle → App Pack (.app)
  ├── entry.hap          # 必须包含
  ├── feature1.hap       # 可选
  ├── feature2.hsp       # 可选（动态共享包）
  └── pack.info          # 描述每个 HAP/HSP 的属性
```

### HAP 内部结构（运行时可见）

```
entry.hap (ZIP 格式)
├── ets/
│   └── modules.abc       # 合并的 ETS 字节码
├── module.json           # 合并后的模块配置
├── resources.index       # 资源索引
└── resources/
    ├── base/
    │   ├── element/
    │   ├── media/
    │   └── ...
    └── ...
```

## 对 Android 移植的关键影响

1. **Record 名包含 `src/main/`**: 这是构建系统（hvigor/es2abc）决定的，不是运行时行为。Android 上加载 OHOS HAP 时必须处理这种 record 命名格式。

2. **ArkCompiler 核心与平台无关**: Core、Execution、Compiler、Runtime 四个子系统都是平台无关的 C++ 代码，通过 ArkUI-X 已经证明可以在 Android 上编译运行。

3. **ets_frontend 不需要移植**: 只需要运行时（ets_runtime + runtime_core），不需要编译前端。

4. **模块级 srcEntry vs Ability 级 srcEntry**: module.json 中可能有多个入口点（Module 级 `srcEntry` 指向 AbilityStage，Ability 级 `srcEntry` 指向 UIAbility）。加载时需要根据目标 ability 选择正确的入口。
