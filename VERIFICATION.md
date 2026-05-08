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
