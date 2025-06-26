
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

#include "perf_monitor_adapter.h"

#include "animator_monitor.h"
#include "input_monitor.h"
#include "jank_frame_monitor.h"
#include "perf_reporter.h"
#include "perf_utils.h"
#include "scene_monitor.h"
#include "white_block_monitor.h"

namespace OHOS {
namespace HiviewDFX {

PerfMonitorAdapter& PerfMonitorAdapter::GetInstance()
{
    static PerfMonitorAdapter instance;
    return instance;
}

void PerfMonitorAdapter::RegisterFrameCallback(IFrameCallback * cb)
{
    JankFrameMonitor::GetInstance().RegisterFrameCallback(cb);
}

void PerfMonitorAdapter::UnregisterFrameCallback(IFrameCallback * cb)
{
    JankFrameMonitor::GetInstance().UnregisterFrameCallback(cb);
}

void PerfMonitorAdapter::RegisterAnimatorCallback(IAnimatorCallback * cb)
{
    AnimatorMonitor::GetInstance().RegisterAnimatorCallback(cb);
}

void PerfMonitorAdapter::UnregisterAnimatorCallback(IAnimatorCallback * cb)
{
    AnimatorMonitor::GetInstance().UnregisterAnimatorCallback(cb);
}

void PerfMonitorAdapter::RegisterSceneCallback(ISceneCallback* cb)
{
    SceneMonitor::GetInstance().RegisterSceneCallback(cb);
}

void PerfMonitorAdapter::UnregisterSceneCallback(ISceneCallback* cb)
{
    SceneMonitor::GetInstance().UnregisterSceneCallback(cb);
}

void PerfMonitorAdapter::RecordInputEvent(PerfActionType type, PerfSourceType sourceType, int64_t time)
{
    InputMonitor::GetInstance().RecordInputEvent(type, sourceType, time);
}

int64_t PerfMonitorAdapter::GetInputTime(const std::string& sceneId, PerfActionType type, const std::string& note)
{
    return InputMonitor::GetInstance().GetInputTime(sceneId, type, note);
}

void PerfMonitorAdapter::NotifyAppJankStatsBegin()
{
    SceneMonitor::GetInstance().NotifyAppJankStatsBegin();
}

void PerfMonitorAdapter::NotifyAppJankStatsEnd()
{
    SceneMonitor::GetInstance().NotifyAppJankStatsEnd();
}

void PerfMonitorAdapter::SetPageUrl(const std::string& pageUrl)
{
    SceneMonitor::GetInstance().SetPageUrl(pageUrl);
}

std::string PerfMonitorAdapter::GetPageUrl()
{
    return SceneMonitor::GetInstance().GetPageUrl();
}

void PerfMonitorAdapter::SetPageName(const std::string& pageName)
{
    SceneMonitor::GetInstance().SetPageName(pageName);
}

std::string PerfMonitorAdapter::GetPageName()
{
    return SceneMonitor::GetInstance().GetPageName();
}

void PerfMonitorAdapter::SetAppForeground(bool isShow)
{
    SceneMonitor::GetInstance().SetAppForeground(isShow);
}

void PerfMonitorAdapter::SetAppStartStatus()
{
    SceneMonitor::GetInstance().SetAppStartStatus();
}

void PerfMonitorAdapter::SetAppInfo(AceAppInfo& appInfo)
{
    SceneMonitor::GetInstance().SetAppInfo(appInfo);
}

bool PerfMonitorAdapter::IsScrollJank(const std::string& sceneId)
{
    return SceneMonitor::GetInstance().IsScrollJank(sceneId);
}

void PerfMonitorAdapter::Start(const std::string& sceneId, PerfActionType type, const std::string& note)
{
    AnimatorMonitor::GetInstance().Start(sceneId, type, note);
}

void PerfMonitorAdapter::End(const std::string& sceneId, bool isRsRender)
{
    AnimatorMonitor::GetInstance().End(sceneId, isRsRender);
}

void PerfMonitorAdapter::StartCommercial(const std::string& sceneId, PerfActionType type, const std::string& note)
{
    AnimatorMonitor::GetInstance().Start(sceneId, type, note);
}

void PerfMonitorAdapter::EndCommercial(const std::string& sceneId, bool isRsRender)
{
    AnimatorMonitor::GetInstance().End(sceneId, isRsRender);
}

void PerfMonitorAdapter::SetFrameTime(int64_t vsyncTime, int64_t duration, double jank, const std::string& windowName)
{
    JankFrameMonitor::GetInstance().OnFrameEnd(vsyncTime, duration, jank, windowName);
}

void PerfMonitorAdapter::SetSubHealthInfo(const std::string& info, const std::string& reason, const int32_t duration)
{
    SubHealthInfo subHealthInfo;
    subHealthInfo.info = info;
    subHealthInfo.subHealthReason = reason;
    subHealthInfo.subHealthTime = duration;
    AnimatorMonitor::GetInstance().SetSubHealthInfo(subHealthInfo);
}

void PerfMonitorAdapter::ReportJankFrameApp(double jank, int32_t jankThreshold)
{
    PerfReporter::GetInstance().ReportJankFrameApp(jank, jankThreshold);
}

void PerfMonitorAdapter::ReportPageShowMsg(const std::string& pageUrl, const std::string& bundleName,
    const std::string& pageName)
{
    PerfReporter::GetInstance().ReportPageShowMsg(pageUrl, bundleName, pageName);
}

void PerfMonitorAdapter::StartRecordImageLoadStat(int64_t id)
{
    WhiteBlockMonitor::GetInstance().StartRecordImageLoadStat(id);
}
 
void PerfMonitorAdapter::EndRecordImageLoadStat(int64_t id, std::pair<int, int> size, const std::string& type,
                                                int state)
{
    WhiteBlockMonitor::GetInstance().EndRecordImageLoadStat(id, size, type, state);
}

void PerfMonitorAdapter::OnSceneChanged(const SceneType& type, bool status)
{
    SceneMonitor::GetInstance().OnSceneChanged(type, status);
}

}
}