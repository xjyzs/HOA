/*
 * Copyright (c) 2024 HongEngine Project.
 */

#include "adapter/android/entrance/ui_content_impl_android.h"

#include "base/log/log.h"

namespace OHOS::Ace::Platform {

UIContentErrorCode UIContentImplAndroid::Initialize(
    const std::string& assetPath, const std::string& params,
    const std::vector<std::string>& hapPathList)
{
    LOGI("UIContentImplAndroid::Initialize assetPath=%{public}s", assetPath.c_str());
    initialized_ = true;
    return UIContentErrorCode::NO_ERRORS;
}

UIContentErrorCode UIContentImplAndroid::RunPage(
    const std::string& content, const std::string& params,
    const std::string& profile)
{
    LOGI("UIContentImplAndroid::RunPage content=%{public}s", content.c_str());
    // Stub for Step A — Step B will trigger PipelineContext and first frame
    return UIContentErrorCode::NO_ERRORS;
}

void UIContentImplAndroid::Destroy()
{
    LOGI("UIContentImplAndroid::Destroy");
    initialized_ = false;
}

void UIContentImplAndroid::UpdateViewConfig(const ViewportConfig& config)
{
    LOGI("UIContentImplAndroid::UpdateViewConfig: %{public}dx%{public}d, density=%{public}f",
         config.width, config.height, config.density);

    auto container = Container::Current();
    if (!container) {
        return;
    }

    auto aceView = container->GetAceView();
    if (!aceView) {
        return;
    }

    auto* view = static_cast<AceViewAndroid*>(AceType::RawPtr(aceView));
    if (view) {
        view->NotifySurfaceChanged(config.width, config.height);
    }
}

void UIContentImplAndroid::UpdateConfiguration(
    const std::shared_ptr<AbilityBase::Configuration>& configuration,
    const ConfigurationChange& configurationChange)
{
    // Stub — configuration update not needed for first frame
}

} // namespace OHOS::Ace::Platform
