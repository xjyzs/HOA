/*
 * Copyright (c) 2024 HongEngine Project.
 */

#include "adapter/android/osal/asset_provider_android.h"

#include <cstdio>
#include <dirent.h>
#include <cstring>
#include <memory>
#include <sys/stat.h>

#include "base/log/log.h"
#include "base/utils/utils.h"

namespace OHOS::Ace::Platform {

namespace {

class FileAsset : public Asset {
public:
    explicit FileAsset(std::unique_ptr<char[]> data, size_t size)
        : data_(std::move(data)), size_(size) {}

    size_t GetSize() const override { return size_; }
    const uint8_t* GetData() const override
    {
        return reinterpret_cast<const uint8_t*>(data_.get());
    }

private:
    std::unique_ptr<char[]> data_;
    size_t size_;
};

} // anonymous namespace

DirAssetProviderAndroid::DirAssetProviderAndroid(const std::string& basePath)
    : basePath_(basePath + "/")
{
    LOGI("DirAssetProviderAndroid: basePath=%{public}s", basePath_.c_str());
}

std::string DirAssetProviderAndroid::GetAssetPath(const std::string& assetName, bool isAddHapPath)
{
    std::string filePath = basePath_ + assetName;
    char realPath[PATH_MAX] = {0};
    if (realpath(filePath.c_str(), realPath) == nullptr) {
        return {};
    }
    struct stat st;
    if (stat(realPath, &st) == 0 && S_ISREG(st.st_mode)) {
        return basePath_;
    }
    return {};
}

void DirAssetProviderAndroid::GetAssetList(const std::string& path, std::vector<std::string>& assetList)
{
    std::string dirPath = basePath_ + path;
    DIR* dp = opendir(dirPath.c_str());
    if (dp == nullptr) {
        return;
    }
    struct dirent* dptr;
    while ((dptr = readdir(dp)) != nullptr) {
        if (strcmp(dptr->d_name, ".") != 0 && strcmp(dptr->d_name, "..") != 0) {
            assetList.push_back(dptr->d_name);
        }
    }
    closedir(dp);
}

RefPtr<Asset> DirAssetProviderAndroid::GetAsset(const std::string& assetName) const
{
    std::string fileName = basePath_ + assetName;
    char realPath[PATH_MAX] = {0};
    if (realpath(fileName.c_str(), realPath) == nullptr) {
        return nullptr;
    }

    auto fp = std::fopen(realPath, "rb");
    if (!fp) {
        return nullptr;
    }

    if (std::fseek(fp, 0, SEEK_END) != 0) {
        std::fclose(fp);
        return nullptr;
    }

    long size = std::ftell(fp);
    if (size < 0) {
        std::fclose(fp);
        return nullptr;
    }

    auto data = std::make_unique<char[]>(size);
    if (data == nullptr) {
        std::fclose(fp);
        return nullptr;
    }

    if (std::fseek(fp, 0, SEEK_SET) != 0) {
        std::fclose(fp);
        return nullptr;
    }

    auto rsize = std::fread(data.get(), 1, size, fp);
    std::fclose(fp);

    if (rsize <= 0) {
        return nullptr;
    }
    return AceType::MakeRefPtr<FileAsset>(std::move(data), rsize);
}

} // namespace OHOS::Ace::Platform
