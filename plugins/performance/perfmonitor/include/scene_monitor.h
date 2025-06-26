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
#include <vector>
#include "animator_monitor.h"
#include "perf_constants.h"
#include "perf_model.h"
#include "iremote_broker.h"

#include "transaction/rs_render_service_client.h"

namespace OHOS {
namespace HiviewDFX {

class SceneManager {
public:
    void OnSceneStart(const SceneType& type);
    void OnSceneStop(const SceneType& type);
    void OnSceneStart(const SceneType& type, const std::string& sceneId);
    void OnSceneStop(const SceneType& type, const std::string& sceneId);
    SceneRecord* GetRecordByType(const SceneType& type);
    uint64_t GetSceneTag();
    uint64_t GetSceneTagByType(const SceneType& type);
private:
    mutable std::mutex mMutex;
    std::map<SceneType, SceneRecord*> sceneBoard;
};

class SceneMonitor : public IAnimatorCallback , public ISceneCallback {
public:
    static SceneMonitor& GetInstance();
    SceneMonitor();
    ~SceneMonitor();
    void RegisterSceneCallback(ISceneCallback* cb);
    void UnregisterSceneCallback(ISceneCallback* cb);
    void OnSceneEvent(const SceneType& type, bool status) override;
    void OnSceneEvent(const SceneType& type, bool status, const std::string& sceneId) override;
    void OnSceneChanged(const SceneType& type, bool status);
    void OnSceneChanged(const SceneType& type, bool status, const std::string& sceneId);
    void OnAnimatorStart(const std::string& sceneId, PerfActionType type, const std::string& note) override;
    void OnAnimatorStop(const std::string& sceneId, bool isRsRender) override;

    // outer interface for scene-info
    void SetAppInfo(AceAppInfo& aceAppInfo);
    void SetPageUrl(const std::string& pageUrl);
    void SetPageName(const std::string& pageName);
    void SetCurrentSceneId(const std::string& sceneId);

    // outer interface when scene-change
    void SetAppForeground(bool isShow);
    void SetAppStartStatus();

    // inner interface when scene-change 
    void SetAppGCStatus(const std::string& sceneId, int64_t value);

    // inner interface get scene-info
    BaseInfo GetBaseInfo();
    std::string GetPageUrl();
    std::string GetPageName();
    int32_t GetPid();
    std::string GetCurrentSceneId();

    // innner interface when scene-change
    bool IsScrollJank(const std::string& sceneId);
    uint64_t GetNonexpFilterTag();
    
    // stats jank frame for app
    void NotifyAppJankStatsBegin();
    void NotifyAppJankStatsEnd();
    void NotifyAppJankStatsReport(int64_t duration);
    void NotifyScbJankStatsBegin(const std::string& sceneId);
    void NotifyScbJankStatsEnd(const std::string& sceneId);
    void SetJankFrameRecord(OHOS::Rosen::AppInfo& appInfo, int64_t startTime, int64_t endTime);
    bool GetIsStats();
    void SetStats(bool status);
    
    // inner interface for response time out
    bool IsExceptResponseTime(int64_t time, const std::string& sceneId);
    
    // inner interface when non-experience scene
    void SetVsyncLazyMode(uint64_t sceneTag);

    void SetSubHealthInfo(const SubHealthInfo& info);
    void FlushSubHealthInfo();

private:
    void NotifyRsJankStatsBegin();
    void NotifyRsJankStatsEnd(int64_t endTime);
private:
    BaseInfo baseInfo;
    std::string currentSceneId {""};
    bool isStats {false};
    mutable std::mutex mMutex;
    SceneManager mNonexpManager;
    std::vector<ISceneCallback*> sceneCallbacks;

    SubHealthInfo subHealthInfo;
    bool isSubHealthScene = false;
};

}
}

#endif // SCENE_MONITOR_H