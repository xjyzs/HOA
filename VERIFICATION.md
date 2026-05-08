# HOA — Device Verification Guide

## Install & Run

```bash
# Install APK
adb install -r app/build/outputs/apk/debug/app-debug.apk

# Launch app
adb shell am start -n app.hackeris.hoa/.MainActivity

# Watch logs (filter by HOA tags)
adb logcat -s HOA.App:V HOA.Main:V HOA.Ability:V
```

## Expected Log Sequence

### Step 1: App Launch — MainActivity

```
I/HOA.Main: ========== HOA MainActivity START ==========
I/HOA.Main: Device: <model>, SDK: <int>
I/HOA.Main: ABI: [arm64-v8a, ...]
I/HOA.Main: nativeLibraryDir: /data/app/.../lib/arm64
I/HOA.Main:   libarkui_android.so: FOUND
I/HOA.Main:   libhilog.so: FOUND
I/HOA.Main:   libarkui_componentsnapshot.so: FOUND
I/HOA.Main:   libarkui_focuscontroller.so: FOUND
I/HOA.Main: Asset check:
I/HOA.Main:   arkui-x/entry/ets/modules.abc: FOUND
I/HOA.Main:   arkui-x/entry/module.json: FOUND
I/HOA.Main:   arkui-x/entry/resources.index: FOUND
I/HOA.Main: ========== HOA MainActivity END ==========
```

**If any "MISSING"**: .so not extracted or assets not packed — check `extractNativeLibs=true` in manifest.

### Step 2: Tap "Launch HAP" — HoaAbilityActivity

```
I/HOA.Main: Launching HoaAbilityActivity
I/HOA.App: ========== HOA Application onCreate START ==========
I/HOA.App: Process: <pid>
I/HOA.App: StageApplication.onCreate() completed successfully
I/HOA.App: ========== HOA Application onCreate END ==========
I/HOA.Ability: ========== HoaAbilityActivity onCreate START ==========
I/HOA.Ability: bundleName=com.example.enjoyarkuix, moduleName=entry, abilityName=DynamicHapAbility
I/HOA.Ability: instanceName=com.example.enjoyarkuix:entry:DynamicHapAbility:
I/HOA.Ability: setInstanceName() OK
I/HOA.Ability: super.onCreate() completed — ArkUI rendering surface should be created
I/HOA.Ability: ========== HoaAbilityActivity onCreate END ==========
```

### Step 3: ArkUI-X Internal Logs (unfiltered)

Also check ArkUI-X internal logs with broader filter:

```bash
adb logcat | grep -iE "StageApp|StageActivity|AppMain|Frontend|Ace|ArkVM|PandaFile|napi|EcmaVM|ace_engine|libarkui"
```

Expected internal sequence:
1. `StageApplicationDelegate` — native lib loaded, assets copied
2. `AppMain::LaunchApplication` — native app framework init
3. `CreateRuntime` / `JsRuntime::Initialize` — Ark VM created
4. `EcmaVM::Create` — VM instance ready
5. `StageActivityDelegate` — Activity attached to native
6. `ArktsFrontend::RunPage` / `LoadModule` — .abc loaded and executed
7. `PipelineContext` — rendering pipeline started
8. `SurfaceCreated` — ANativeWindow bound, first frame rendering

### Step 4: Lifecycle Verification

```
# Press home button
I/HOA.Ability: onPause — UIAbility.onBackground() should fire

# Bring app back
I/HOA.Ability: onResume — UIAbility.onForeground() should fire

# Press back / close
I/HOA.Ability: onDestroy — UIAbility.onDestroy() should fire
```

## Common Failure Modes

| Log Symptom | Cause | Fix |
|-------------|-------|-----|
| `UnsatisfiedLinkError: dlopen failed: library "libarkui_android.so" not found` | .so not extracted from APK | Verify `android:extractNativeLibs="true"` in Manifest |
| `UnsatisfiedLinkError: dlopen failed: wrong ELF class` | 32-bit device, only arm64 .so included | Build with armeabi-v7a or use arm64 device |
| `HOA.App: FATAL: Failed to load native library` | StageApplication can't find/link native lib | Check `nativeLibraryDir` contents, verify .so ABI |
| Black screen, no ArkUI logs | .abc not found or entry class mismatch | Check `instanceName` format; verify assets/arkui-x/ structure |
| `CheckFileVersion: version is NOT supported` | .abc bytecode version incompatible with VM | Check .abc header version with `xxd -l 16 file.abc` |
| `Unable to find __EntryWrapper` or `Cannot find entry class` | Entry class name format mismatch | ArkUI-X uses `arkui.ArkUIEntry.Application` (ANI); OHOS uses `__EntryWrapper_*` (NAPI) |
| `resources.index` related errors | Missing or corrupt resource index | Ensure compiled `resources.index` is in assets |
| Crash with no HOA logs | Native crash before Java code runs | Check `adb logcat | grep -i "FATAL\|SIGSEGV\|SIGABRT\|tombstone"` |

## Stage 1 验证清单

在设备上运行以下命令查看日志：

```bash
adb logcat -s HOA.App:V HOA.Main:V HOA.Ability:V HOA.Verify:V
```

`Stage1Verifier` 会在 `HoaApplication.onCreate()` 中自动运行，在 `HOA.Verify` 标签下输出结构化的 PASS/FAIL 报告。

### 预期验证输出

```
I/HOA.Verify: ===== Stage 1 Verification START =====
I/HOA.Verify: nativeLibraryDir: /data/app/.../lib/arm64
I/HOA.Verify: ===== Stage 1 Verification: 10/10 PASSED =====
```

### 全部检查项（13 项）

| # | 检查项 | 通过条件 | 失败原因 |
|---|--------|----------|----------|
| 1 | `NativeLib/libarkui_android.so` | 文件存在于 nativeLibraryDir | .so 未解压 — 检查 Manifest 中 `extractNativeLibs=true` |
| 2 | `NativeLib/libhilog.so` | 文件存在于 nativeLibraryDir | jniLibs 中缺失 |
| 3 | `NativeLib/libarkui_componentsnapshot.so` | 文件存在于 nativeLibraryDir | jniLibs 中缺失 |
| 4 | `NativeLib/libarkui_focuscontroller.so` | 文件存在于 nativeLibraryDir | jniLibs 中缺失 |
| 5 | `NativeLibSize/libarkui_android.so` | 文件大小 > 50 MB | 文件过小 — 可能损坏或复制了错误文件 |
| 6 | `Asset/arkui-x/entry/ets/modules.abc` | 存在于 APK assets 中 | 未运行 setup-runtime.sh |
| 7 | `Asset/arkui-x/entry/module.json` | 存在于 APK assets 中 | 未运行 setup-runtime.sh |
| 8 | `Asset/arkui-x/entry/resources.index` | 存在于 APK assets 中 | 未运行 setup-runtime.sh |
| 9 | `AbcBytecode` | PANDA 魔数 + 版本号 10–13 | .abc 文件错误或字节码版本不兼容 |
| 10 | `ModuleJson` | 合法 JSON，包含 `module.name` 和 `module.type` | module.json 缺失或损坏 |
| 11 | `ResourcesIndex` | 大小 > 1000 字节 | resources.index 为空或损坏 |
| 12 | `InstalledHaps` | （参考项）列出 filesDir/haps/ 下已安装的 HAP | 尚未安装 HAP — 点击"Install HAP"按钮安装 |
| 13 | `RuntimeClasses` | StageApplication、StageActivity、BridgeManager 可加载 | arkui_android_adapter.jar 缺失或未在 classpath 中 |

### AbcBytecode 检查详情

验证器读取 `modules.abc` 的前 16 字节并检查：
- 字节 0–4：`P` `A` `N` `D` `A`（PANDA 文件魔数）
- 字节 0x0C–0x0F：版本号 `[major, minor, feature, build]`
  - 版本 12.x = OpenHarmony API 12 字节码
  - 版本 13.x = OpenHarmony API 13 / ArkUI-X 运行时
  - v12 字节码与 v13 运行时向后兼容

失败示例：
```
W/HOA.Verify:   FAIL: AbcBytecode — magic=INVALID, version=0.0.0.0 (MAY BE INCOMPATIBLE)
```

### HoaApplication 初始化追踪

除验证器外，`HoaApplication` 还暴露了两个字段供下游检查：

- `initSuccess: Boolean` — `StageApplication.onCreate()` 无异常完成时为 `true`
- `initError: Throwable?` — 初始化失败时的异常对象

`HoaAbilityActivity` 在尝试渲染前会检查这些字段，并在 Application 初始化失败时输出警告日志。

### 增强的 Ability Activity 日志

启动 HAP 时，`HoaAbilityActivity` 现在会输出：

```
I/HOA.Ability: bundleName=..., moduleName=..., abilityName=...
I/HOA.Ability: instanceName=com.example:entry:MainAbility:
I/HOA.Ability: setInstanceName() OK
I/HOA.Ability: super.onCreate() completed — ArkUI rendering surface should be created
I/HOA.Ability: instanceId=<int>, instanceName=com.example:entry:MainAbility:
```

如果 Application 初始化失败：
```
E/HOA.Ability: StageApplication init FAILED — ArkUI rendering will not work
E/HOA.Ability:   Error was: <exception message>
```

## Quick Diagnostic Commands

```bash
# Full log capture for debugging
adb logcat -v time > hoa_logcat.txt

# Check if .so was extracted
adb shell ls -la /data/app/*/app.hackeris.hoa-*/lib/arm64/

# Check app data dir for asset extraction
adb shell ls -laR /data/data/app.hackeris.hoa/files/

# Force stop and restart
adb shell am force-stop app.hackeris.hoa
adb shell am start -n app.hackeris.hoa/.MainActivity
```
