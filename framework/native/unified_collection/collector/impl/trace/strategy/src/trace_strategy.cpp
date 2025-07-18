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

#include "event_publish.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "time_util.h"
#include "trace_utils.h"
#include "trace_worker.h"
#include "trace_state_machine.h"
#include "collect_event.h"
#include "json/json.h"
#include "memory_collector.h"

using namespace OHOS::HiviewDFX::Hitrace;
namespace OHOS::HiviewDFX {
namespace {
DEFINE_LOG_TAG("UCollectUtil-TraceCollector");
constexpr int32_t FULL_TRACE_DURATION = -1;
const uint32_t MB_TO_KB = 1024;
const uint32_t KB_TO_BYTE = 1024;
const uint32_t MS_UNIT = 1000;

void InitDumpEvent(DumpEvent &dumpEvent, const std::string &caller, uint32_t maxDuration, uint64_t happenTime)
{
    dumpEvent.caller = caller;
    dumpEvent.reqDuration = maxDuration;
    dumpEvent.reqTime = happenTime;
    dumpEvent.execTime = TimeUtil::GenerateTimestamp() / MS_UNIT; // convert execTime into ms unit
}

void UpdateDumpEvent(DumpEvent &dumpEvent, const TraceRet &ret, int64_t execDuration,
    const TraceRetInfo &retInfo)
{
    dumpEvent.errorCode = GetUcError(ret);
    dumpEvent.execDuration = execDuration;
    dumpEvent.coverDuration = retInfo.coverDuration;
    dumpEvent.coverRatio = retInfo.coverRatio;
    dumpEvent.traceMode = retInfo.mode;
    dumpEvent.tags = std::move(retInfo.tags);
}

void LoadMemoryInfo(DumpEvent &dumpEvent)
{
    std::shared_ptr<UCollectUtil::MemoryCollector> collector = UCollectUtil::MemoryCollector::Create();
    CollectResult<SysMemory> data = collector->CollectSysMemory();
    dumpEvent.sysMemTotal = data.data.memTotal / MB_TO_KB;
    dumpEvent.sysMemFree = data.data.memFree / MB_TO_KB;
    dumpEvent.sysMemAvail = data.data.memAvailable / MB_TO_KB;
}

void WriteDumpTraceHisysevent(DumpEvent &dumpEvent)
{
    LoadMemoryInfo(dumpEvent);
    int ret = HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::PROFILER, "DUMP_TRACE",
        OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        "CALLER", dumpEvent.caller,
        "ERROR_CODE", dumpEvent.errorCode,
        "IPC_TIME", dumpEvent.ipcTime,
        "REQ_TIME", dumpEvent.reqTime,
        "REQ_DURATION", dumpEvent.reqDuration,
        "EXEC_TIME", dumpEvent.execTime,
        "EXEC_DURATION", dumpEvent.execDuration,
        "COVER_DURATION", dumpEvent.coverDuration,
        "COVER_RATIO", dumpEvent.coverRatio,
        "TAGS", dumpEvent.tags,
        "FILE_SIZE", dumpEvent.fileSize,
        "SYS_MEM_TOTAL", dumpEvent.sysMemTotal,
        "SYS_MEM_FREE", dumpEvent.sysMemFree,
        "SYS_MEM_AVAIL", dumpEvent.sysMemAvail,
        "SYS_CPU", dumpEvent.sysCpu,
        "TRACE_MODE", dumpEvent.traceMode);
    if (ret != 0) {
        HIVIEW_LOGE("HiSysEventWrite failed, ret is %{public}d", ret);
    }
}
}

TraceRet TraceStrategy::DumpTrace(DumpEvent &dumpEvent, TraceRetInfo &traceRetInfo) const
{
    InitDumpEvent(dumpEvent, caller_, maxDuration_, happenTime_);
    auto start = std::chrono::steady_clock::now();
    uint32_t maxDuration = (maxDuration_ == FULL_TRACE_DURATION) ? 0 : maxDuration_;
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

TraceRet TraceDevStrategy::DoDump(std::vector<std::string> &outputFiles)
{
    DumpEvent dumpEvent;
    dumpEvent.caller = caller_;
    TraceRetInfo traceRetInfo;
    TraceRet ret = DumpTrace(dumpEvent, traceRetInfo);
    if (!ret.IsSuccess()) {
        WriteDumpTraceHisysevent(dumpEvent);
        return TraceRet(TraceFlowCode::TRACE_DUMP_DENY);
    }
    if (traceRetInfo.outputFiles.empty()) {
        HIVIEW_LOGE("TraceDevStrategy outputFiles empty.");
        return ret;
    }
    const int64_t traceSize = traceRetInfo.fileSize;
    if (traceSize <= static_cast<int64_t>(INT32_MAX) * MB_TO_KB * KB_TO_BYTE) {
        dumpEvent.fileSize = traceSize / MB_TO_KB / KB_TO_BYTE;
    }
    if (traceHandler_ != nullptr) {
        outputFiles = traceHandler_->HandleTrace(traceRetInfo.outputFiles);
    }
    if (zipHandler_ == nullptr) {
        return ret;
    }
    int64_t traceRemainingSize = traceFlowController_->GetRemainingTraceSize();
    if (traceRemainingSize <= traceSize) {
        dumpEvent.errorCode = TransFlowToUcError(TraceFlowCode::TRACE_UPLOAD_DENY);
        WriteDumpTraceHisysevent(dumpEvent);
        HIVIEW_LOGI("over flow, trace generate in special dir, can not upload.");
        return TraceRet(TraceFlowCode::TRACE_UPLOAD_DENY);
    }
    outputFiles = zipHandler_->HandleTrace(traceRetInfo.outputFiles);
    WriteDumpTraceHisysevent(dumpEvent);
    traceFlowController_->StoreDb(traceSize);
    return ret;
}

TraceRet TraceFlowControlStrategy::DoDump(std::vector<std::string> &outputFiles)
{
    int64_t traceRemainingSize = traceFlowController_->GetRemainingTraceSize();
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
    const int64_t traceSize = traceRetInfo.fileSize;
    if (traceSize <= static_cast<int64_t>(INT32_MAX) * MB_TO_KB * KB_TO_BYTE) {
        dumpEvent.fileSize = traceSize / MB_TO_KB / KB_TO_BYTE;
    }
    if (traceSize > traceRemainingSize) {
        dumpEvent.errorCode = TransFlowToUcError(TraceFlowCode::TRACE_UPLOAD_DENY);
        WriteDumpTraceHisysevent(dumpEvent);
        HIVIEW_LOGI("trace is over flow, can not upload.");
        return TraceRet(TraceFlowCode::TRACE_UPLOAD_DENY);
    }
    if (traceHandler_ != nullptr) {
        outputFiles = traceHandler_->HandleTrace(traceRetInfo.outputFiles);
    }
    WriteDumpTraceHisysevent(dumpEvent);
    traceFlowController_->StoreDb(traceSize);
    return ret;
}

TraceRet TelemetryStrategy::DoDump(std::vector<std::string> &outputFiles)
{
    TraceRetInfo traceRetInfo;
    if (auto ret = TraceStateMachine::GetInstance().DumpTraceWithFilter(maxDuration_, happenTime_, traceRetInfo);
        !ret.IsSuccess()) {
        HIVIEW_LOGE("fail stateError:%{public}d codeError:%{public}d", static_cast<int>(ret.GetStateError()),
            static_cast<int>(ret.GetCodeError()));
        return ret;
    }
    const int64_t traceSize = traceRetInfo.fileSize;
    if (auto flowRet = traceFlowController_->NeedTelemetryDump(caller_, traceSize); flowRet != TelemetryRet::SUCCESS) {
        HIVIEW_LOGI("trace is over flow, can not dump.");
        return TraceRet(TraceFlowCode::TRACE_DUMP_DENY);
    }
    if (traceRetInfo.outputFiles.empty()) {
        HIVIEW_LOGW("TraceFlowControlStrategy outputFiles empty.");
        return TraceRet(traceRetInfo.errorCode);
    }
    if (traceHandler_ != nullptr) {
        outputFiles = traceHandler_->HandleTrace(traceRetInfo.outputFiles);
    }
    return {};
}

TraceRet TraceAsyncStrategy::DoDump(std::vector<std::string> &outputFiles)
{
    int64_t traceRemainingSize = traceFlowController_->GetRemainingTraceSize();
    if (traceRemainingSize <= 0) {
        HIVIEW_LOGI("trace is over flow, can not dump.");
        return TraceRet(TraceFlowCode::TRACE_DUMP_DENY);
    }
    DumpEvent dumpEvent;
    InitDumpEvent(dumpEvent, caller_, maxDuration_, happenTime_);
    auto start = std::chrono::steady_clock::now();
    DumpTraceArgs args = {scenario_, maxDuration_, happenTime_};
    TraceRetInfo traceRetInfo;
    TraceRet ret = TraceStateMachine::GetInstance().DumpTraceAsync(args, traceRemainingSize, traceRetInfo,
        [strategy = shared_from_this()](TraceRetInfo asyncTraceRetInfo) {
            if (asyncTraceRetInfo.errorCode != TraceErrorCode::SUCCESS || asyncTraceRetInfo.outputFiles.empty()) {
                HIVIEW_LOGW("caller %{public}s callback not implement or trace file empty", strategy->caller_.c_str());
                return;
            }
            if (strategy->traceHandler_ != nullptr) {
                strategy->traceHandler_->HandleTrace(asyncTraceRetInfo.outputFiles);
            }
            if (strategy->zipHandler_ != nullptr && !asyncTraceRetInfo.isOverflowControl) {
                strategy->zipHandler_ ->HandleTrace(asyncTraceRetInfo.outputFiles);
            }
    });
    auto end = std::chrono::steady_clock::now();
    auto execDuration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    UpdateDumpEvent(dumpEvent, ret, execDuration, traceRetInfo);
    HIVIEW_LOGI("caller:%{public}s trace size:%{public}" PRId64 "", caller_.c_str(), traceRetInfo.fileSize);
    if (!ret.IsSuccess()) {
        WriteDumpTraceHisysevent(dumpEvent);
        return ret;
    }
    const int64_t traceSize = traceRetInfo.fileSize;
    if (traceSize <= static_cast<int64_t>(INT32_MAX) * MB_TO_KB * KB_TO_BYTE) {
        dumpEvent.fileSize = traceSize / MB_TO_KB / KB_TO_BYTE;
    }
    if (traceRetInfo.isOverflowControl) {
        SetResultCopyFiles(outputFiles, traceRetInfo.outputFiles);
        dumpEvent.errorCode = TransFlowToUcError(TraceFlowCode::TRACE_UPLOAD_DENY);
        WriteDumpTraceHisysevent(dumpEvent);
        HIVIEW_LOGI("over flow, trace generate in special dir, can not upload.");
        return TraceRet(TraceFlowCode::TRACE_UPLOAD_DENY);
    }
    SetResultZipFiles(outputFiles, traceRetInfo.outputFiles);
    WriteDumpTraceHisysevent(dumpEvent);
    traceFlowController_->StoreDb(traceSize);
    return ret;
}

TraceRet TraceAppStrategy::DoDump(std::vector<std::string> &outputFiles)
{
    TraceRetInfo traceRetInfo;
    auto appPid = TraceStateMachine::GetInstance().GetCurrentAppPid();
    if (appPid != appCallerEvent_->pid_) {
        HIVIEW_LOGW("pid check fail, maybe is app state closed, current:%{public}d, pid:%{public}d", appPid,
                    appCallerEvent_->pid_);
        return TraceRet(TraceStateCode::FAIL);
    }
    if (traceFlowController_->HasCallOnceToday(appCallerEvent_->uid_, appCallerEvent_->happenTime_)) {
        HIVIEW_LOGE("already capture trace uid=%{public}d pid==%{public}d", appCallerEvent_->uid_,
                    appCallerEvent_->pid_);
        return TraceRet(TraceFlowCode::TRACE_HAS_CAPTURED_TRACE);
    }
    TraceRet ret = TraceStateMachine::GetInstance().DumpTrace(scenario_, 0, 0, traceRetInfo);
    appCallerEvent_->taskBeginTime_ = static_cast<int64_t>(TraceStateMachine::GetInstance().GetTaskBeginTime());
    appCallerEvent_->taskEndTime_ = static_cast<int64_t>(TimeUtil::GetMilliseconds());
    if (ret.IsSuccess()) {
        outputFiles = traceRetInfo.outputFiles;
        if (outputFiles.empty() || outputFiles[0].empty()) {
            HIVIEW_LOGE("TraceAppStrategy dump file empty");
            return ret;
        }
        std::string traceFileName = InnerMakeTraceFileName(appCallerEvent_);
        FileUtil::RenameFile(outputFiles[0], traceFileName);
        if (traceHandler_ != nullptr) {
            traceHandler_->HandleTrace(traceRetInfo.outputFiles);
        }
        appCallerEvent_->externalLog_ = traceFileName;
        traceFlowController_->RecordCaller(appCallerEvent_);
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
    uint64_t timeNow = TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC;
    uint32_t secondsOfThreeDays = 3 * TimeUtil::SECONDS_PER_DAY; // 3 : clean data three days ago
    if (timeNow < secondsOfThreeDays) {
        HIVIEW_LOGW("time is invalid");
        return;
    }
    uint64_t timeThreeDaysAgo = timeNow - secondsOfThreeDays;
    std::string dateThreeDaysAgo = TimeUtil::TimestampFormatToDate(timeThreeDaysAgo, "%Y%m%d");
    int32_t dateNum = 0;
    auto result = std::from_chars(dateThreeDaysAgo.c_str(), dateThreeDaysAgo.c_str() + dateThreeDaysAgo.size(),
        dateNum);
    if (result.ec != std::errc()) {
        HIVIEW_LOGW("convert error, dateStr: %{public}s", dateThreeDaysAgo.c_str());
        return;
    }
    traceFlowController_->CleanOldAppTrace(dateNum);
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
}
