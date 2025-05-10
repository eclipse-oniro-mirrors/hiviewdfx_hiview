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

#include "hisysevent.h"
#include "render_service_client/core/transaction/rs_interfaces.h"
#ifdef RESOURCE_SCHEDULE_SERVICE_ENABLE
#include "res_sched_client.h"
#include "res_type.h"
#endif // RESOURCE_SCHEDULE_SERVICE_ENABLE

namespace OHOS {
namespace HiviewDFX {

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
    constexpr char STATISTIC_DURATION[] = "DURATION";
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
    if (jank >= static_cast<double>(jankThreshold) && !JankFrameMonitor::GetInstance().GetIsBackgroundApp()) {
        JankInfo jankInfo;
        jankInfo.skippedFrameTime = static_cast<int64_t>(jank * SINGLE_FRAME_TIME);
        SceneMonitor::GetInstance().RecordBaseInfo(nullptr);
        jankInfo.baseInfo = SceneMonitor::GetInstance().GetBaseInfo();
        EventReporter::ReportJankFrameApp(jankInfo);
    }
}

void PerfReporter::ReportPageShowMsg(const std::string& pageUrl, const std::string& bundleName,
    const std::string& pageName)
{
    EventReporter::ReportPageShowMsg(pageUrl, bundleName, pageName);
}

void PerfReporter::ReportAnimateStart(const std::string& sceneId, SceneRecord* record)
{
    if (record == nullptr) {
        return;
    }
    DataBase data;
    FlushDataBase(record, data);
    ReportPerfEvent(EVENT_RESPONSE, data);
}

void PerfReporter::ReportAnimateEnd(const std::string& sceneId, SceneRecord* record)
{
    if (record == nullptr) {
        return;
    }
    DataBase data;
    FlushDataBase(record, data);
    ReportPerfEvent(EVENT_JANK_FRAME, data);
    ReportPerfEvent(EVENT_COMPLETE, data);
}

void PerfReporter::FlushDataBase(SceneRecord* record, DataBase& data)
{
    if (record == nullptr) {
        return;
    }
    data.sceneId = record->sceneId;
    data.inputTime = record->inputTime;
    data.beginVsyncTime = record->beginVsyncTime;
    if (data.beginVsyncTime < data.inputTime) {
        data.inputTime = data.beginVsyncTime;
    }
    data.endVsyncTime = record->endVsyncTime;
    if (data.beginVsyncTime > data.endVsyncTime) {
        data.endVsyncTime = data.beginVsyncTime;
    }
    data.maxFrameTime = record->maxFrameTime;
    data.maxFrameTimeSinceStart = record->maxFrameTimeSinceStart;
    data.maxHitchTime = record->maxHitchTime;
    data.maxHitchTimeSinceStart = record->maxHitchTimeSinceStart;
    data.maxSuccessiveFrames = record->maxSuccessiveFrames;
    data.totalMissed = record->totalMissed;
    data.totalFrames = record->totalFrames;
    data.needReportRs = record->needReportRs;
    data.isDisplayAnimator = record->isDisplayAnimator;
    data.sourceType = record->sourceType;
    data.actionType = record->actionType;
    data.baseInfo = SceneMonitor::GetInstance().GetBaseInfo();
}

void PerfReporter::ReportPerfEvent(PerfEventType type, DataBase& data)
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

void PerfReporter::ReportJankStatsApp(int64_t duration)
{
    int32_t jankFrameTotalCount = JankFrameMonitor::GetInstance().GetJankFrameTotalCount();
    int64_t jankFrameRecordBeginTime = JankFrameMonitor::GetInstance().GetJankFrameRecordBeginTime();
    const auto& baseInfo = SceneMonitor::GetInstance().GetBaseInfo();
    const auto& jankFrameRecord = JankFrameMonitor::GetInstance().GetJankFrameRecord();
    XPERF_TRACE_SCOPED("ReportJankStatsApp count=%" PRId32 ";duration=%" PRId64 ";beginTime=%" PRId64 ";",
        jankFrameTotalCount, duration, jankFrameRecordBeginTime);
    if (duration > DEFAULT_VSYNC && jankFrameTotalCount > 0 && jankFrameRecordBeginTime > 0) {
        EventReporter::JankFrameReport(jankFrameRecordBeginTime, duration, jankFrameRecord,
            baseInfo.pageUrl, JANK_STATS_VERSION);
    }
    JankFrameMonitor::GetInstance().ClearJankFrameRecord();
}

void PerfReporter::ReportJankFrame(double jank, const std::string& windowName)
{
    if (jank >= static_cast<double>(DEFAULT_JANK_REPORT_THRESHOLD)) {
        JankInfo jankInfo;
        jankInfo.skippedFrameTime = static_cast<int64_t>(jank * SINGLE_FRAME_TIME);
        jankInfo.windowName = windowName;
        SceneMonitor::GetInstance().RecordBaseInfo(nullptr);
        jankInfo.baseInfo = SceneMonitor::GetInstance().GetBaseInfo();
        jankInfo.filterType = JankFrameMonitor::GetInstance().GetFilterType();
        if (!AnimatorMonitor::GetInstance().RecordsIsEmpty()) {
            jankInfo.sceneId = SceneMonitor::GetInstance().GetCurrentSceneId();
        } else {
            jankInfo.sceneId = DEFAULT_SCENE_ID;
        }
        jankInfo.realSkippedFrameTime = jankInfo.filterType == 0 ? jankInfo.skippedFrameTime : 0;
        EventReporter::ReportJankFrameUnFiltered(jankInfo);
        if (!JankFrameMonitor::GetInstance().IsExclusionFrame()) {
            EventReporter::ReportJankFrameFiltered(jankInfo);
        }
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
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::ACE, eventName,
        OHOS::HiviewDFX::HiSysEvent::EventType::FAULT,
        EVENT_KEY_PROCESS_NAME, processName,
        EVENT_KEY_MODULE_NAME, bundleName,
        EVENT_KEY_ABILITY_NAME, abilityName,
        EVENT_KEY_PAGE_URL, pageUrl,
        EVENT_KEY_VERSION_CODE, versionCode,
        EVENT_KEY_VERSION_NAME, versionName,
        EVENT_KEY_PAGE_NAME, pageName,
        EVENT_KEY_SKIPPED_FRAME_TIME, static_cast<uint64_t>(skippedFrameTime));
    XPERF_TRACE_SCOPED("JANK_FRAME_APP: skipppedFrameTime=%lld(ms)", static_cast<long long>(skippedFrameTime / NS_TO_MS));
#ifdef RESOURCE_SCHEDULE_SERVICE_ENABLE
    ReportAppFrameDropToRss(false, bundleName);
#endif // RESOURCE_SCHEDULE_SERVICE_ENABLE
}

void EventReporter::ReportPageShowMsg(const std::string& pageUrl, const std::string& bundleName,
    const std::string& pageName)
{
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::ACE, "APP_PAGE_INFO_UPDATE",
        OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        EVENT_KEY_PAGE_URL, pageUrl, EVENT_KEY_BUNDLE_NAME, bundleName,
        EVENT_KEY_PAGE_NAME, pageName);
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
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::ACE, eventName,
        OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        EVENT_KEY_APP_PID, appPid,
        EVENT_KEY_BUNDLE_NAME, bundleName,
        EVENT_KEY_PROCESS_NAME, processName,
        EVENT_KEY_ABILITY_NAME, abilityName,
        EVENT_KEY_PAGE_URL, pageUrl,
        EVENT_KEY_VERSION_CODE, versionCode,
        EVENT_KEY_VERSION_NAME, versionName,
        EVENT_KEY_PAGE_NAME, pageName,
        EVENT_KEY_SCENE_ID, sceneId,
        EVENT_KEY_SOURCE_TYPE, sourceType,
        EVENT_KEY_INPUT_TIME, static_cast<uint64_t>(inputTime),
        EVENT_KEY_ANIMATION_START_LATENCY, static_cast<uint64_t>(animationStartLantency),
        EVENT_KEY_ANIMATION_END_LATENCY, static_cast<uint64_t>(animationEndLantency),
        EVENT_KEY_E2E_LATENCY, static_cast<uint64_t>(e2eLatency),
        EVENT_KEY_NOTE, note);
    XPERF_TRACE_SCOPED("INTERACTION_COMPLETED_LATENCY: sceneId =%s, inputTime=%lld(ms),"
        "e2eLatency=%lld(ms)", sceneId.c_str(),
        static_cast<long long>(inputTime), static_cast<long long>(e2eLatency));
}

void EventReporter::ReportEventJankFrame(DataBase& data)
{
    std::string eventName = "INTERACTION_APP_JANK";
    const auto& uniqueId = data.beginVsyncTime / NS_TO_MS;
    const auto& sceneId = data.sceneId;
    const auto& bundleName = data.baseInfo.bundleName;
    const auto& processName = data.baseInfo.processName;
    const auto& abilityName = data.baseInfo.abilityName;
    auto startTime = data.beginVsyncTime;
    ConvertRealtimeToSystime(data.beginVsyncTime, startTime);
    const auto& durition = (data.endVsyncTime - data.beginVsyncTime) / NS_TO_MS;
    const auto& totalFrames = data.totalFrames;
    const auto& totalMissedFrames = data.totalMissed;
    const auto& maxFrameTime = data.maxFrameTime / NS_TO_MS;
    const auto& maxFrameTimeSinceStart = data.maxFrameTimeSinceStart;
    const auto& maxHitchTime = data.maxHitchTime;
    const auto& maxHitchTimeSinceStart = data.maxHitchTimeSinceStart;
    const auto& maxSeqMissedFrames = data.maxSuccessiveFrames;
    const auto& note = data.baseInfo.note;
    const auto& isDisplayAnimator = data.isDisplayAnimator;
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::ACE, eventName,
        OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        EVENT_KEY_UNIQUE_ID, static_cast<int32_t>(uniqueId),
        EVENT_KEY_SCENE_ID, sceneId,
        EVENT_KEY_PROCESS_NAME, processName,
        EVENT_KEY_MODULE_NAME, bundleName,
        EVENT_KEY_ABILITY_NAME, abilityName,
        EVENT_KEY_PAGE_URL, data.baseInfo.pageUrl,
        EVENT_KEY_VERSION_CODE, data.baseInfo.versionCode,
        EVENT_KEY_VERSION_NAME, data.baseInfo.versionName,
        EVENT_KEY_PAGE_NAME, data.baseInfo.pageName,
        EVENT_KEY_STARTTIME, static_cast<uint64_t>(startTime),
        EVENT_KEY_DURITION, static_cast<uint64_t>(durition),
        EVENT_KEY_TOTAL_FRAMES, totalFrames,
        EVENT_KEY_TOTAL_MISSED_FRAMES, totalMissedFrames,
        EVENT_KEY_MAX_FRAMETIME, static_cast<uint64_t>(maxFrameTime),
        EVENT_KEY_MAX_FRAMETIME_SINCE_START, static_cast<uint64_t>(maxFrameTimeSinceStart),
        EVENT_KEY_MAX_HITCH_TIME, static_cast<uint64_t>(maxHitchTime),
        EVENT_KEY_MAX_HITCH_TIME_SINCE_START, static_cast<uint64_t>(maxHitchTimeSinceStart),
        EVENT_KEY_MAX_SEQ_MISSED_FRAMES, maxSeqMissedFrames,
        EVENT_KEY_NOTE, note,
        EVENT_KEY_DISPLAY_ANIMATOR, isDisplayAnimator);
    XPERF_TRACE_SCOPED("INTERACTION_APP_JANK: sceneId =%s, startTime=%lld(ms),"
        "maxFrameTime=%lld(ms), pageName=%s", sceneId.c_str(), static_cast<long long>(startTime),
        static_cast<long long>(maxFrameTime), data.baseInfo.pageName.c_str());
#ifdef RESOURCE_SCHEDULE_SERVICE_ENABLE
    if (isDisplayAnimator && maxFrameTime > MAX_JANK_FRAME_TIME) {
        ReportAppFrameDropToRss(true, bundleName, maxFrameTime);
    }
#endif // RESOURCE_SCHEDULE_SERVICE_ENABLE
}

void EventReporter::JankFrameReport(int64_t startTime, int64_t duration, const std::vector<uint16_t>& jank,
    const std::string& pageUrl, uint32_t jankStatusVersion)
{
    std::string eventName = "JANK_STATS_APP";
    auto baseInfo = SceneMonitor::GetInstance().GetBaseInfo();
    auto app_version_code = baseInfo.versionCode;
    auto app_version_name = baseInfo.versionName;
    auto packageName = baseInfo.bundleName;
    auto abilityName = baseInfo.abilityName;
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::ACE, eventName,
        OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC,
        EVENT_KEY_STARTTIME, startTime,
        STATISTIC_DURATION, duration,
        EVENT_KEY_VERSION_CODE, app_version_code,
        EVENT_KEY_VERSION_NAME, app_version_name,
        EVENT_KEY_BUNDLE_NAME, packageName,
        EVENT_KEY_ABILITY_NAME, abilityName,
        EVENT_KEY_PAGE_URL, pageUrl,
        EVENT_KEY_JANK_STATS, jank,
        EVENT_KEY_JANK_STATS_VER, jankStatusVersion);
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
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::ACE, eventName,
        OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        EVENT_KEY_PROCESS_NAME, processName,
        EVENT_KEY_MODULE_NAME, bundleName,
        EVENT_KEY_ABILITY_NAME, abilityName,
        EVENT_KEY_PAGE_URL, pageUrl,
        EVENT_KEY_VERSION_CODE, versionCode,
        EVENT_KEY_VERSION_NAME, versionName,
        EVENT_KEY_PAGE_NAME, pageName,
        EVENT_KEY_SKIPPED_FRAME_TIME, static_cast<uint64_t>(skippedFrameTime));
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
    const auto& filterType = info.filterType;
    const auto& sceneId = info.sceneId;
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::ACE, eventName,
        OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        EVENT_KEY_PROCESS_NAME, processName,
        EVENT_KEY_MODULE_NAME, bundleName,
        EVENT_KEY_ABILITY_NAME, abilityName,
        EVENT_KEY_PAGE_URL, pageUrl,
        EVENT_KEY_VERSION_CODE, versionCode,
        EVENT_KEY_VERSION_NAME, versionName,
        EVENT_KEY_PAGE_NAME, pageName,
        EVENT_KEY_FILTER_TYPE, filterType,
        EVENT_KEY_SCENE_ID, sceneId,
        EVENT_KEY_REAL_SKIPPED_FRAME_TIME, static_cast<uint64_t>(realSkippedFrameTime),
        EVENT_KEY_SKIPPED_FRAME_TIME, static_cast<uint64_t>(skippedFrameTime));
    XPERF_TRACE_SCOPED("JANK_FRAME_UNFILTERED: skipppedFrameTime=%lld(ms), windowName=%s, filterType=%d",
        static_cast<long long>(skippedFrameTime / NS_TO_MS), windowName.c_str(), filterType);
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

}
}