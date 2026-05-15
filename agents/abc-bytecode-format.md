# ABC 字节码格式与 VM 执行原理

**参考**: `/src/HOA/ark_bytecode_analysis.md`

## 1. 文件格式

### 基本特征

- **Magic header**: `PANDA\0\0\0` (8 bytes)
- **字节序**: Little-endian
- **对齐**: 4-byte alignment
- **版本控制**: 带向后兼容的版本系统
- **索引区域**: classes, methods, fields, prototypes
- **外部实体**: 支持引用其它 .abc 文件中的类型

### ISA 特征

- **基于寄存器** (register-based)，带专用累加器:
  - 64-bit 宽寄存器 (基本类型 + 引用)
  - 128-bit tagged 寄存器 (GC 推荐)
- **间接线程分派解释器** (indirect threaded dispatch) — 快速指令分派
- **无栈设计** (stackless) — 解释器调用不占用宿主栈帧
- 100+ 字节码指令

## 2. 加载与执行流水线

### Step 1: 文件加载

```cpp
auto jsPandaFile = JSPandaFileManager::GetInstance()
    ->LoadJSPandaFile(thread, filename, entry, needUpdate);
```

1. 打开并 mmap .abc 文件
2. 验证 magic header 和 checksum
3. 解析文件头和区域索引
4. 加载 class/method/field 元数据
5. 解析实体间引用

### Step 2: 字节码验证

- **Checksum 验证**: 文件完整性
- **常量池验证**: 所有常量格式正确
- **寄存器索引验证**: 访问在范围内
- **控制流检查**: 异常处理和返回路径有效
- **SAFE verifier**: 安全分析检查

### Step 3: 解释执行

1. 获取下一字节码指令
2. 解码 opcode + 操作数
3. 使用虚拟寄存器执行
4. 更新程序计数器
5. 循环直到终止或中断

## 3. 类加载和方法解析

- **ClassLinker**: 加载和链接 .abc 文件中的类
- **CHA (Class Hierarchy Analysis)**: 优化方法分派
- **Method resolution**: 使用 v-table 处理虚方法
- **Dynamic linking**: 运行时解析外部类引用

### 方法执行流

1. 创建函数帧（function frame）+ 虚拟寄存器
2. 参数拷贝到被调用方帧
3. 字节码顺序执行
4. 返回值通过累加器传递
5. 函数退出时销毁帧

## 4. NAPI 桥接

NAPI 允许 C/C++ 代码与 JS/ArkTS 交互:

```
NativeModuleManager::LoadNativeModule
  → dlopen("lib{module}*.so")
  → napi_module_register()
  → 注入 JS 全局对象
```

能力:
- 从 native 创建和管理 JS 值
- 从 native 调用 JS 函数
- 向 JS 暴露 C/C++ 函数
- 处理对象引用和 GC
- 支持异步操作和回调

## 5. 跨平台执行的前提条件

.abc 文件**可以**在 Android 上不重编译直接执行，前提是:

1. **架构匹配**: ARM64 .abc 只能在 ARM64 上运行
2. **运行时存在**: Ark runtime (Panda VM) 必须在目标设备上
3. **依赖满足**: 所有引用的 native 方法必须有 Android 实现
4. **安全检查**: Android 安全模型和 SAFE verifier 检查通过
5. **平台 API**: 不能依赖 Android 上不存在的 HarmonyOS 特有 API

## 6. es2panda 编译管线

```
TypeScript/ArkTS 源码
  → es2panda (arkcompiler/ets_frontend)
    → 解析 (词法分析 + AST)
    → 转换 (AST → IR)
    → 优化 (类型检查、内联)
    → 生成 .abc 字节码
```

## 7. 运行时核心架构

```
runtime_core/
├── libpandafile/       # .abc 文件底层操作
├── verifier/           # 字节码验证
├── interpreter/        # VM 解释器
├── class_linker/       # 类加载和解析
└── memory-management/  # GC 和堆管理

ets_runtime/
├── ecmascript/         # JS/ArkTS 运行时
├── jspandafile/        # JS 运行时 .abc 管理
└── napi/               # NAPI 桥接
```

## 8. 对 HAP-on-Android 项目的意义

1. **ABC 格式是标准化的**: OHOS HAP 中的 .abc 遵循 PANDA 格式规范，Android 上的 ArkVM 可以理解
2. **阻塞点不是格式**: .abc 文件本身能被加载（LoadJSPandaFile 成功），问题在于加载后 `CheckAndGetRecordInfo` 的 record 名查找
3. **NAPI 桥接是关键**: 所有 `@ohos:` 外部类依赖通过 NAPI 桥接解析，Android 上需要对应实现
4. **编译前端不需要**: 只需要运行时执行 .abc，不需要 es2panda
