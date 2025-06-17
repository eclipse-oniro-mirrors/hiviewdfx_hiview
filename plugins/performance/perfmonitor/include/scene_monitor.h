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

#ifndef SCENE_MONITOR_H
#define SCENE_MONITOR_H

#include <map>
#include <mutex>
#include "perf_constants.h"
#include "perf_model.h"

#include "transaction/rs_render_service_client.h"

namespace OHOS {
namespace HiviewDFX {

class SceneMonitor {
public:
    static SceneMonitor& GetInstance();
    void NotifyAppJankStatsBegin();
    void NotifyAppJankStatsEnd();
    void SetPageUrl(const std::string& pageUrl);
    std::string GetPageUrl();
    void SetPageName(const std::string& pageName);
    std::string GetPageName();
    void SetAppForeground(bool isShow);
    void SetAppStartStatus();
    bool IsScrollJank(const std::string& sceneId);
    bool GetIsStats();
    const BaseInfo& GetBaseInfo();

    void SetCurrentSceneId(const std::string& sceneId);
    const std::string& GetCurrentSceneId();
    void SetAppInfo(AceAppInfo& aceAppInfo);

    void RecordBaseInfo(SceneRecord* record);
    void NotifySbdJankStatsBegin(const std::string& sceneId);
    void NotifySdbJankStatsEnd(const std::string& sceneId);
    bool IsSceneIdInSceneWhiteList(const std::string& sceneId);
    void CheckTimeOutOfExceptAnimatorStatus(const std::string& sceneId);
    void SetJankFrameRecord(OHOS::Rosen::AppInfo& appInfo, int64_t startTime, int64_t endTime);

    bool IsExceptResponseTime(int64_t time, const std::string& sceneId);
    int32_t GetFilterType() const;
    bool IsExclusionFrame();
    void SetVsyncLazyMode();
    void CheckInStartAppStatus();
    void CheckExclusionWindow(const std::string& windowName);
    void CheckResponseStatus();

    void SetIsBackgroundApp(bool val);
    bool GetIsBackgroundApp();
    void SetIsStartAppFrame(bool val);
    void SetStartAppTime(int64_t val);
    void SetIsExceptAnimator(bool val);
    void SetIsResponseExclusion(bool val);

    void SetSubHealthInfo(const SubHealthInfo& info);
    void FlushSubHealthInfo();
private:
    void NotifyRsJankStatsBegin();
    void NotifyRsJankStatsEnd(int64_t endTime);

    AceAppInfo appInfo;
    BaseInfo baseInfo;
    std::string currentSceneId {""};
    bool isStats = {false};
    bool isResponseExclusion {false};
    bool isStartAppFrame {false};
    bool isBackgroundApp {false};
    bool isExclusionWindow {false};
    bool isExceptAnimator {false};
    int64_t startAppTime {0};

    SubHealthInfo subHealthInfo;
    bool isSubHealthScene = false;
};

}
}

#endif // SCENE_MONITOR_H