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

#include "animator_monitor.h"
#include "jank_frame_monitor.h"
#include "perf_reporter.h"
#include "perf_trace.h"
#include "perf_utils.h"
#include "scene_monitor.h"
#include "xperf_event_builder.h"
#include "xperf_event_reporter.h"

#include "hisysevent.h"
#include "render_service_client/core/transaction/rs_interfaces.h"
#include "hiview_logger.h"

#ifdef RESOURCE_SCHEDULE_SERVICE_ENABLE
#include "res_sched_client.h"
#include "res_type.h"
#endif // RESOURCE_SCHEDULE_SERVICE_ENABLE

namespace OHOS {
namespace HiviewDFX {

DEFINE_LOG_LABEL(0xD002D66, "Hiview-PerfMonitor");

namespace {
    constexpr char EVENT_KEY_PROCESS_NAME[] = "PROCESS_NAME";
    constexpr char EVENT_KEY_ABILITY_NAME[] = "ABILITY_NAME";
    constexpr char EVENT_KEY_PAGE_URL[] = "PAGE_URL";
    constexpr char EVENT_KEY_PAGE_NAME[] = "PAGE_NAME";
    constexpr char EVENT_KEY_VERSION_CODE[] = "VERSION_CODE";
    constexpr char EVENT_KEY_VERSION_NAME[] = "VERSION_NAME";
    constexpr char EVENT_KEY_BUNDLE_NAME[] = "BUNDLE_NAME";
    constexpr char EVENT_KEY_JANK_STATS[] = "JANK_STATS";
    constexpr char EVENT_KEY_JANK_STATS_VER[] = "JANK_STATS_VER";
    constexpr char EVENT_KEY_APP_PID[] = "APP_PID";
    constexpr char EVENT_KEY_SCENE_ID[] = "SCENE_ID";
    constexpr char EVENT_KEY_INPUT_TIME[] = "INPUT_TIME";
    constexpr char EVENT_KEY_ANIMATION_START_LATENCY[] = "ANIMATION_START_LATENCY";
    constexpr char EVENT_KEY_ANIMATION_END_LATENCY[] = "ANIMATION_END_LATENCY";
    constexpr char EVENT_KEY_E2E_LATENCY[] = "E2E_LATENCY";
    constexpr char EVENT_KEY_UNIQUE_ID[] = "UNIQUE_ID";
    constexpr char EVENT_KEY_MODULE_NAME[] = "MODULE_NAME";
    constexpr char EVENT_KEY_DURITION[] = "DURITION";
    constexpr char EVENT_KEY_TOTAL_FRAMES[] = "TOTAL_FRAMES";
    constexpr char EVENT_KEY_TOTAL_MISSED_FRAMES[] = "TOTAL_MISSED_FRAMES";
    constexpr char EVENT_KEY_MAX_FRAMETIME[] = "MAX_FRAMETIME";
    constexpr char EVENT_KEY_MAX_FRAMETIME_SINCE_START[] = "MAX_FRAMETIME_SINCE_START";
    constexpr char EVENT_KEY_MAX_HITCH_TIME[] = "MAX_HITCH_TIME";
    constexpr char EVENT_KEY_MAX_HITCH_TIME_SINCE_START[] = "MAX_HITCH_TIME_SINCE_START";
    constexpr char EVENT_KEY_MAX_SEQ_MISSED_FRAMES[] = "MAX_SEQ_MISSED_FRAMES";
    constexpr char EVENT_KEY_SOURCE_TYPE[] = "SOURCE_TYPE";
    constexpr char EVENT_KEY_NOTE[] = "NOTE";
    constexpr char EVENT_KEY_DISPLAY_ANIMATOR[] = "DISPLAY_ANIMATOR";
    constexpr char EVENT_KEY_SKIPPED_FRAME_TIME[] = "SKIPPED_FRAME_TIME";
    constexpr char EVENT_KEY_REAL_SKIPPED_FRAME_TIME[] = "REAL_SKIPPED_FRAME_TIME";
    constexpr char EVENT_KEY_FILTER_TYPE[] = "FILTER_TYPE";
    constexpr char EVENT_KEY_STARTTIME[] = "STARTTIME";
    constexpr char EVENT_KEY_SUBHEALTH_INFO[] = "SUB_HEALTH_INFO";
    constexpr char EVENT_KEY_SUBHEALTH_REASON[] = "SUB_HEALTH_REASON";
    constexpr char EVENT_KEY_SUBHEALTH_TIME[] = "SUB_HEALTH_TIME";
    constexpr char EVENT_KEY_VSYNC_TIME[] = "VSYNC_TIME";
    constexpr char STATISTIC_DURATION[] = "DURATION";
    constexpr char KEY_SCROLL_START_TIME[] = "SCROLL_START_TIME";
    constexpr char KEY_SCROLL_END_TIME[] = "SCROLL_END_TIME";
    constexpr char KEY_TOTAL_NUM[] = "TOTAL_NUM";
    constexpr char KEY_FAILED_NUM[] = "FAILED_NUM";
    constexpr char KEY_TOTAL_SIZE[] = "TOTAL_SIZE";
    constexpr char KEY_FAILED_SIZE[] = "FAILED_SIZE";
    constexpr char KEY_TYPE_DETAILS[] = "TYPE_DETAILS";
    constexpr char KEY_PID[] = "PID";
    constexpr char KEY_SURFACE_NAME[] = "SURFACE_NAME";
    constexpr char KEY_COMPONENT_NAME[] = "COMPONENT_NAME";

#ifdef RESOURCE_SCHEDULE_SERVICE_ENABLE
    constexpr int32_t MAX_JANK_FRAME_TIME = 32;
#endif // RESOURCE_SCHEDULE_SERVICE_ENABLE

} // namespace

PerfReporter& PerfReporter::GetInstance()
{
    static PerfReporter instance;
    return instance;
}

void PerfReporter::ReportJankFrameApp(double jank, int32_t jankThreshold)
{
    if (jank >= static_cast<double>(jankThreshold)) {
        JankInfo jankInfo;
        jankInfo.skippedFrameTime = static_cast<int64_t>(jank * SINGLE_FRAME_TIME);
        jankInfo.baseInfo = SceneMonitor::GetInstance().GetBaseInfo();
        EventReporter::ReportJankFrameApp(jankInfo);
    }
}

void PerfReporter::ReportPageShowMsg(const std::string& pageUrl, const std::string& bundleName,
    const std::string& pageName)
{
    EventReporter::ReportPageShowMsg(pageUrl, bundleName, pageName);
}

void PerfReporter::ReportAnimatorEvent(PerfEventType type, DataBase& data)
{
    switch (type) {
        case EVENT_RESPONSE:
            data.eventType = EVENT_RESPONSE;
            break;
        case EVENT_COMPLETE:
            data.eventType = EVENT_COMPLETE;
            break;
        case EVENT_JANK_FRAME:
            data.eventType = EVENT_JANK_FRAME;
            break;
        default :
            break;
    }
    ReportPerfEventToUI(data);
    ReportPerfEventToRS(data);
}

void PerfReporter::ReportPerfEventToRS(DataBase& data)
{
    OHOS::Rosen::DataBaseRs dataRs;
    ConvertToRsData(dataRs, data);
    switch (dataRs.eventType) {
        case EVENT_RESPONSE:
            {
                XPERF_TRACE_SCOPED("EVENT_REPORT_RESPONSE_RS sceneId = %s, uniqueId = %lld",
                    dataRs.sceneId.c_str(), static_cast<long long> (dataRs.uniqueId));
                Rosen::RSInterfaces::GetInstance().ReportEventResponse(dataRs);
                break;
            }
        case EVENT_COMPLETE:
            {
                if (data.needReportRs) {
                    XPERF_TRACE_SCOPED("EVENT_REPORT_COMPLETE_RS sceneId = %s, uniqueId = %lld",
                        dataRs.sceneId.c_str(), static_cast<long long> (dataRs.uniqueId));
                    Rosen::RSInterfaces::GetInstance().ReportEventComplete(dataRs);
                }
                break;
            }
        case EVENT_JANK_FRAME:
            {
                XPERF_TRACE_SCOPED("EVENT_REPORT_JANK_RS sceneId = %s, uniqueId = %lld",
                    dataRs.sceneId.c_str(), static_cast<long long> (dataRs.uniqueId));
                Rosen::RSInterfaces::GetInstance().ReportEventJankFrame(dataRs);
                break;
            }
        default :
            break;
    }
}

void PerfReporter::ReportPerfEventToUI(DataBase data)
{
    switch (data.eventType) {
        case EVENT_COMPLETE:
            if (!data.needReportRs) {
                EventReporter::ReportEventComplete(data);
            }
            break;
        case EVENT_JANK_FRAME:
            if (data.totalFrames > 0) {
                EventReporter::ReportEventJankFrame(data);
            }
            break;
        default :
            break;
    }
}

void PerfReporter::ConvertToRsData(OHOS::Rosen::DataBaseRs &dataRs, DataBase& data)
{
    dataRs.eventType = static_cast<int32_t>(data.eventType);
    dataRs.sceneId = data.sceneId;
    dataRs.appPid = data.baseInfo.pid;
    dataRs.uniqueId = data.beginVsyncTime / NS_TO_MS;
    dataRs.inputTime = data.inputTime;
    dataRs.beginVsyncTime = data.beginVsyncTime;
    dataRs.endVsyncTime = data.endVsyncTime;
    dataRs.versionCode = data.baseInfo.versionCode;
    dataRs.versionName = data.baseInfo.versionName;
    dataRs.bundleName = data.baseInfo.bundleName;
    dataRs.processName = data.baseInfo.processName;
    dataRs.abilityName = data.baseInfo.abilityName;
    dataRs.pageUrl = data.baseInfo.pageUrl;
    dataRs.sourceType = GetSourceTypeName(data.sourceType);
    dataRs.note = data.baseInfo.note;
    dataRs.isDisplayAnimator = data.isDisplayAnimator;
}

void PerfReporter::ReportSingleJankFrame(JankInfo& jankInfo)
{
    EventReporter::ReportJankFrameUnFiltered(jankInfo);
    if (!jankInfo.sceneTag) {
        EventReporter::ReportJankFrameFiltered(jankInfo);
    }
}

void PerfReporter::ReportStatsJankFrame(int64_t jankFrameRecordBeginTime, int64_t duration,
    const std::vector<uint16_t>& jankFrameRecord, int32_t jankFrameTotalCount, const BaseInfo& baseInfo)
{
    XPERF_TRACE_SCOPED("ReportJankStatsApp count=%" PRId32 ";duration=%" PRId64 ";beginTime=%" PRId64 ";",
        jankFrameTotalCount, duration, jankFrameRecordBeginTime);
    if (duration > DEFAULT_VSYNC && jankFrameTotalCount > 0 && jankFrameRecordBeginTime > 0) {
        EventReporter::ReportStatsJankFrame(jankFrameRecordBeginTime, duration, jankFrameRecord,
            baseInfo, JANK_STATS_VERSION);
    }
}

void PerfReporter::ReportWhiteBlockStat(uint64_t scrollStartTime, uint64_t scrollEndTime,
                                        const std::map<int64_t, ImageLoadInfo*>& mRecords)
{
    if (mRecords.size() == 0) {
        HIVIEW_LOGD("no data to report");
        return;
    }
    std::string imageLoadStat;
    int totalNum = 0; //总个数
    int failedNum = 0; //失败个数
    int64_t totalSize = 0; //总大小
    int64_t failedSize = 0; //失败总大小
    std::map<std::string, std::pair<int, int>> typeDetails;
    int64_t size = 0;
    for (const auto& pair : mRecords) {
        ImageLoadInfo* record = pair.second;
        if (record == nullptr) {
            continue;
        }
        if (record->loadEndTime == 0) {
            continue;
        }
        size = (record->width * record->height);
        totalNum++;
        totalSize += size;
        if (record->loadState == 0) {
            failedNum ++;
            failedSize += size;
        }
 
        auto it = typeDetails.find(record->imageType);
        if (it != typeDetails.end()) {
            (it->second).first++;
            if (record->loadState == 0) {
                (it->second).second++;
            }
        } else {
            typeDetails.emplace(record->imageType, std::make_pair(1, ((record->loadState == 0) ? 1 : 0)));
        }
    }
    for (const auto& typeDetail : typeDetails) {
        std::string str = ("type=" + typeDetail.first + ",total=" + std::to_string(typeDetail.second.first) +
                ",failed=" + std::to_string(typeDetail.second.second) + ";");
        imageLoadStat += str;
    }
 
    ImageLoadStat stat = {scrollStartTime, scrollEndTime, totalNum, failedNum, totalSize, failedSize, imageLoadStat};
    EventReporter::ReportImageLoadStat(stat);
}

void PerfReporter::ReportSurface(const int64_t& uniqueId, const std::string& surfaceName,
    const std::string& componentName, const std::string& bundleName, const int32_t& pid)
{
    SurfaceInfo surfaceInfo = {uniqueId, surfaceName, componentName, bundleName, pid};
    EventReporter::ReportSurfaceInfo(surfaceInfo);
}

void EventReporter::ReportJankFrameApp(JankInfo& info)
{
    std::string eventName = "JANK_FRAME_APP";
    const auto& bundleName = info.baseInfo.bundleName;
    const auto& processName = info.baseInfo.processName;
    const auto& abilityName = info.baseInfo.abilityName;
    const auto& pageUrl = info.baseInfo.pageUrl;
    const auto& versionCode = info.baseInfo.versionCode;
    const auto& versionName = info.baseInfo.versionName;
    const auto& pageName = info.baseInfo.pageName;
    const auto& skippedFrameTime = info.skippedFrameTime;
    XperfEventBuilder builder;
    XperfEvent event = builder.EventName(eventName)
        .EventType(HISYSEVENT_FAULT)
        .Param(EVENT_KEY_PROCESS_NAME, processName)
        .Param(EVENT_KEY_MODULE_NAME, bundleName)
        .Param(EVENT_KEY_ABILITY_NAME, abilityName)
        .Param(EVENT_KEY_PAGE_URL, pageUrl)
        .Param(EVENT_KEY_VERSION_CODE, versionCode)
        .Param(EVENT_KEY_VERSION_NAME, versionName)
        .Param(EVENT_KEY_PAGE_NAME, pageName)
        .Param(EVENT_KEY_SKIPPED_FRAME_TIME, static_cast<uint64_t>(skippedFrameTime))
        .Build();
    XperfEventReporter reporter;
    reporter.Report(ACE_DOMAIN, event);
    XPERF_TRACE_SCOPED("JANK_FRAME_APP: skipppedFrameTime=%lld(ms)",
        static_cast<long long>(skippedFrameTime / NS_TO_MS));
#ifdef RESOURCE_SCHEDULE_SERVICE_ENABLE
    ReportAppFrameDropToRss(false, bundleName);
#endif // RESOURCE_SCHEDULE_SERVICE_ENABLE
}

void EventReporter::ReportPageShowMsg(const std::string& pageUrl, const std::string& bundleName,
    const std::string& pageName)
{
    XperfEventBuilder builder;
    XperfEvent event = builder.EventName("APP_PAGE_INFO_UPDATE")
        .EventType(HISYSEVENT_BEHAVIOR)
        .Param(EVENT_KEY_PAGE_URL, pageUrl)
        .Param(EVENT_KEY_BUNDLE_NAME, bundleName)
        .Param(EVENT_KEY_PAGE_NAME, pageName)
        .Build();
    XperfEventReporter reporter;
    reporter.Report(ACE_DOMAIN, event);
}

void EventReporter::ReportEventComplete(DataBase& data)
{
    std::string eventName = "INTERACTION_COMPLETED_LATENCY";
    const auto& appPid = data.baseInfo.pid;
    const auto& bundleName = data.baseInfo.bundleName;
    const auto& processName = data.baseInfo.processName;
    const auto& abilityName = data.baseInfo.abilityName;
    const auto& pageUrl = data.baseInfo.pageUrl;
    const auto& versionCode = data.baseInfo.versionCode;
    const auto& versionName = data.baseInfo.versionName;
    const auto& pageName = data.baseInfo.pageName;
    const auto& sceneId = data.sceneId;
    const auto& sourceType = GetSourceTypeName(data.sourceType);
    auto inputTime = data.inputTime;
    ConvertRealtimeToSystime(data.inputTime, inputTime);
    const auto& animationStartLantency = (data.beginVsyncTime - data.inputTime) / NS_TO_MS;
    const auto& animationEndLantency = (data.endVsyncTime - data.beginVsyncTime) / NS_TO_MS;
    const auto& e2eLatency = animationStartLantency + animationEndLantency;
    const auto& note = data.baseInfo.note;
    const auto& extend = data.baseInfo.subHealthInfo.info;
    const auto& reason = data.baseInfo.subHealthInfo.subHealthReason;
    const auto& subHealthTime = data.baseInfo.subHealthInfo.subHealthTime;
    XperfEventBuilder builder;
    XperfEvent event = builder.EventName(eventName)
        .EventType(HISYSEVENT_BEHAVIOR)
        .Param(EVENT_KEY_APP_PID, appPid)
        .Param(EVENT_KEY_BUNDLE_NAME, bundleName)
        .Param(EVENT_KEY_PROCESS_NAME, processName)
        .Param(EVENT_KEY_ABILITY_NAME, abilityName)
        .Param(EVENT_KEY_PAGE_URL, pageUrl)
        .Param(EVENT_KEY_VERSION_CODE, versionCode)
        .Param(EVENT_KEY_VERSION_NAME, versionName)
        .Param(EVENT_KEY_PAGE_NAME, pageName)
        .Param(EVENT_KEY_SCENE_ID, sceneId)
        .Param(EVENT_KEY_SOURCE_TYPE, sourceType)
        .Param(EVENT_KEY_INPUT_TIME, static_cast<uint64_t>(inputTime))
        .Param(EVENT_KEY_ANIMATION_START_LATENCY, static_cast<uint64_t>(animationStartLantency))
        .Param(EVENT_KEY_ANIMATION_END_LATENCY, static_cast<uint64_t>(animationEndLantency))
        .Param(EVENT_KEY_E2E_LATENCY, static_cast<uint64_t>(e2eLatency))
        .Param(EVENT_KEY_NOTE, note)
        .Param(EVENT_KEY_SUBHEALTH_INFO, extend)
        .Param(EVENT_KEY_SUBHEALTH_REASON, reason)
        .Param(EVENT_KEY_SUBHEALTH_TIME, static_cast<int32_t>(subHealthTime))
        .Build();
    XperfEventReporter reporter;
    reporter.Report(ACE_DOMAIN, event);
    XPERF_TRACE_SCOPED("INTERACTION_COMPLETED_LATENCY: sceneId =%s, inputTime=%lld(ms),"
        "e2eLatency=%lld(ms)", sceneId.c_str(),
        static_cast<long long>(inputTime), static_cast<long long>(e2eLatency));
}

void EventReporter::ReportEventJankFrame(DataBase& data)
{
    std::string eventName = "INTERACTION_APP_JANK";
    const auto& uniqueId = data.beginVsyncTime / NS_TO_MS;
    auto startTime = data.beginVsyncTime;
    ConvertRealtimeToSystime(data.beginVsyncTime, startTime);
    const auto& durition = (data.endVsyncTime - data.beginVsyncTime) / NS_TO_MS;
    const auto& maxFrameTime = data.maxFrameTime / NS_TO_MS;
    XperfEventBuilder builder;
    XperfEvent event = builder.EventName(eventName)
        .EventType(HISYSEVENT_BEHAVIOR)
        .Param(EVENT_KEY_UNIQUE_ID, static_cast<int32_t>(uniqueId)).Param(EVENT_KEY_SCENE_ID, data.sceneId)
        .Param(EVENT_KEY_PROCESS_NAME, data.baseInfo.processName)
        .Param(EVENT_KEY_MODULE_NAME, data.baseInfo.bundleName)
        .Param(EVENT_KEY_ABILITY_NAME, data.baseInfo.abilityName).Param(EVENT_KEY_PAGE_URL, data.baseInfo.pageUrl)
        .Param(EVENT_KEY_VERSION_CODE, data.baseInfo.versionCode)
        .Param(EVENT_KEY_VERSION_NAME, data.baseInfo.versionName)
        .Param(EVENT_KEY_PAGE_NAME, data.baseInfo.pageName)
        .Param(EVENT_KEY_STARTTIME, static_cast<uint64_t>(startTime))
        .Param(EVENT_KEY_DURITION, static_cast<uint64_t>(durition))
        .Param(EVENT_KEY_TOTAL_FRAMES, data.totalFrames).Param(EVENT_KEY_TOTAL_MISSED_FRAMES, data.totalMissed)
        .Param(EVENT_KEY_MAX_FRAMETIME, static_cast<uint64_t>(maxFrameTime))
        .Param(EVENT_KEY_MAX_FRAMETIME_SINCE_START, static_cast<uint64_t>(data.maxFrameTimeSinceStart))
        .Param(EVENT_KEY_MAX_HITCH_TIME, static_cast<uint64_t>(data.maxHitchTime))
        .Param(EVENT_KEY_MAX_HITCH_TIME_SINCE_START, static_cast<uint64_t>(data.maxHitchTimeSinceStart))
        .Param(EVENT_KEY_MAX_SEQ_MISSED_FRAMES, data.maxSuccessiveFrames)
        .Param(EVENT_KEY_NOTE, data.baseInfo.note).Param(EVENT_KEY_DISPLAY_ANIMATOR, data.isDisplayAnimator)
        .Param(EVENT_KEY_SUBHEALTH_INFO, data.baseInfo.subHealthInfo.info)
        .Param(EVENT_KEY_SUBHEALTH_REASON, data.baseInfo.subHealthInfo.subHealthReason)
        .Param(EVENT_KEY_SUBHEALTH_TIME, static_cast<int32_t>(data.baseInfo.subHealthInfo.subHealthTime))
        .Build();
    XperfEventReporter reporter;
    reporter.Report(ACE_DOMAIN, event);
    XPERF_TRACE_SCOPED("INTERACTION_APP_JANK: sceneId =%s, startTime=%lld(ms),"
        "maxFrameTime=%lld(ms), pageName=%s", data.sceneId.c_str(), static_cast<long long>(startTime),
        static_cast<long long>(maxFrameTime), data.baseInfo.pageName.c_str());
#ifdef RESOURCE_SCHEDULE_SERVICE_ENABLE
    if (data.isDisplayAnimator && maxFrameTime > MAX_JANK_FRAME_TIME) {
        ReportAppFrameDropToRss(true, data.baseInfo.bundleName, maxFrameTime);
    }
#endif // RESOURCE_SCHEDULE_SERVICE_ENABLE
}

void EventReporter::ReportStatsJankFrame(int64_t startTime, int64_t duration, const std::vector<uint16_t>& jank,
    const BaseInfo& baseInfo, uint32_t jankStatusVersion)
{
    std::string eventName = "JANK_STATS_APP";
    auto app_version_code = baseInfo.versionCode;
    auto app_version_name = baseInfo.versionName;
    auto packageName = baseInfo.bundleName;
    auto abilityName = baseInfo.abilityName;
    auto pageUrl = baseInfo.pageUrl;
    XperfEventBuilder builder;
    XperfEvent event = builder.EventName(eventName)
        .EventType(HISYSEVENT_STATISTIC)
        .Param(EVENT_KEY_STARTTIME, startTime)
        .Param(STATISTIC_DURATION, duration)
        .Param(EVENT_KEY_VERSION_CODE, app_version_code)
        .Param(EVENT_KEY_VERSION_NAME, app_version_name)
        .Param(EVENT_KEY_BUNDLE_NAME, packageName)
        .Param(EVENT_KEY_ABILITY_NAME, abilityName)
        .Param(EVENT_KEY_PAGE_URL, pageUrl)
        .Param(EVENT_KEY_JANK_STATS, jank)
        .Param(EVENT_KEY_JANK_STATS_VER, jankStatusVersion)
        .Build();
    XperfEventReporter reporter;
    reporter.Report(ACE_DOMAIN, event);
}

void EventReporter::ReportJankFrameFiltered(JankInfo& info)
{
    std::string eventName = "JANK_FRAME_FILTERED";
    const auto& bundleName = info.baseInfo.bundleName;
    const auto& processName = info.baseInfo.processName;
    const auto& abilityName = info.baseInfo.abilityName;
    const auto& pageUrl = info.baseInfo.pageUrl;
    const auto& versionCode = info.baseInfo.versionCode;
    const auto& versionName = info.baseInfo.versionName;
    const auto& pageName = info.baseInfo.pageName;
    const auto& skippedFrameTime = info.skippedFrameTime;
    const auto& windowName = info.windowName;
    XperfEventBuilder builder;
    XperfEvent event = builder.EventName(eventName)
        .EventType(HISYSEVENT_BEHAVIOR)
        .Param(EVENT_KEY_PROCESS_NAME, processName)
        .Param(EVENT_KEY_MODULE_NAME, bundleName)
        .Param(EVENT_KEY_ABILITY_NAME, abilityName)
        .Param(EVENT_KEY_PAGE_URL, pageUrl)
        .Param(EVENT_KEY_VERSION_CODE, versionCode)
        .Param(EVENT_KEY_VERSION_NAME, versionName)
        .Param(EVENT_KEY_PAGE_NAME, pageName)
        .Param(EVENT_KEY_SKIPPED_FRAME_TIME, static_cast<uint64_t>(skippedFrameTime))
        .Build();
    XperfEventReporter reporter;
    reporter.Report(ACE_DOMAIN, event);
    XPERF_TRACE_SCOPED("JANK_FRAME_FILTERED: skipppedFrameTime=%lld(ms), windowName=%s",
        static_cast<long long>(skippedFrameTime / NS_TO_MS), windowName.c_str());
}

void EventReporter::ReportJankFrameUnFiltered(JankInfo& info)
{
    std::string eventName = "JANK_FRAME_UNFILTERED";
    const auto& bundleName = info.baseInfo.bundleName;
    const auto& processName = info.baseInfo.processName;
    const auto& abilityName = info.baseInfo.abilityName;
    const auto& pageUrl = info.baseInfo.pageUrl;
    const auto& versionCode = info.baseInfo.versionCode;
    const auto& versionName = info.baseInfo.versionName;
    const auto& pageName = info.baseInfo.pageName;
    const auto& skippedFrameTime = info.skippedFrameTime;
    const auto& realSkippedFrameTime = info.realSkippedFrameTime;
    const auto& windowName = info.windowName;
    const auto& sceneId = info.sceneId;
    const auto& extend = info.baseInfo.subHealthInfo.info;
    const auto& reason = info.baseInfo.subHealthInfo.subHealthReason;
    const auto& subHealthTime = info.baseInfo.subHealthInfo.subHealthTime;
    const auto& sceneTag = info.sceneTag;
    const auto& vsyncTime = info.vsyncTime;
    XperfEventBuilder builder;
    XperfEvent event = builder.EventName(eventName)
        .EventType(HISYSEVENT_BEHAVIOR)
        .Param(EVENT_KEY_PROCESS_NAME, processName)
        .Param(EVENT_KEY_MODULE_NAME, bundleName)
        .Param(EVENT_KEY_ABILITY_NAME, abilityName)
        .Param(EVENT_KEY_PAGE_URL, pageUrl)
        .Param(EVENT_KEY_VERSION_CODE, versionCode)
        .Param(EVENT_KEY_VERSION_NAME, versionName)
        .Param(EVENT_KEY_PAGE_NAME, pageName)
        .Param(EVENT_KEY_FILTER_TYPE, sceneTag)
        .Param(EVENT_KEY_SCENE_ID, sceneId)
        .Param(EVENT_KEY_REAL_SKIPPED_FRAME_TIME, static_cast<uint64_t>(realSkippedFrameTime))
        .Param(EVENT_KEY_SKIPPED_FRAME_TIME, static_cast<uint64_t>(skippedFrameTime))
        .Param(EVENT_KEY_SUBHEALTH_INFO, extend)
        .Param(EVENT_KEY_SUBHEALTH_REASON, reason)
        .Param(EVENT_KEY_SUBHEALTH_TIME, static_cast<int32_t>(subHealthTime))
        .Param(EVENT_KEY_VSYNC_TIME, static_cast<uint64_t>(vsyncTime))
        .Build();
    XperfEventReporter reporter;
    reporter.Report(ACE_DOMAIN, event);
    XPERF_TRACE_SCOPED("JANK_FRAME_UNFILTERED: skipppedFrameTime=%lld(ms), windowName=%s, filterType=%" PRIu64
        ",vsyncTime=%lld(ns)", static_cast<long long>(skippedFrameTime / NS_TO_MS), windowName.c_str(),
        sceneTag, static_cast<long long>(vsyncTime));
}

#ifdef RESOURCE_SCHEDULE_SERVICE_ENABLE
void EventReporter::ReportAppFrameDropToRss(const bool isInteractionJank, const std::string &bundleName,
    const int64_t maxFrameTime)
{
    uint32_t eventType = ResourceSchedule::ResType::RES_TYPE_APP_FRAME_DROP;
    int32_t subType = isInteractionJank ? ResourceSchedule::ResType::AppFrameDropType::INTERACTION_APP_JANK
                                        : ResourceSchedule::ResType::AppFrameDropType::JANK_FRAME_APP;
    std::unordered_map<std::string, std::string> payload = {
        { "bundleName", bundleName },
        { "maxFrameTime", std::to_string(maxFrameTime) },
    };
    ResourceSchedule::ResSchedClient::GetInstance().ReportData(eventType, subType, payload);
}
#endif // RESOURCE_SCHEDULE_SERVICE_ENABLE

void EventReporter::ReportImageLoadStat(const ImageLoadStat& stat)
{
    XperfEventBuilder builder;
    XperfEvent event = builder.EventName("SCROLL_IMAGE_STAT")
            .EventType(HISYSEVENT_STATISTIC)
            .Param(KEY_SCROLL_START_TIME, stat.startTime)
            .Param(KEY_SCROLL_END_TIME, stat.endTime)
            .Param(KEY_TOTAL_NUM, stat.totalNum)
            .Param(KEY_FAILED_NUM, stat.failedNum)
            .Param(KEY_TOTAL_SIZE, stat.totalSize)
            .Param(KEY_FAILED_SIZE, stat.failedSize)
            .Param(KEY_TYPE_DETAILS, stat.typeDetails)
            .Build();
    XperfEventReporter reporter;
    reporter.Report(PERFORMANCE_DOMAIN, event);
}

void EventReporter::ReportSurfaceInfo(const SurfaceInfo& surface)
{
    XperfEventBuilder builder;
    XperfEvent event = builder.EventName("SURFACE_ATTACH")
            .EventType(HISYSEVENT_BEHAVIOR)
            .Param(KEY_PID, surface.pid)
            .Param(EVENT_KEY_BUNDLE_NAME, surface.bundleName)
            .Param(EVENT_KEY_UNIQUE_ID, surface.uniqueId)
            .Param(KEY_SURFACE_NAME, surface.surfaceName)
            .Param(KEY_COMPONENT_NAME, surface.componentName)
            .Build();
    XperfEventReporter reporter;
    reporter.Report(ACE_DOMAIN, event);
}
}
}