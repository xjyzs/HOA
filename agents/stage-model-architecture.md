# Stage 模型架构与运行时映射

## 核心概念

**参考**: `/src/ohos/docs/zh-cn/application-dev/application-models/stage-model-development-overview.md`

### 关键设计: 共享 ArkTS 引擎

Stage 模型中，**多个应用组件共享一个 ArkTS 引擎实例**（与 FA 模型每个组件独立引擎不同）。这带来两个关键影响：

1. **内存效率**: 组件间可共享对象和状态，减少内存占用
2. **运行时设计**: Android 移植时只需创建一个 EcmaVM 实例，所有 Ability 共享

## 组件类型与生命周期

### AbilityStage (Module 级)

- 每个 Entry/Feature 类型的 HAP 有一个 AbilityStage 实例
- 在 HAP 代码首次加载到进程时创建（**在第一个 UIAbility 之前**）
- 生命周期: `onCreate()` → (等待 Ability 加载)
- 在 module.json5 中通过模块级 `srcEntry` 指定: `"srcEntry": "./ets/myabilitystage/MyAbilityStage.ets"`

### UIAbility (Ability 级)

完整生命周期:

```
Create → WindowStageCreate → Foreground → Background → WindowStageDestroy → Destroy
                                     ↑_______________|
                                        (可多次切换)
```

| 回调 | 时机 | 用途 |
|------|------|------|
| `onCreate(want, launchParam)` | Ability 创建完成 | 页面初始化、变量定义、资源加载 |
| `onWindowStageCreate(windowStage)` | WindowStage 创建后，进入前台前 | **设置 UI 加载** (`windowStage.loadContent()`)，订阅 WindowStage 事件 |
| `onWindowStageWillDestroy()` (API 12+) | WindowStage 销毁前（仍可用） | 注销 WindowStage 事件订阅 |
| `onForeground()` | UI 可见之前 | 申请系统资源（如 GPS） |
| `onBackground()` | UI 完全不可见之后 | 释放无用资源、保存状态 |
| `onWindowStageDestroy()` | WindowStage 销毁时 | 释放 UI 资源 |
| `onDestroy()` | Ability 实例销毁时 | 释放系统资源、保存数据 |

### WindowStage 事件类型

| 事件 | 含义 |
|------|------|
| SHOWN | 切到前台 |
| ACTIVE | 获焦状态 |
| INACTIVE | 失焦状态 |
| HIDDEN | 切到后台 |
| RESUMED | 前台可交互状态 |
| PAUSED | 前台不可交互状态 |

### ExtensionAbility (场景化组件)

- 不由应用直接启动，由系统服务管理生命周期
- 同类型的 ExtensionAbility 运行在**独立的专用进程**中
- 常见类型: FormExtensionAbility (卡片)、InputMethodExtensionAbility (输入法)、BackupExtensionAbility (备份)

## UIAbility 启动模式

**配置**: module.json5 中 `abilities[].launchType`

| 模式 | 行为 |
|------|------|
| **singleton** (默认) | 单实例复用，重复启动触发 `onNewWant()` 而非 `onCreate()` |
| **multiton** | 每次 `startAbility()` 创建新实例 |
| **specified** | 开发者通过 `AbilityStage.onAcceptWant()` 决定：返回唯一 key 字符串 → 系统根据 key 创建新实例或复用已有 |

## 进程模型

**参考**: `process-model-stage.md`

| 进程类型 | 内容 |
|---------|------|
| **主进程** | 所有 UIAbility 组件（同一 bundle） |
| **ExtensionAbility 进程** | 同类型 ExtensionAbility（除 ServiceExtensionAbility/DataShareExtensionAbility）各一个独立进程 |
| **渲染进程** | WebView 独立渲染进程 |

## 线程模型

**参考**: `thread-model-stage.md`

| 线程 | 职责 |
|------|------|
| **主线程** | UI 绘制、ArkTS 引擎管理（**所有 UIAbility 共享**）、交互事件分发、生命周期管理、TaskPool/Worker 消息接收 |
| **TaskPool 线程** | 重操作（系统管理生命周期、支持优先级调度和负载均衡） |
| **Worker 线程** | 重操作（开发者管理生命周期、支持线程间通信） |

## Context 体系

```
Context (基类)
├── ApplicationContext    → 应用级（文件路径、生命周期订阅、内存监控、语言/设置）
├── AbilityStageContext   → 模块级（HapModuleInfo, Configuration）
├── UIAbilityContext      → Ability 级（启动/停止/连接 ability）
└── ExtensionContext      → ExtensionAbility 级
```

文件路径在不同 Context 级别有所不同:
- ApplicationContext: 应用级路径
- AbilityContext: 模块级路径，位于 `.../base/haps/<module-name>/`

## 对 Android 移植的关键映射

### 组件映射

| OHOS | Android | 说明 |
|------|---------|------|
| AbilityStage | `StageApplication.onCreate()` 后的首次 native 调用 | 通过 `StageApplicationDelegate.initApplication()` 触发 |
| UIAbility | `StageActivity` (每个 Ability 对应一个 Activity) | 通过 `setInstanceName("bundleName:moduleName:abilityName:")` 关联 |
| WindowStage | `WindowViewAospSurface` (SurfaceView) | 通过 `WindowViewBuilder.makeWindowViewAosp()` 创建 |
| Want | Intent extras | 通过 `WantParams` 工具类传递 |

### Instance Name 约定

```
bundleName:moduleName:abilityName:
```

- `bundleName` 必须与 Android 的 `packageName` 一致
- Activity 命名规则: `moduleName + abilityName + "Activity"`
- 解析在 `AppMain::TransformToWant()` 中完成

### 单引擎共享

Stage 模型的"多组件共享一个 ArkTS 引擎"设计意味着:
- Android 端只需**一个** `EcmaVM` 实例
- 所有 Ability 共用同一个 JsRuntime
- `StageApplicationDelegate.initApplication()` 中初始化一次，`StageActivity.onCreate()` 中触发具体 Ability 的加载
