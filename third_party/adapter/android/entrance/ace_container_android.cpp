/*
 * Copyright (c) 2024 HongEngine Project.
 */

#include "adapter/android/entrance/ace_container_android.h"

#include "base/log/log.h"
#include "base/utils/utils.h"
#include "core/common/ace_application_info.h"
#include "core/common/container.h"
#include "core/common/frontend.h"
#include "core/common/platform_bridge.h"
#include "core/common/platform_res_register.h"
#include "core/common/thread_model_impl.h"

namespace OHOS::Ace::Platform {

// Stub PlatformResRegister implementation
class AndroidPlatformResRegister : public PlatformResRegister {
public:
    bool OnMethodCall(const std::string& method, const std::string& param, std::string& result) override { return false; }
    int64_t CreateResource(const std::string& resourceType, const std::string& param) override { return 0; }
    bool ReleaseResource(const std::string& resourceHash) override { return false; }
};

// Stub AssetManager implementation
class AndroidAssetManager : public AssetManager {
public:
    ~AndroidAssetManager() override = default;
    void PushFront(RefPtr<AssetProvider> provider) override {}
    void PushBack(RefPtr<AssetProvider> provider) override {}
    RefPtr<Asset> GetAsset(const std::string& assetName) override { return nullptr; }
    std::vector<RefPtr<Asset>> GetAssetFromI18n(const std::string& assetName) override { return {}; }
    std::string GetAssetPath(const std::string& assetName, bool isAddHapPath) override { return {}; }
    void SetLibPath(const std::string& appLibPathKey, const std::vector<std::string>& packagePath) override {}
    std::vector<std::string> GetLibPath() const override { return {}; }
    std::string GetAppLibPathKey() const override { return {}; }
    void GetAssetList(const std::string& path, std::vector<std::string>& assetList) const override {}
    bool GetFileInfo(const std::string& fileName, MediaFileInfo& fileInfo) const override { return false; }
};

// Static container registry — single instance for now
static RefPtr<AceContainerAndroid> g_containerInstance;

void AceContainerAndroid::CreateContainer(
    int32_t instanceId, FrontendType type, bool useNewPipeline)
{
    auto container = AceType::MakeRefPtr<AceContainerAndroid>(instanceId, type, useNewPipeline);
    Container::UpdateCurrent(instanceId);
    g_containerInstance = container;
}

void AceContainerAndroid::DestroyContainer(int32_t instanceId)
{
    if (g_containerInstance) {
        g_containerInstance->Destroy();
        g_containerInstance = nullptr;
    }
}

RefPtr<AceContainerAndroid> AceContainerAndroid::GetContainerInstance(int32_t instanceId)
{
    return g_containerInstance;
}

AceContainerAndroid::AceContainerAndroid(int32_t instanceId, FrontendType type, bool useNewPipeline)
    : instanceId_(instanceId), type_(type), useNewPipeline_(useNewPipeline)
{
    if (useNewPipeline_) {
        SetUseNewPipeline();
    }
}

void AceContainerAndroid::Initialize()
{
    LOGI("AceContainerAndroid::Initialize instanceId=%{public}d", instanceId_);

    // Create asset manager (stub)
    assetManager_ = Referenced::MakeRefPtr<AndroidAssetManager>();

    // Create resource register (stub)
    resRegister_ = Referenced::MakeRefPtr<AndroidPlatformResRegister>();

    // Initialize frontend
    InitializeFrontend();
}

void AceContainerAndroid::Destroy()
{
    LOGI("AceContainerAndroid::Destroy instanceId=%{public}d", instanceId_);
    if (pipelineContext_) {
        pipelineContext_->Destroy();
        pipelineContext_ = nullptr;
    }
    frontend_ = nullptr;
    assetManager_ = nullptr;
    taskExecutor_ = nullptr;
}

void AceContainerAndroid::DestroyView()
{
    aceView_ = nullptr;
}

void AceContainerAndroid::InitializeFrontend()
{
    // Frontend initialization happens when we have a JS engine available.
    // For Step A (compilation only), this is a stub.
    LOGI("AceContainerAndroid::InitializeFrontend — stub, JS engine not yet wired");
}

void AceContainerAndroid::SetView(AceViewAndroid* view, double density, int32_t width, int32_t height)
{
    aceView_ = AceType::Claim(view);
}

void AceContainerAndroid::UpdateResourceConfiguration(const std::string& jsonStr)
{
    resourceInfo_.SetResourceConfiguration(ResourceConfiguration());
}

void AceContainerAndroid::CallCurlFunction(const RequestData requestData, const int32_t callbackId) const
{
    // Stub — network not needed for first frame
}

void AceContainerAndroid::Dispatch(
    const std::string& group, std::vector<uint8_t>&& data, int32_t id, bool replyToComponent) const
{
    // Stub — message dispatch not needed for first frame
}

void AceContainerAndroid::DispatchPluginError(int32_t callbackId, int32_t errorCode, std::string&& errorMessage) const
{
    // Stub
}

} // namespace OHOS::Ace::Platform
