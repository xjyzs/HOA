# Building HOA

## Quick start

```bash
# 1. Build ArkUI-X for Android (~30 min, 700MB+ output)
cd /data/share/hoa2/arkui-x
./build.sh --product-name arkui-x --target-os android

# 2. Copy all outputs into HOA project
cd /path/to/HOA
./scripts/sync_arkui_x.sh

# 3. Build APK
./gradlew assembleDebug
```

Output: `app/build/outputs/apk/debug/app-debug.apk` (~85 MB, 5 native libraries).

## Prerequisites

| Tool | Version | Check |
|------|---------|-------|
| Gradle | 8.x (wrapper) | `./gradlew --version` |

The ArkUI-X build (step 1) requires its own prerequisites (GN, Ninja, Clang, etc.) — refer to `/data/share/hoa2/arkui-x/` documentation for details. The `sync_arkui_x.sh` script handles bringing all compiled artifacts into the HOA project.

## Build output (native libraries)

All native libraries come from the ArkUI-X build. No third-party source compilation is needed.

| .so | Size | Source |
|-----|------|--------|
| libarkui_android.so | 82 MB | ArkUI-X build (`arkui/arkui-x/`) |
| libarkui_componentsnapshot.so | 454 KB | ArkUI-X build (`arkui/ace_engine_cross/`) |
| libarkui_focuscontroller.so | 377 KB | ArkUI-X build (`arkui/ace_engine_cross/`) |
| libhilog.so | 156 KB | ArkUI-X build (`plugins/hilog/`) |
| libc++_shared.so | 1.7 MB | NDK (copied by ArkUI-X build) |

Previously, 12 additional .so files were compiled from `third_party/arkcompiler_runtime_core` (libarkruntime.so, libarkcompiler.so, etc.). These were confirmed redundant on 2026-05-15 — `libarkui_android.so` already embeds the ETS VM, compiler, and all other runtime components.

## ArkUI-X output files

The `./build.sh --product-name arkui-x --target-os android` build produces these files:

| Output | Path under `out/arkui-x/aosp_clang_arm64_release/` | Destination in HOA |
|--------|------|-----------|
| `arkui_android_adapter.jar` | `.` | `app/libs/` |
| `libarkui_android.so` | `arkui/arkui-x/` | `app/src/main/jniLibs/arm64-v8a/` |
| `libarkui_componentsnapshot.so` | `arkui/ace_engine_cross/` | `app/src/main/jniLibs/arm64-v8a/` |
| `libarkui_focuscontroller.so` | `arkui/ace_engine_cross/` | `app/src/main/jniLibs/arm64-v8a/` |
| `libhilog.so` | `plugins/hilog/` | `app/src/main/jniLibs/arm64-v8a/` |
| `libc++_shared.so` | `third_party/llvm-project/` | `app/src/main/jniLibs/arm64-v8a/` |
| `resources.index` (1.3MB) | `obj/interface/sdk/systemres/` | `app/src/main/assets/arkui-x/systemres/` |
| `resources/` (base/dark/wearable) | `obj/interface/sdk/systemres/resources/` | `app/src/main/assets/arkui-x/systemres/resources/` |
| Fonts (`HMSymbolVF.ttf` etc.) | `obj/interface/sdk/systemres_fonts/` | `app/src/main/assets/arkui-x/systemres/fonts/` |
| ICU data `icudt74l.dat` (30MB) | `icu_data/out/` | `app/src/main/assets/arkui-x/systemres/icudt72l.dat` |
| Framework .abc files (~50+) | `arkui/ace_engine_cross/*.abc` | `app/src/main/assets/arkui-x/systemres/abc/` |

Note: the ICU file is renamed from `icudt74l.dat` to `icudt72l.dat` to match
the filename the Android adapter expects at runtime. If the build produces a
different ICU version, adjust the target filename accordingly.

## `scripts/sync_arkui_x.sh` — 自动化产物同步

重建 ArkUI-X 后，运行此脚本将**所有**编译产物复制到 HOA 项目。所有从 ArkUI-X 外部复制到项目中的文件均由该脚本统一管理。

**用法**：

```bash
# 完整同步（所有文件）
./scripts/sync_arkui_x.sh

# 仅同步 .so（修改 C++ 后快速更新）
./scripts/sync_arkui_x.sh --so-only

# 仅同步 systemres .abc
./scripts/sync_arkui_x.sh --abc-only

# 仅同步资源文件 (JAR/resources/fonts/ICU/stub.an)
./scripts/sync_arkui_x.sh --res-only

# 预览（不实际复制）
./scripts/sync_arkui_x.sh --dry-run

# 指定构建产物目录
ARKUI_BUILD=/custom/path/out/.../aosp_clang_arm64_release ./scripts/sync_arkui_x.sh
```

**完整同步清单**（约 90 个文件）：

| Section | 数量 | 源路径 | 目标路径 | 说明 |
|---------|------|------|------|------|
| A1 | 1 | `arkui/arkui-x/libarkui_android.so` | `jniLibs/arm64-v8a/` | ArkUI-X Android 主库（含 OHOS HAP Patch） |
| A2 | 39 | `arkui/ace_engine_cross/*.so` | `jniLibs/arm64-v8a/` | ACE 渲染引擎组件库 |
| A3 | 1 | `arkui/arkui_components/libplatformview.so` | `jniLibs/arm64-v8a/` | Android 平台视图桥接 |
| A4 | 1 | `plugins/hilog/libhilog.so` | `jniLibs/arm64-v8a/` | OHOS 日志系统 NAPI 模块 |
| B1 | 43 | `arkui/ace_engine_cross/*.abc` | `assets/arkui-x/systemres/abc/` | 框架 UI 系统组件 |
| C1 | 1 | `arkui_android_adapter.jar` | `app/libs/` | ArkUI-X Android Java 适配器 |
| C2 | 1 | `obj/.../systemres/resources.index` | `assets/arkui-x/systemres/` | 系统资源索引 |
| C3 | ~ | `obj/.../systemres/resources/` | `assets/arkui-x/systemres/resources/` | 系统资源目录 |
| C4 | ~ | `obj/.../systemres_fonts/` | `assets/arkui-x/systemres/fonts/` | 字体文件 |
| C5 | 1 | `icu_data/out/icudt74l.dat` | `assets/arkui-x/systemres/icudt72l.dat` | ICU 国际化数据 |
| C6 | 1 | `gen/arkcompiler/ets_runtime/stub.an` | `assets/arkui-x/stub/arm64-v8a/` | ArkTS 运行时桩文件 |

**常见工作流**：

```bash
# 修改了 ArkUI-X 的 C++ 代码后：
cd /data/share/hoa2/arkui-x
./build.sh --product-name arkui-x --target-os android
cd /path/to/HOA
./scripts/sync_arkui_x.sh --so-only    # 仅更新变化的 .so
./gradlew assembleDebug                # 重新打包 APK
adb install -r app/build/outputs/apk/debug/app-debug.apk

# 完整重建（全部产物更新）：
./scripts/sync_arkui_x.sh              # 所有文件
./gradlew assembleDebug
```

## ArkUI-X 源码修改

为了运行 OHOS 原生格式的 HAP，需要对 ArkUI-X 源码进行定向修改。涉及 4 个 git 仓库，共约 12 个文件。修改内容详见 `docs/ARKUI-X_PATCHES.md`。

修改后的源码通过 `repo manifest -r -o manifest_ohos-hap-mode.xml` 生成快照，可复现完整的构建环境。

## Project structure

```
HOA/
├── scripts/
│   └── sync_arkui_x.sh           # ArkUI-X 产物同步脚本
├── app/
│   ├── src/main/jniLibs/         # Native libraries (gitignored, from ArkUI-X build)
│   ├── src/main/assets/arkui-x/  # System resources
│   ├── src/main/java/            # Android Kotlin/Java source
│   └── build.gradle.kts
├── docs/
│   ├── BUILD.md                  # 构建文档
│   └── ARKUI-X_PATCHES.md        # ArkUI-X 源码修改说明
└── agents/                       # 技术调研文档
```

## Troubleshooting

**APK too large** — The 82 MB `libarkui_android.so` dominates. If you only need
ArkCompiler runtime (no UI), you can exclude ArkUI-X .so files.

**Build fails after ArkUI-X update** — Run `./scripts/sync_arkui_x.sh` to ensure
all .so files and assets are in sync with the latest ArkUI-X build output.
