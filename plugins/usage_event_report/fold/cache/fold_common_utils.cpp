/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "fold_common_utils.h"

#include "hiview_logger.h"
#include "window_manager.h"

namespace OHOS {
namespace HiviewDFX {
namespace FoldCommonUtils {
DEFINE_LOG_TAG("FoldCommonUtils");

std::pair<std::string, int> GetFocusedAppAndType()
{
    std::vector<sptr<Rosen::WindowVisibilityInfo>> winInfos;
    Rosen::WMError ret = Rosen::WindowManager::GetInstance().GetVisibilityWindowInfo(winInfos);
    if (ret != Rosen::WMError::WM_OK) {
        HIVIEW_LOGI("get windowInfos failed, ret=%{public}d", ret);
        return std::pair<std::string, int>("", SYSTEM_WINDOW_BASE);
    }
    for (auto winInfo : winInfos) {
        if (winInfo == nullptr) {
            continue;
        }
        if (winInfo->IsFocused()) {
            return std::make_pair(winInfo->GetBundleName(), static_cast<int>(winInfo->GetWindowType()));
        }
    }
    return std::pair<std::string, int>("", SYSTEM_WINDOW_BASE);
}
} // namespace FoldCommonUtils
} // namespace HiviewDFX
} // namespace OHOS