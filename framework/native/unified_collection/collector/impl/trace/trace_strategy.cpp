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
DEFINE_LOG_TAG("TraceStrategy");
constexpr uint32_t MB_TO_KB = 1024;
constexpr uint32_t KB_TO_BYTE = 1024;
const std::string UNIFIED_SHARE_PATH = "/data/log/hiview/unified_collection/trace/share/";
const std::string UNIFIED_SPECIAL_PATH = "/data/log/hiview/unified_collection/trace/special/";
const std::string UNIFIED_TELEMETRY_PATH = "/data/log/hiview/unified_collection/trace/telemetry/";
const std::string UNIFIED_SHARE_TEMP_PATH = UNIFIED_SHARE_PATH + "temp/";
const std::string TELEMETRY_STRATEGY = "TelemetryStrategy";
constexpr int32_t FULL_TRACE_DURATION = -1;
const uint32_t UNIFIED_SHARE_COUNTS = 25;
const uint32_t UNIFIED_TELEMETRY_COUNTS = 20;
const uint32_t UNIFIED_APP_SHARE_COUNTS = 40;
const uint32_t UNIFIED_SPECIAL_COUNTS = 3;
const uint32_t UNIFIED_SPECIAL_OTHER = 5;
const uint32_t MS_UNIT = 1000;
}

TraceStrategy::TraceStrategy(int32_t maxDuration, uint64_t happenTime, const std::string &caller,
    TraceScenario scenario)
    : maxDuration_(maxDuration), happenTime_(happenTime), caller_(caller), scenario_(scenario)
{
    CreateTracePath(UNIFIED_SHARE_PATH);
    CreateTracePath(UNIFIED_SPECIAL_PATH);
    CreateTracePath(UNIFIED_TELEMETRY_PATH);
}

TraceRet TraceStrategy::DumpTrace(DumpEvent &dumpEvent, TraceRetInfo &traceRetInfo) const
{
    TraceRet ret;
    dumpEvent.reqDuration = maxDuration_;
    dumpEvent.reqTime = happenTime_;
    dumpEvent.execTime = TimeUtil::GenerateTimestamp() / MS_UNIT; // convert execTime into ms unit
    auto start = std::chrono::steady_clock::now();
    if (maxDuration_ == FULL_TRACE_DURATION) {
        ret = TraceStateMachine::GetInstance().DumpTrace(scenario_, 0, happenTime_, traceRetInfo);
    } else {
        ret = TraceStateMachine::GetInstance().DumpTrace(scenario_, maxDuration_, happenTime_, traceRetInfo);
    }
    if (!ret.IsSuccess()) {
        HIVIEW_LOGW("scenario_:%{public}d, stateError:%{public}d, codeError:%{public}d", static_cast<int>(scenario_),
            static_cast<int>(ret.stateError_), ret.codeError_);
    }
    auto end = std::chrono::steady_clock::now();
    dumpEvent.execDuration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    dumpEvent.coverDuration = traceRetInfo.coverDuration;
    dumpEvent.coverRatio = traceRetInfo.coverRatio;
    dumpEvent.tags = std::move(traceRetInfo.tags);
    return ret;
}

void TraceStrategy::DoClean(const std::string &tracePath, uint32_t threshold, bool hasPrefix)
{
    // Load all files under the path
    std::vector<std::string> files;
    FileUtil::GetDirFiles(tracePath, files);

    // Filter files that belong to me
    std::deque<std::pair<uint64_t, std::string>> filesWithTimes;
    for (const auto &file : files) {
        if (!hasPrefix || IsMine(file)) {
            struct stat fileInfo;
            stat(file.c_str(), &fileInfo);
            filesWithTimes.emplace_back(fileInfo.st_mtime, file);
        }
    }
    std::sort(filesWithTimes.begin(), filesWithTimes.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    HIVIEW_LOGI("myFiles size : %{public}zu, MyThreshold : %{public}u.", filesWithTimes.size(), threshold);

    // Clean up old files, new copied file is still working in sub thread now, only can clean old files here
    while (filesWithTimes.size() >= threshold) {
        FileUtil::RemoveFile(filesWithTimes.front().second);
        HIVIEW_LOGI("remove file : %{public}s is deleted.", filesWithTimes.front().second.c_str());
        filesWithTimes.pop_front();
    }
}

TraceRet TraceDevStrategy::DoDump(std::vector<std::string> &outputFile)
{
    DumpEvent dumpEvent;
    dumpEvent.caller = caller_;
    TraceRetInfo traceRetInfo;
    TraceRet ret = DumpTrace(dumpEvent, traceRetInfo);
    if (!ret.IsSuccess()) {
        WriteDumpTraceHisysevent(dumpEvent, TransCodeToUcError(traceRetInfo.errorCode));
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
    WriteDumpTraceHisysevent(dumpEvent, UcError::SUCCESS);
    if (scenario_== TraceScenario::TRACE_COMMAND) {
        outputFile = traceRetInfo.outputFiles;
    } else {
        outputFile = GetUnifiedSpecialFiles(traceRetInfo.outputFiles, caller_);
        DoClean(UNIFIED_SPECIAL_PATH, UNIFIED_SPECIAL_OTHER, true);
    }
    return ret;
}

bool TraceDevStrategy::IsMine(const std::string &fileName)
{
    bool result = fileName.find(caller_) != std::string::npos;
    HIVIEW_LOGI("TraceDevStrategy caller_:%{public}s fileName:%{public}s result:%{public}d", caller_.c_str(),
        fileName.c_str(), result);
    return result;
}

TraceRet TraceFlowControlStrategy::DoDump(std::vector<std::string> &outputFile)
{
    if (!flowController_->NeedDump()) {
        HIVIEW_LOGI("trace is over flow, can not dump.");
        return TraceRet(TraceFlowCode::TRACE_DUMP_DENY);
    }
    DumpEvent dumpEvent;
    dumpEvent.caller = caller_;
    TraceRetInfo traceRetInfo;
    TraceRet ret = DumpTrace(dumpEvent, traceRetInfo);
    if (!ret.IsSuccess()) {
        WriteDumpTraceHisysevent(dumpEvent, TransCodeToUcError(traceRetInfo.errorCode));
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
    if (!flowController_->NeedUpload(traceSize)) {
        WriteDumpTraceHisysevent(dumpEvent, TransFlowToUcError(TraceFlowCode::TRACE_UPLOAD_DENY));
        HIVIEW_LOGI("trace is over flow, can not upload.");
        return TraceRet(TraceFlowCode::TRACE_UPLOAD_DENY);
    }
    outputFile = GetUnifiedZipFiles(traceRetInfo.outputFiles, UNIFIED_SHARE_PATH);
    DoClean(UNIFIED_SHARE_PATH, UNIFIED_SHARE_COUNTS, false);
    WriteDumpTraceHisysevent(dumpEvent, UcError::SUCCESS);
    flowController_->StoreDb();
    return {};
}

bool TraceFlowControlStrategy::IsMine(const std::string &fileName)
{
    return true;
}

TraceRet TraceMixedStrategy::DoDump(std::vector<std::string> &outputFile)
{
    DumpEvent dumpEvent;
    dumpEvent.caller = caller_;
    TraceRetInfo traceRetInfo;

    // first dump trace in special dir then check flow to decide whether put trace in share dir
    TraceRet ret = DumpTrace(dumpEvent, traceRetInfo);
    if (!ret.IsSuccess()) {
        WriteDumpTraceHisysevent(dumpEvent, GetUcError(ret));
        return ret;
    }
    if (traceRetInfo.outputFiles.empty()) {
        HIVIEW_LOGE("TraceMixedStrategy outputFiles empty.");
        return ret;
    }
    int64_t traceSize = GetTraceSize(traceRetInfo);
    outputFile = GetUnifiedSpecialFiles(traceRetInfo.outputFiles, caller_);
    DoClean(UNIFIED_SPECIAL_PATH, UNIFIED_SPECIAL_COUNTS, true);
    if (flowController_->NeedDump()) {
        if (!flowController_->NeedUpload(traceSize)) {
            WriteDumpTraceHisysevent(dumpEvent, TransFlowToUcError(TraceFlowCode::TRACE_UPLOAD_DENY));
            HIVIEW_LOGI("over flow, trace generate in specil dir, can not upload.");
            return {};
        }
        outputFile = GetUnifiedZipFiles(traceRetInfo.outputFiles, UNIFIED_SHARE_PATH);
        DoClean(UNIFIED_SHARE_PATH, UNIFIED_SHARE_COUNTS, false);
    } else {
        HIVIEW_LOGI("over flow, trace generate in specil dir, can not upload.");
        return {};
    }
    WriteDumpTraceHisysevent(dumpEvent, UcError::SUCCESS);
    flowController_->StoreDb();
    return {};
}

bool TraceMixedStrategy::IsMine(const std::string &fileName)
{
    return fileName.find(caller_) != std::string::npos;
}

TraceRet TelemetryStrategy::DoDump(std::vector<std::string> &outputFile)
{
    TraceRetInfo traceRetInfo;
    auto ret = TraceStateMachine::GetInstance().DumpTraceWithFilter(pidList_, maxDuration_, happenTime_, traceRetInfo);
    if (!ret.IsSuccess()) {
        HIVIEW_LOGE("fail stateError:%{public}d codeError:%{public}d", static_cast<int>(ret.GetStateError()),
            static_cast<int>(ret.GetCodeError()));
        return ret;
    }
    int64_t traceSize = GetTraceSize(traceRetInfo);
    auto event = std::make_shared<Event>(TELEMETRY_STRATEGY);
    event->eventName_ = TelemetryEvent::TELEMETRY_STOP;
    switch (flowController_->NeedTelemetryDump(caller_, traceSize)) {
        case TelemetryFlow::EXIT:
            if (HiviewGlobal::GetInstance() != nullptr &&
                HiviewGlobal::GetInstance()->PostSyncEventToTarget(UCollectUtil::UCOLLECTOR_PLUGIN, event)) {
                HIVIEW_LOGD("PostSyncEventToTarget exit message to UnifiedCollector");
            }
            return TraceRet(TraceFlowCode::TRACE_DUMP_DENY);
        case TelemetryFlow::OVER_FLOW:
            HIVIEW_LOGI("trace is over flow, can not dump.");
            return TraceRet(TraceFlowCode::TRACE_DUMP_DENY);
        default:
            break;
    }
    if (traceRetInfo.outputFiles.empty()) {
        HIVIEW_LOGW("TraceFlowControlStrategy outputFiles empty.");
        return TraceRet(traceRetInfo.errorCode);
    }
    outputFile = GetUnifiedZipFiles(traceRetInfo.outputFiles, UNIFIED_TELEMETRY_PATH);
    DoClean(UNIFIED_TELEMETRY_PATH, UNIFIED_TELEMETRY_COUNTS, false);
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
    appCallerEvent_->taskBeginTime_ = TraceStateMachine::GetInstance().GetTaskBeginTime();
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

bool TraceAppStrategy::IsMine(const std::string &fileName)
{
    if (fileName.find("/" + ClientName::APP) != std::string::npos) {
        return true;
    }
    return false;
}

void TraceAppStrategy::CleanOldAppTrace()
{
    DoClean(UNIFIED_SHARE_PATH, UNIFIED_APP_SHARE_COUNTS, true);
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
    Json::Value eventJson;
    eventJson[UCollectUtil::APP_EVENT_PARAM_UID] = appCallerEvent->uid_;
    eventJson[UCollectUtil::APP_EVENT_PARAM_PID] = appCallerEvent->pid_;
    eventJson[UCollectUtil::APP_EVENT_PARAM_TIME] = appCallerEvent->happenTime_;
    eventJson[UCollectUtil::APP_EVENT_PARAM_BUNDLE_NAME] = appCallerEvent->bundleName_;
    eventJson[UCollectUtil::APP_EVENT_PARAM_BUNDLE_VERSION] = appCallerEvent->bundleVersion_;
    eventJson[UCollectUtil::APP_EVENT_PARAM_BEGIN_TIME] = appCallerEvent->beginTime_;
    eventJson[UCollectUtil::APP_EVENT_PARAM_END_TIME] = appCallerEvent->endTime_;
    eventJson[UCollectUtil::APP_EVENT_PARAM_ISBUSINESSJANK] = appCallerEvent->isBusinessJank_;
    Json::Value externalLog;
    externalLog.append(appCallerEvent->externalLog_);
    eventJson[UCollectUtil::APP_EVENT_PARAM_EXTERNAL_LOG] = externalLog;
    std::string param = Json::FastWriter().write(eventJson);

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
        case UCollect::TraceCaller::RELIABILITY:
            return std::make_shared<TraceMixedStrategy>(maxDuration, happenTime, EnumToString(caller));
        case UCollect::TraceCaller::XPOWER:
        case UCollect::TraceCaller::HIVIEW:
        case UCollect::TraceCaller::FOUNDATION:
            return std::make_shared<TraceFlowControlStrategy>(maxDuration, happenTime, EnumToString(caller));
        case UCollect::TraceCaller::OTHER:
        case UCollect::TraceCaller::BETACLUB:
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
