# HAP 读取与 resource.index 集成

## 关键发现

ArkUI-X **已内置** HAP (ZIP) 解析和 `resources.index` 读取能力，无需从零构建 `HapReader`。

---

## 一、已有的 ZIP/HAP 解析库

### 1.1 底层库

**minizip** — `third_party/zlib/contrib/minizip/unzip.h`

标准的 minizip unzip API:
```cpp
#include <unzip.h>
unzFile unzOpen(const char *path);
int unzLocateFile(unzFile file, const char *szFileName, int iCaseSensitivity);
int unzOpenCurrentFile(unzFile file);
int unzReadCurrentFile(unzFile file, voidp buf, unsigned len);
```

### 1.2 HapParser 封装

**文件**: `base/global/resource_management/frameworks/resmgr/include/hap_parser.h`

```cpp
class HapParser {
    // 从 HAP 读取任意文件
    static RState ReadRawFileFromHap(const std::string &hapPath,
        const std::string &patchPath, const std::string &rawFileName,
        size_t &len, std::unique_ptr<uint8_t[]> &outValue);

    // 从 HAP 读取 resources.index
    static bool GetIndexDataFromHap(const char *path,
        std::unique_ptr<uint8_t[]> &buf, size_t &bufLen);

    // 从独立的 resources.index 文件读取
    static bool GetIndexDataFromIndex(const char *path,
        std::unique_ptr<uint8_t[]> &buf, size_t &bufLen);

    // 自动判断是 HAP 文件还是独立 index 文件
    static bool GetIndexData(const char *path,
        std::unique_ptr<uint8_t[]> &buf, size_t &bufLen);
};
```

**判断逻辑** (`hap_parser.cpp:63-69`):
```cpp
bool HapParser::GetIndexData(const char *path, ...) {
    if (Utils::ContainsTail(path, Utils::tailSet)) {
        return GetIndexDataFromHap(path, buf, bufLen);  // HAP 后缀 → ZIP 模式
    } else {
        return GetIndexDataFromIndex(path, buf, bufLen); // 其他 → 独立文件模式
    }
}
```

### 1.3 已链接进 libarkui_android.so

`resource_management` 模块是 `libarkui_android.so` 的依赖（详见 `libarkui-android-contents.md`），意味着 `HapParser` 的所有能力都已可用。

---

## 二、对 HAP-on-Android 的影响

### 2.1 简化了 HAP 读取

原计划的 Patch 4 (HapReader) 不需要从零构建。只需调用已有的 `HapParser::ReadRawFileFromHap` 即可从 HAP 中提取:

- `module.json` — 复制 `StageAssetManager` 的读取路径适配 HAP
- `ets/modules.abc` — 直接按路径名读取
- `resources.index` — 使用现有的 `GetIndexDataFromHap`

### 2.2 集成方式

当前 ArkUI-X 通过 `StageAssetProvider` → `StageAssetManager` 从 Android AssetManager 或文件系统读取资源。HAP-on-Android 需要:

**方式 A**: 在 `StageAssetProvider` 层增加 HAP 路径支持

当文件路径以 `.hap` 结尾时，调用 `HapParser::ReadRawFileFromHap` 代替文件系统读取。

**方式 B**: 先解压 HAP 到临时目录

调用 Java 层 `ZipFile` API 一次性解压 HAP 到 `filesDir`，然后走现有的 `StageAssetManager` 文件路径。

**方式 C**: 在资源管理初始化时传入 HAP 路径

在 `nativeSetHapPath` 或新的 JNI 接口中传递 HAP 路径，native 层判断路径格式自动切换 HAP/文件系统读取。

### 2.3 resources.index 不需要额外处理

`resources.index` 的格式解析已由 `resource_management` 模块完整实现。对于 OHOS HAP，只需确保 `HapParser::GetIndexDataFromHap` 被调用（传入 .hap 路径），其余流程复用现有代码。

---

## 三、Gap 5-6 结论

| Gap | 原风险评估 | 实际状态 |
|-----|:---:|------|
| Gap 5: HAP ZIP 读取库与集成点 | 中低 | **已有方案** — `HapParser::ReadRawFileFromHap` 基于 minizip，已链接进 .so |
| Gap 6: resource.index 格式与加载 | 低 | **已有方案** — 格式解析由 `resource_management` 完整实现 |

**两个 gap 的核心工作已从"构建新能力"缩减为"路由集成"** — 让现有 `HapParser` 在 HAP-on-Android 场景下被正确调用。

---

## 四、源码索引

| 文件 | 关键行 | 内容 |
|------|--------|------|
| `resmgr/include/hap_parser.h` | 34-186 | HapParser 类定义 |
| `resmgr/src/hap_parser.cpp` | 63-69 | GetIndexData 自动判断逻辑 |
| `resmgr/src/hap_parser.cpp` | 135- | GetIndexDataFromHap 实现 |
| `resmgr/src/hap_parser.cpp` | 241- | ReadRawFileFromHap 实现 |
| `third_party/zlib/contrib/minizip/unzip.h` | — | minizip unzip API |
| `libarkui-android-contents.md` | — | resource_management 链接关系 |
