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

#include "display_info.h"
#include "display_manager_lite.h"
#include "usage_event_common.h"
#include "window_manager_lite.h"

namespace OHOS {
namespace HiviewDFX {
namespace FoldCommonUtils {
namespace {
int32_t GetMultiWindowMode(int32_t mode)
{
    switch (mode) {
        case 100: // 100-SPLIT_PRIMARY
            return MultiWindowMode::WINDOW_MODE_SPLIT_PRIMARY;
        case 101: // 101-SPLIT_SECONDARY
            return MultiWindowMode::WINDOW_MODE_SPLIT_SECONDARY;
        case 102: // 102-FLOATING
            return MultiWindowMode::WINDOW_MODE_FLOATING;
        default:
            return -1;
    }
}
}

int32_t GetFoldStatus()
{
    return static_cast<int32_t>(Rosen::DisplayManagerLite::GetInstance().GetFoldStatus());
}

int32_t GetVhMode()
{
    int32_t vhMode = 0;
    auto display = Rosen::DisplayManagerLite::GetInstance().GetDefaultDisplay();
    if (display != nullptr) {
        auto displayInfo = display->GetDisplayInfo();
        if (displayInfo != nullptr) {
            auto orientation = displayInfo->GetDisplayOrientation();
            if (orientation == Rosen::DisplayOrientation::PORTRAIT ||
                orientation == Rosen::DisplayOrientation::PORTRAIT_INVERTED) {
                vhMode = 0; // 0-Portrait
            } else {
                vhMode = 1; // 1-landscape
            }
        }
    }
    return vhMode;
}

void GetFocusedAppAndWindowInfos(std::pair<std::string, bool>& focusedAppPair,
    std::unordered_map<std::string, int32_t>& multiWindowInfos)
{
    focusedAppPair = std::pair<std::string, bool>("", false);
    multiWindowInfos.clear();
    Rosen::WindowInfoOption windowInfoOption;
    windowInfoOption.windowInfoFilterOption = Rosen::WindowInfoFilterOption::FOREGROUND;
    std::vector<sptr<Rosen::WindowInfo>> winInfos;
    Rosen::WMError ret = Rosen::WindowManagerLite::GetInstance().ListWindowInfo(windowInfoOption, winInfos);
    if (ret != Rosen::WMError::WM_OK) {
        return;
    }
    for (auto winInfo : winInfos) {
        if (winInfo == nullptr) {
            continue;
        }
        int32_t windowType = static_cast<int32_t>(winInfo->windowMetaInfo.windowType);
        int32_t windowMode = GetMultiWindowMode(static_cast<int32_t>(winInfo->windowMetaInfo.windowMode));
        if (winInfo->windowMetaInfo.isFocused) {
            focusedAppPair = std::make_pair(winInfo->windowMetaInfo.bundleName, (windowType < SYSTEM_WINDOW_BASE));
        }
        if (windowType < SYSTEM_WINDOW_BASE && windowMode > 0) {
            multiWindowInfos[winInfo->windowMetaInfo.bundleName] = windowMode;
            if (winInfo->windowMetaInfo.isMidScene) {
                multiWindowInfos[winInfo->windowMetaInfo.bundleName] = MultiWindowMode::WINDOW_MODE_MIDSCENE;
            }
        }
    }
}
} // namespace FoldCommonUtils
} // namespace HiviewDFX
} // namespace OHOS
