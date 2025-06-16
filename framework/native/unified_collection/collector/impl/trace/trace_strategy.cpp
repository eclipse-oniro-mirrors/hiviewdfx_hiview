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
#include "trace_strategy.h"

#include <charconv>

#include "cjson_util.h"
#include "collect_event.h"
#include "event_publish.h"
#include "file_util.h"
#include "hiview_global.h"
#include "hiview_logger.h"
#include "hisysevent.h"
#include "time_util.h"
#include "trace_utils.h"

using namespace OHOS::HiviewDFX::Hitrace;
namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("UCollectUtil-TraceCollector");
constexpr uint32_t MB_TO_KB = 1024;
constexpr uint32_t KB_TO_BYTE = 1024;
constexpr int32_t FULL_TRACE_DURATION = -1;
const uint32_t MS_UNIT = 1000;

void InitDumpEvent(DumpEvent &dumpEvent, const std::string &caller, int maxDuration, uint64_t happenTime)
{
    dumpEvent.caller = caller;
    dumpEvent.reqDuration = maxDuration;
    dumpEvent.reqTime = happenTime;
    dumpEvent.execTime = TimeUtil::GenerateTimestamp() / MS_UNIT; // convert execTime into ms unit
}

void UpdateDumpEvent(DumpEvent &dumpEvent, const TraceRet &ret, uint64_t execDuration, const TraceRetInfo &retInfo)
{
    dumpEvent.errorCode = GetUcError(ret);
    dumpEvent.execDuration = execDuration;
    dumpEvent.coverDuration = retInfo.coverDuration;
    dumpEvent.coverRatio = retInfo.coverRatio;
    dumpEvent.traceMode = retInfo.mode;
    dumpEvent.tags = std::move(retInfo.tags);
}
}

TraceRet TraceStrategy::DumpTrace(DumpEvent &dumpEvent, TraceRetInfo &traceRetInfo) const
{
    InitDumpEvent(dumpEvent, caller_, maxDuration_, happenTime_);
    auto start = std::chrono::steady_clock::now();
    int maxDuration = (maxDuration_ == FULL_TRACE_DURATION) ? 0 : maxDuration_;
    TraceRet ret = TraceStateMachine::GetInstance().DumpTrace(scenario_, maxDuration, happenTime_, traceRetInfo);
    if (!ret.IsSuccess()) {
        HIVIEW_LOGW("scenario_:%{public}d, stateError:%{public}d, codeError:%{public}d", static_cast<int>(scenario_),
            static_cast<int>(ret.stateError_), ret.codeError_);
    }
    auto end = std::chrono::steady_clock::now();
    auto execDuration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    UpdateDumpEvent(dumpEvent, ret, execDuration, traceRetInfo);
    return ret;
}

TraceRet TraceDevStrategy::DoDump(std::vector<std::string> &outputFile)
{
    DumpEvent dumpEvent;
    dumpEvent.caller = caller_;
    TraceRetInfo traceRetInfo;
    TraceRet ret = DumpTrace(dumpEvent, traceRetInfo);
    if (!ret.IsSuccess()) {
        WriteDumpTraceHisysevent(dumpEvent);
        return ret;
    }
    if (traceRetInfo.outputFiles.empty()) {
        HIVIEW_LOGE("TraceDevStrategy outputFiles empty.");
        return ret;
    }
    int64_t traceSize = GetTraceSize(traceRetInfo);
    if (traceSize <= static_cast<int64_t>(INT32_MAX) * MB_TO_KB * KB_TO_BYTE) {
        dumpEvent.fileSize = traceSize / MB_TO_KB / KB_TO_BYTE;
    }
    WriteDumpTraceHisysevent(dumpEvent);
    if (scenario_== TraceScenario::TRACE_COMMAND) {
        outputFile = traceRetInfo.outputFiles;
        return ret;
    }
    outputFile = GetUnifiedSpecialFiles(traceRetInfo, caller_);
    return ret;
}

TraceRet TraceFlowControlStrategy::DoDump(std::vector<std::string> &outputFile)
{
    int64_t traceRemainingSize = flowController_->GetRemainingTraceSize();
    if (traceRemainingSize <= 0) {
        HIVIEW_LOGI("trace is over flow, can not dump.");
        return TraceRet(TraceFlowCode::TRACE_DUMP_DENY);
    }
    DumpEvent dumpEvent;
    dumpEvent.caller = caller_;
    TraceRetInfo traceRetInfo;
    TraceRet ret = DumpTrace(dumpEvent, traceRetInfo);
    if (!ret.IsSuccess()) {
        WriteDumpTraceHisysevent(dumpEvent);
        return ret;
    }
    if (traceRetInfo.outputFiles.empty()) {
        HIVIEW_LOGE("TraceFlowControlStrategy outputFiles empty.");
        return ret;
    }
    int64_t traceSize = GetTraceSize(traceRetInfo);
    if (traceSize <= static_cast<int64_t>(INT32_MAX) * MB_TO_KB * KB_TO_BYTE) {
        dumpEvent.fileSize = traceSize / MB_TO_KB / KB_TO_BYTE;
    }
    if (traceSize > traceRemainingSize) {
        dumpEvent.errorCode = TransFlowToUcError(TraceFlowCode::TRACE_UPLOAD_DENY);
        WriteDumpTraceHisysevent(dumpEvent);
        HIVIEW_LOGI("trace is over flow, can not upload.");
        return TraceRet(TraceFlowCode::TRACE_UPLOAD_DENY);
    }
    outputFile = GetUnifiedZipFiles(traceRetInfo, UNIFIED_SHARE_PATH);
    WriteDumpTraceHisysevent(dumpEvent);
    flowController_->StoreDb(traceSize);
    return {};
}

TraceRet TraceMixedStrategy::DoDump(std::vector<std::string> &outputFile)
{
    DumpEvent dumpEvent;
    dumpEvent.caller = caller_;
    TraceRetInfo traceRetInfo;

    // first dump trace in special dir then check flow to decide whether put trace in share dir
    TraceRet ret = DumpTrace(dumpEvent, traceRetInfo);
    if (!ret.IsSuccess()) {
        WriteDumpTraceHisysevent(dumpEvent);
        return ret;
    }
    if (traceRetInfo.outputFiles.empty()) {
        HIVIEW_LOGE("TraceMixedStrategy outputFiles empty.");
        return ret;
    }
    int64_t traceSize = GetTraceSize(traceRetInfo);
    if (traceSize <= static_cast<int64_t>(INT32_MAX) * MB_TO_KB * KB_TO_BYTE) {
        dumpEvent.fileSize = traceSize / MB_TO_KB / KB_TO_BYTE;
    }
    outputFile = GetUnifiedSpecialFiles(traceRetInfo, caller_);
    int64_t traceRemainingSize = flowController_->GetRemainingTraceSize();
    if (traceRemainingSize > 0) {
        if (traceRemainingSize < traceSize) {
            dumpEvent.errorCode = TransFlowToUcError(TraceFlowCode::TRACE_UPLOAD_DENY);
            WriteDumpTraceHisysevent(dumpEvent);
            HIVIEW_LOGI("over flow, trace generate in specil dir, can not upload.");
            return {};
        }
        outputFile = GetUnifiedZipFiles(traceRetInfo, UNIFIED_SHARE_PATH);
    } else {
        HIVIEW_LOGI("over flow, trace generate in specil dir, can not upload.");
        return {};
    }
    WriteDumpTraceHisysevent(dumpEvent);
    flowController_->StoreDb(traceSize);
    return {};
}

TraceRet TraceAsyncStrategy::DoDump(std::vector<std::string> &outputFile)
{
    DumpEvent dumpEvent;
    InitDumpEvent(dumpEvent, caller_, maxDuration_, happenTime_);
    auto start = std::chrono::steady_clock::now();
    DumpTraceArgs args = {
        .scenario = scenario_,
        .maxDuration = maxDuration_,
        .happenTime = happenTime_,
    };
    TraceRetInfo traceRetInfo;
    int64_t traceRemainingSize = flowController_->GetRemainingTraceSize();
    DumpTraceCallback callback = CreateDumpTraceCallback(caller_);
    TraceRet ret = TraceStateMachine::GetInstance().DumpTraceAsync(args, traceRemainingSize, traceRetInfo, callback);
    auto end = std::chrono::steady_clock::now();
    auto execDuration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    UpdateDumpEvent(dumpEvent, ret, execDuration, traceRetInfo);
    if (!ret.IsSuccess()) {
        WriteDumpTraceHisysevent(dumpEvent);
        for (const auto &file : traceRetInfo.outputFiles) {
            outputFile.emplace_back(GetTraceSpecialPath(file, caller_));
        }
        return ret;
    }
    if (traceRemainingSize >= traceRetInfo.fileSize) {
        flowController_->StoreDb(traceRetInfo.fileSize);
    }
    WriteDumpTraceHisysevent(dumpEvent);
    for (const auto &file : traceRetInfo.outputFiles) {
        outputFile.emplace_back(GetTraceZipFinalPath(file, UNIFIED_SHARE_PATH));
    }
    return {};
}

TraceRet TelemetryStrategy::DoDump(std::vector<std::string> &outputFile)
{
    TraceRetInfo traceRetInfo;
    auto ret = TraceStateMachine::GetInstance().DumpTraceWithFilter(maxDuration_, happenTime_, traceRetInfo);
    if (!ret.IsSuccess()) {
        HIVIEW_LOGE("fail stateError:%{public}d codeError:%{public}d", static_cast<int>(ret.GetStateError()),
            static_cast<int>(ret.GetCodeError()));
        return ret;
    }
    int64_t traceSize = GetTraceSize(traceRetInfo);
    if (auto fet = flowController_->NeedTelemetryDump(caller_, traceSize); fet != TelemetryRet::SUCCESS) {
        HIVIEW_LOGI("trace is over flow, can not dump.");
        return TraceRet(TraceFlowCode::TRACE_DUMP_DENY);
    }
    if (traceRetInfo.outputFiles.empty()) {
        HIVIEW_LOGW("TraceFlowControlStrategy outputFiles empty.");
        return TraceRet(traceRetInfo.errorCode);
    }
    outputFile = GetUnifiedZipFiles(traceRetInfo, UNIFIED_TELEMETRY_PATH);
    return {};
}

TraceRet TraceAppStrategy::DoDump(std::vector<std::string> &outputFile)
{
    TraceRetInfo traceRetInfo;
    auto appPid = TraceStateMachine::GetInstance().GetCurrentAppPid();
    if (appPid != appCallerEvent_->pid_) {
        HIVIEW_LOGW("pid check fail, maybe is app state closed, current:%{public}d, pid:%{public}d", appPid,
            appCallerEvent_->pid_);
        return TraceRet(TraceStateCode::FAIL);
    }
    if (flowController_->HasCallOnceToday(appCallerEvent_->uid_, appCallerEvent_->happenTime_)) {
        HIVIEW_LOGE("already capture trace uid=%{public}d pid==%{public}d", appCallerEvent_->uid_,
                    appCallerEvent_->pid_);
        return TraceRet(TraceFlowCode::TRACE_HAS_CAPTURED_TRACE);
    }
    TraceRet ret = TraceStateMachine::GetInstance().DumpTrace(scenario_, 0, 0, traceRetInfo);
    appCallerEvent_->taskBeginTime_ = static_cast<int64_t>(TraceStateMachine::GetInstance().GetTaskBeginTime());
    appCallerEvent_->taskEndTime_ = static_cast<int64_t>(TimeUtil::GetMilliseconds());
    if (ret.IsSuccess()) {
        outputFile = traceRetInfo.outputFiles;
        if (outputFile.empty() || outputFile[0].empty()) {
            HIVIEW_LOGE("TraceAppStrategy dump file empty");
            return ret;
        }
        std::string traceFileName = InnerMakeTraceFileName(appCallerEvent_);
        FileUtil::RenameFile(outputFile[0], traceFileName);
        appCallerEvent_->externalLog_ = traceFileName;
        flowController_->RecordCaller(appCallerEvent_);
    }
    InnerShareAppEvent(appCallerEvent_);
    CleanOldAppTrace();
    InnerReportMainThreadJankForTrace(appCallerEvent_);
    return ret;
}

void TraceAppStrategy::InnerReportMainThreadJankForTrace(std::shared_ptr<AppCallerEvent> appCallerEvent)
{
    HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, UCollectUtil::MAIN_THREAD_JANK, HiSysEvent::EventType::FAULT,
        UCollectUtil::SYS_EVENT_PARAM_BUNDLE_NAME, appCallerEvent->bundleName_,
        UCollectUtil::SYS_EVENT_PARAM_BUNDLE_VERSION, appCallerEvent->bundleVersion_,
        UCollectUtil::SYS_EVENT_PARAM_BEGIN_TIME, appCallerEvent->beginTime_,
        UCollectUtil::SYS_EVENT_PARAM_END_TIME, appCallerEvent->endTime_,
        UCollectUtil::SYS_EVENT_PARAM_THREAD_NAME, appCallerEvent->threadName_,
        UCollectUtil::SYS_EVENT_PARAM_FOREGROUND, appCallerEvent->foreground_,
        UCollectUtil::SYS_EVENT_PARAM_LOG_TIME, appCallerEvent->taskEndTime_,
        UCollectUtil::SYS_EVENT_PARAM_JANK_LEVEL, 1); // 1: over 450ms
}

void TraceAppStrategy::CleanOldAppTrace()
{
    DoClean(UNIFIED_SHARE_PATH, ClientName::APP);
    uint64_t timeNow = TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC;
    uint32_t secondsOfThreeDays = 3 * TimeUtil::SECONDS_PER_DAY; // 3 : clean data three days ago
    if (timeNow < secondsOfThreeDays) {
        HIVIEW_LOGW("time is invalid");
        return;
    }
    uint64_t timeThreeDaysAgo = timeNow - secondsOfThreeDays;
    std::string dateThreeDaysAgo = TimeUtil::TimestampFormatToDate(timeThreeDaysAgo, "%Y%m%d");
    int32_t dateNum = 0;
    auto result = std::from_chars(dateThreeDaysAgo.c_str(),
        dateThreeDaysAgo.c_str() + dateThreeDaysAgo.size(), dateNum);
    if (result.ec != std::errc()) {
        HIVIEW_LOGW("convert error, dateStr: %{public}s", dateThreeDaysAgo.c_str());
        return;
    }
    flowController_->CleanOldAppTrace(dateNum);
}

void TraceAppStrategy::InnerShareAppEvent(std::shared_ptr<AppCallerEvent> appCallerEvent)
{
    cJSON* eventJson = cJSON_CreateObject();
    if (eventJson == nullptr) {
        return;
    }
    cJSON_AddNumberToObject(eventJson, UCollectUtil::APP_EVENT_PARAM_UID, appCallerEvent->uid_);
    cJSON_AddNumberToObject(eventJson, UCollectUtil::APP_EVENT_PARAM_PID, appCallerEvent->pid_);
    cJSON_AddNumberToObject(eventJson, UCollectUtil::APP_EVENT_PARAM_TIME, appCallerEvent->happenTime_);
    cJSON_AddStringToObject(eventJson, UCollectUtil::APP_EVENT_PARAM_BUNDLE_NAME,
        appCallerEvent->bundleName_.c_str());
    cJSON_AddStringToObject(eventJson, UCollectUtil::APP_EVENT_PARAM_BUNDLE_VERSION,
        appCallerEvent->bundleVersion_.c_str());
    cJSON_AddNumberToObject(eventJson, UCollectUtil::APP_EVENT_PARAM_BEGIN_TIME, appCallerEvent->beginTime_);
    cJSON_AddNumberToObject(eventJson, UCollectUtil::APP_EVENT_PARAM_END_TIME, appCallerEvent->endTime_);
    cJSON_AddBoolToObject(eventJson, UCollectUtil::APP_EVENT_PARAM_ISBUSINESSJANK, appCallerEvent->isBusinessJank_);
    cJSON* externalLog = cJSON_CreateArray();
    cJSON* subLog = cJSON_CreateString(appCallerEvent->externalLog_.c_str());
    if (!cJSON_AddItemToArray(externalLog, subLog)) {
        cJSON_Delete(subLog);
    }
    cJSON_AddItemToObjectCS(eventJson, UCollectUtil::APP_EVENT_PARAM_EXTERNAL_LOG, externalLog);
    std::string param;
    CJsonUtil::BuildJsonString(eventJson, param);
    cJSON_Delete(eventJson);
    HIVIEW_LOGI("send for uid=%{public}d pid=%{public}d", appCallerEvent->uid_, appCallerEvent->pid_);
    EventPublish::GetInstance().PushEvent(appCallerEvent->uid_, UCollectUtil::MAIN_THREAD_JANK,
                                          HiSysEvent::EventType::FAULT, param);
}

std::string TraceAppStrategy::InnerMakeTraceFileName(std::shared_ptr<AppCallerEvent> appCallerEvent)
{
    std::string &bundleName = appCallerEvent->bundleName_;
    int32_t pid = appCallerEvent->pid_;
    int64_t beginTime = appCallerEvent->taskBeginTime_;
    int64_t endTime = appCallerEvent->taskEndTime_;
    int32_t costTime = (appCallerEvent->taskEndTime_ - appCallerEvent->taskBeginTime_);

    std::string d1 = TimeUtil::TimestampFormatToDate(beginTime/ TimeUtil::SEC_TO_MILLISEC, "%Y%m%d%H%M%S");
    std::string d2 = TimeUtil::TimestampFormatToDate(endTime/ TimeUtil::SEC_TO_MILLISEC, "%Y%m%d%H%M%S");

    std::string name;
    name.append(UNIFIED_SHARE_PATH).append("APP_").append(bundleName).append("_").append(std::to_string(pid));
    name.append("_").append(d1).append("_").append(d2).append("_").append(std::to_string(costTime)).append(".sys");
    return name;
}

std::shared_ptr<TraceStrategy> TraceFactory::CreateTraceStrategy(UCollect::TraceCaller caller, int32_t maxDuration,
    uint64_t happenTime)
{
    switch (caller) {
        case UCollect::TraceCaller::XPERF:
            return std::make_shared<TraceMixedStrategy>(maxDuration, happenTime, EnumToString(caller));
        case UCollect::TraceCaller::RELIABILITY:
            return std::make_shared<TraceAsyncStrategy>(maxDuration, happenTime, EnumToString(caller));
        case UCollect::TraceCaller::XPOWER:
        case UCollect::TraceCaller::HIVIEW:
            return std::make_shared<TraceFlowControlStrategy>(maxDuration, happenTime, EnumToString(caller));
        case UCollect::TraceCaller::OTHER:
        case UCollect::TraceCaller::SCREEN:
            return std::make_shared<TraceDevStrategy>(maxDuration, happenTime, EnumToString(caller),
                TraceScenario::TRACE_COMMON);
        default:
            return nullptr;
    }
}

std::shared_ptr<TraceStrategy> TraceFactory::CreateTraceStrategy(UCollect::TraceClient client, int32_t maxDuration,
    uint64_t happenTime)
{
    switch (client) {
        case UCollect::TraceClient::COMMAND:
            return std::make_shared<TraceDevStrategy>(maxDuration, happenTime, ClientToString(client),
                TraceScenario::TRACE_COMMAND);
        case UCollect::TraceClient::COMMON_DEV:
        case UCollect::TraceClient::BETACLUB:
            return std::make_shared<TraceDevStrategy>(maxDuration, happenTime, ClientToString(client),
                TraceScenario::TRACE_COMMON);
        default:
            return nullptr;
    }
}

std::shared_ptr<TraceAppStrategy> TraceFactory::CreateAppStrategy(std::shared_ptr<AppCallerEvent> appCallerEvent)
{
    return std::make_shared<TraceAppStrategy>(appCallerEvent);
}
}
}
