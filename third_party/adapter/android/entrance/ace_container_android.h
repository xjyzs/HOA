/*
 * Copyright (c) 2024 HongEngine Project.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HONGENGINE_ADAPTER_ANDROID_ACE_CONTAINER_ANDROID_H
#define HONGENGINE_ADAPTER_ANDROID_ACE_CONTAINER_ANDROID_H

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "adapter/android/entrance/ace_view_android.h"
#include "adapter/preview/osal/request_data.h"
#include "base/resource/asset_manager.h"
#include "base/thread/task_executor.h"
#include "base/utils/noncopyable.h"
#include "core/common/ace_view.h"
#include "core/common/container.h"
#include "core/common/js_message_dispatcher.h"
#include "core/common/platform_bridge.h"
#include "core/event/crown_event.h"

namespace OHOS::Ace::Platform {

// Minimal AceContainer for Android — instance id hardcoded to 0.
// Follows preview adapter pattern but with fewer OHOS dependencies.
class AceContainerAndroid : public Container, public JsMessageDispatcher {
    DECLARE_ACE_TYPE(AceContainerAndroid, Container, JsMessageDispatcher);

public:
    static constexpr int32_t ACE_INSTANCE_ID = 0;

    static void CreateContainer(int32_t instanceId, FrontendType type, bool useNewPipeline);
    static void DestroyContainer(int32_t instanceId);
    static RefPtr<AceContainerAndroid> GetContainerInstance(int32_t instanceId);

    AceContainerAndroid(int32_t instanceId, FrontendType type, bool useNewPipeline);
    ~AceContainerAndroid() override = default;

    // Container interface
    void Initialize() override;
    void Destroy() override;
    void DestroyView() override;

    int32_t GetInstanceId() const override { return instanceId_; }
    std::string GetHostClassName() const override { return ""; }
    RefPtr<Frontend> GetFrontend() const override { return frontend_; }
    RefPtr<TaskExecutor> GetTaskExecutor() const override { return taskExecutor_; }
    RefPtr<AssetManager> GetAssetManager() const override { return assetManager_; }
    RefPtr<PlatformResRegister> GetPlatformResRegister() const override { return resRegister_; }
    RefPtr<PipelineBase> GetPipelineContext() const override { return pipelineContext_; }

    int32_t GetViewWidth() const override { return aceView_ ? aceView_->GetWidth() : 0; }
    int32_t GetViewHeight() const override { return aceView_ ? aceView_->GetHeight() : 0; }
    int32_t GetViewPosX() const override { return 0; }
    int32_t GetViewPosY() const override { return 0; }
    uint32_t GetWindowId() const override { return 0; }
    void SetWindowId(uint32_t windowId) override {}
    bool WindowIsShow() const override { return true; }
    RefPtr<AceView> GetAceView() const override { return aceView_; }
    void* GetView() const override { return static_cast<void*>(AceType::RawPtr(aceView_)); }

    ResourceConfiguration GetResourceConfiguration() const override { return resourceInfo_.GetResourceConfiguration(); }
    void SetResourceConfiguration(const ResourceConfiguration& config) { resourceInfo_.SetResourceConfiguration(config); }
    void UpdateResourceConfiguration(const std::string& jsonStr) override;

    void CallCurlFunction(const RequestData requestData, const int32_t callbackId) const override;
    void Dispatch(const std::string& group, std::vector<uint8_t>&& data, int32_t id, bool replyToComponent) const override;
    void DispatchSync(const std::string& group, std::vector<uint8_t>&& data, uint8_t** resData, int64_t& position) const override {}
    void DispatchPluginError(int32_t callbackId, int32_t errorCode, std::string&& errorMessage) const override;

    void SetCardFrontend(WeakPtr<Frontend> frontend, int64_t cardId) override {}
    WeakPtr<Frontend> GetCardFrontend(int64_t cardId) const override { return nullptr; }
    void SetCardPipeline(WeakPtr<PipelineBase> pipeline, int64_t cardId) override {}
    WeakPtr<PipelineBase> GetCardPipeline(int64_t cardId) const override { return nullptr; }

    bool IsUseNewPipeline() const { return useNewPipeline_; }
    FrontendType GetType() const { return type_; }

    void SetView(AceViewAndroid* view, double density, int32_t width, int32_t height);

    const ResourceInfo& GetResourceInfo() const { return resourceInfo_; }

private:
    void InitializeFrontend();

    int32_t instanceId_;
    FrontendType type_;
    bool useNewPipeline_;
    RefPtr<AceViewAndroid> aceView_;
    RefPtr<TaskExecutor> taskExecutor_;
    RefPtr<AssetManager> assetManager_;
    RefPtr<PlatformResRegister> resRegister_;
    RefPtr<PipelineBase> pipelineContext_;
    RefPtr<Frontend> frontend_;
    RefPtr<PlatformBridge> messageBridge_;
    ResourceInfo resourceInfo_;
    std::string bundleName_;
    std::string moduleName_;

    ACE_DISALLOW_COPY_AND_MOVE(AceContainerAndroid);
};

} // namespace OHOS::Ace::Platform

#endif // HONGENGINE_ADAPTER_ANDROID_ACE_CONTAINER_ANDROID_H
