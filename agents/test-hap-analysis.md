# 测试 HAP 样本分析

**样本文件**: `/data/share/entry-default-unsigned.hap` (123KB, HAP v2.0 ZIP 格式)

## 基本信息

| 属性 | 值 |
|------|-----|
| bundleName | `app.hackeris.harmonyexample` |
| Module 名 | `entry`, type: `entry` |
| compileMode | `esmodule` (ES Module 模式) |
| virtualMachine | `ark13.0.1.0` |
| targetAPIVersion | 60002022 (API 22, OHOS 6.0.2) |
| compileSdkVersion | `6.0.2.130` |
| 目标设备 | `phone` |

## HAP 内部文件清单

| 文件 | 大小 | 说明 |
|------|------|------|
| `ets/modules.abc` | 12,860 bytes | 应用字节码 |
| `ets/sourceMaps.map` | 3,373 bytes | Source Map |
| `module.json` | 1,574 bytes | 合并后的模块配置 |
| `resources.index` | 1,030 bytes | 资源索引 |
| `resources/base/media/background.png` | 70KB | 背景图 |
| `resources/base/media/foreground.png` | 13KB | 前景图 |
| `resources/base/media/startIcon.png` | 20KB | 启动图标 |
| `resources/base/profile/backup_config.json` | 29 bytes | 备份配置 |
| `resources/base/profile/main_pages.json` | 23 bytes | 页面路由 `["pages/Index"]` |
| `pack.info` | 612 bytes | 包信息 |
| `pkgContextInfo.json` | 444 bytes | 包上下文信息 |

## Ability 定义

### EntryAbility (主入口)

```json
{
  "name": "EntryAbility",
  "srcEntry": "./ets/entryability/EntryAbility.ets",
  "exported": true,
  "skills": [{
    "entities": ["entity.system.home"],
    "actions": ["ohos.want.action.home"]
  }]
}
```

### EntryBackupAbility (扩展能力)

```json
{
  "name": "EntryBackupAbility",
  "srcEntry": "./ets/entrybackupability/EntryBackupAbility.ets",
  "type": "backup"
}
```

## modules.abc 内部结构

### Magic Header

```
PANDA\0\0\0  (8 bytes, 确认是有效 ABC 文件)
```

### Record 名（3 个模块记录）

```
entry/src/main/ets/entryability/EntryAbility     (入口 Ability)
entry/src/main/ets/entrybackupability/EntryBackupAbility  (备份扩展)
entry/src/main/ets/pages/Index                   (首页)
```

### 外部类依赖清单

#### @ohos: 类型 (OHOS_MODULE — 需要 NAPI .so)

| 依赖 | 说明 | 在 ArkUI-X 中? |
|------|------|:---:|
| `@ohos:app.ability.UIAbility` | UIAbility 基类 | **需要移植** |
| `@ohos:hilog` | 日志模块 | **已有** (libhilog.so) |
| `@ohos:app.ability.ConfigurationConstant` | 配置常量 | **需要移植** |
| `@ohos:application.BackupExtensionAbility` | 备份扩展基类 | **需要 stub** |

#### @native: 类型 (NATIVE_MODULE — 系统内建)

| 依赖 | 说明 |
|------|------|
| `@native.ohos.app` | 应用基础框架 |
| `@native.ohos.curves` | 动画曲线 |
| `@native.ohos.matrix4` | 4×4 矩阵 |

#### 类引用 (L-class 格式)

| 引用 | 类型 |
|------|------|
| `L@ohos.app;` | OHOS 应用类 |
| `L@ohos.curves;` | 曲线类 |
| `L@ohos.matrix4;` | 矩阵类 |

### 依赖分类汇总

```
OHOS_MODULE (需 NAPI .so):   4 个
NATIVE_MODULE (系统内建):     3 个  
L-CLASS (类引用):             3 个
─────────────────────────────
总计外部依赖:                10 个
```

## 额外发现

### 源文件映射信息

ABC 中包含编译时的源文件路径引用:
```
entry|entry|1.0.0|src/main/ets/entryability/EntryAbility.ts
entry|entry|1.0.0|src/main/ets/entrybackupability/EntryBackupAbility.ts
entry|entry|1.0.0|src/main/ets/pages/Index.ts
```

这表明 es2abc 在编译时插入了 `{module}|{module}|{version}|{sourcePath}` 格式的源文件元数据。

### 页面路由

仅有一个页面: `pages/Index`

## 复杂度评估

这个 HAP 是**最小可用样本**的理想选择:
- 仅 3 个模块记录（EntryAbility + EntryBackupAbility + Index 页面）
- 仅 1 个页面
- 外部依赖 10 个，其中 1 个在 ArkUI-X 中已有实现
- modules.abc 仅 12KB，加载开销小
- ES Module 模式（非 Bundle），与 ArkUI-X 相同

## 验证策略

Phase 2 验证时可先忽略 `EntryBackupAbility`（backup extension），只聚焦 `EntryAbility` → `pages/Index` 路径，将依赖从 10 个缩减到约 7 个。
