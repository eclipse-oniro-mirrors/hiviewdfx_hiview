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

#ifndef PERF_MONITOR_ADAPTER_H
#define PERF_MONITOR_ADAPTER_H

#include "perf_constants.h"
#include "perf_model.h"

namespace OHOS {
namespace HiviewDFX {

class __attribute__((visibility("default"))) PerfMonitorAdapter {
public:
    static PerfMonitorAdapter& GetInstance();
    PerfMonitorAdapter() = default;
    ~PerfMonitorAdapter() = default;
    void RegisterFrameCallback(IFrameCallback* cb);
    void UnregisterFrameCallback(IFrameCallback* cb);
    void RegisterAnimatorCallback(IAnimatorCallback* cb);
    void UnregisterAnimatorCallback(IAnimatorCallback* cb);
    void RegisterSceneCallback(ISceneCallback* cb);
    void UnregisterSceneCallback(ISceneCallback* cb);

    void RecordInputEvent(PerfActionType type, PerfSourceType sourceType, int64_t time);
    int64_t GetInputTime(const std::string& sceneId, PerfActionType type, const std::string& note);

    void NotifyAppJankStatsBegin();
    void NotifyAppJankStatsEnd();
    void SetPageUrl(const std::string& pageUrl);
    std::string GetPageUrl();
    void SetPageName(const std::string& pageName);
    std::string GetPageName();
    void SetAppForeground(bool isShow);
    void SetAppStartStatus();
    void SetAppInfo(AceAppInfo& appInfo);
    void SetSubHealthInfo(const std::string& info, const std::string& reason, const int32_t duration);
    bool IsScrollJank(const std::string& sceneId);

    void Start(const std::string& sceneId, PerfActionType type, const std::string& note);
    void End(const std::string& sceneId, bool isRsRender);
    void StartCommercial(const std::string& sceneId, PerfActionType type, const std::string& note);
    void EndCommercial(const std::string& sceneId, bool isRsRender);
    void SetFrameTime(int64_t vsyncTime, int64_t duration, double jank, const std::string& windowName);
    void OnFrameEnd(int64_t vsyncTime, int64_t duration, double jank, const std::string& windowName);

    void ReportJankFrameApp(double jank, int32_t jankThreshold);
    void ReportPageShowMsg(const std::string& pageUrl, const std::string& bundleName,
        const std::string& pageName);

    void StartRecordImageLoadStat(int64_t id);
    void EndRecordImageLoadStat(int64_t id, std::pair<int, int> size, const std::string& type, int state);
    void OnSceneChanged(const SceneType& type, bool status);

    void ReportSurface(const int64_t& uniqueId, const std::string& surfaceName, const std::string& componentName,
        const std::string& bundleName, const int32_t& pid);
};
}
}
#endif  // PERF_MONITOR_ADAPTER_H