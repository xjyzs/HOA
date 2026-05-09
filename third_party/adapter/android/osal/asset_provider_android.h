/*
 * Copyright (c) 2024 HongEngine Project.
 */

#ifndef HONGENGINE_ADAPTER_ANDROID_ASSET_PROVIDER_ANDROID_H
#define HONGENGINE_ADAPTER_ANDROID_ASSET_PROVIDER_ANDROID_H

#include <string>
#include <vector>

#include "base/resource/asset_manager.h"
#include "base/utils/macros.h"

namespace OHOS::Ace::Platform {

// Simple asset provider for Android — reads files from a base directory.
// In production, this would use AAssetManager from the Android NDK.
class DirAssetProviderAndroid : public AssetProvider {
    DECLARE_ACE_TYPE(DirAssetProviderAndroid, AssetProvider);

public:
    explicit DirAssetProviderAndroid(const std::string& basePath);
    ~DirAssetProviderAndroid() override = default;

    bool IsValid() const override { return true; }
    std::string GetAssetPath(const std::string& assetName, bool isAddHapPath) override;
    void GetAssetList(const std::string& path, std::vector<std::string>& assetList) override;
    RefPtr<Asset> GetAsset(const std::string& assetName) const;

private:
    std::string basePath_;
};

} // namespace OHOS::Ace::Platform

#endif // HONGENGINE_ADAPTER_ANDROID_ASSET_PROVIDER_ANDROID_H
