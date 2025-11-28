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
#include "hiview_logger.h"
#include "time_util.h"
#include "trace_utils.h"
#include "collect_event.h"
#include "json/json.h"
#include "memory_collector.h"
#include "parameter_ex.h"
#ifndef TRACE_STRATEGY_UNITTEST
#include "trace_state_machine.h"
#else
#include "test_trace_state_machine.h"
#endif

namespace OHOS::HiviewDFX {
namespace {
DEFINE_LOG_TAG("UCollectUtil-TraceCollector");
const uint32_t BYTE_UNIT = 1024;
const uint32_t MS_UNIT = 1000;

void InitDumpEvent(DumpEvent &dumpEvent, const std::string &caller, uint32_t maxDuration, uint64_t happenTime)
{
    dumpEvent.caller = caller;
    dumpEvent.reqDuration = static_cast<int32_t>(maxDuration);
    dumpEvent.reqTime = happenTime;
    dumpEvent.execTime = TimeUtil::GenerateTimestamp() / MS_UNIT; // convert execTime into ms unit
    dumpEvent.startTime = TimeUtil::GetSteadyClockTimeMs();
}

void UpdateDumpEvent(DumpEvent &dumpEvent, const TraceRet &ret, const TraceRetInfo &retInfo)
{
    auto endTime = TimeUtil::GetSteadyClockTimeMs();
    dumpEvent.errorCode = GetUcError(ret);
    dumpEvent.execDuration = static_cast<int32_t>(endTime - dumpEvent.startTime);
    dumpEvent.coverDuration = retInfo.coverDuration;
    dumpEvent.coverRatio = retInfo.coverRatio;
    dumpEvent.traceMode = retInfo.mode;
    dumpEvent.tags = retInfo.tags;
}

void LoadMemoryInfo(DumpEvent &dumpEvent)
{
    std::shared_ptr<UCollectUtil::MemoryCollector> collector = UCollectUtil::MemoryCollector::Create();
    CollectResult<SysMemory> data = collector->CollectSysMemory();
    dumpEvent.sysMemTotal = data.data.memTotal / BYTE_UNIT;
    dumpEvent.sysMemFree = data.data.memFree / BYTE_UNIT;
    dumpEvent.sysMemAvail = data.data.memAvailable / BYTE_UNIT;
}

void WriteDumpTraceHisysevent(DumpEvent &dumpEvent)
{
#ifndef TRACE_STRATEGY_UNITTEST
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
#endif
}
}

TraceRet TraceStrategy::DoDump(std::vector<std::string> &outputFiles, TraceRetInfo &traceRetInfo)
{
    DumpEvent dumpEvent;
#ifndef TRACE_STRATEGY_UNITTEST
    TraceRet ret = DumpTrace(dumpEvent, traceRetInfo, scenario_);
#else
    TraceRet ret(traceRetInfo.errorCode);
#endif
    if (traceHandler_ == nullptr) {
        // trace command scenario will return original trace file
        outputFiles = traceRetInfo.outputFiles;
        return ret;
    }

    // handle other trace without flow control
    outputFiles = traceHandler_->HandleTrace(traceRetInfo.outputFiles);
    WriteDumpTraceHisysevent(dumpEvent);
    return ret;
}

TraceRet TraceStrategy::DumpTrace(DumpEvent &dumpEvent, TraceRetInfo &traceRetInfo, TraceScenario scenario) const
{
    InitDumpEvent(dumpEvent, caller_, maxDuration_, happenTime_);
#ifndef TRACE_STRATEGY_UNITTEST
    TraceRet ret = TraceStateMachine::GetInstance().DumpTrace(scenario, maxDuration_, happenTime_, traceRetInfo);
#else
    TraceRet ret(traceRetInfo.errorCode);
#endif
    if (!ret.IsSuccess()) {
        HIVIEW_LOGW("scenario_:%{public}d, stateError:%{public}d, codeError:%{public}d", static_cast<int>(scenario),
            static_cast<int>(ret.stateError_), ret.codeError_);
    }
    UpdateDumpEvent(dumpEvent, ret, traceRetInfo);
    return ret;
}

TraceRet TraceDevStrategy::DoDump(std::vector<std::string> &outputFiles, TraceRetInfo &traceRetInfo)
{
    if (traceFlowController_->IsIoOverFlow()) {
        HIVIEW_LOGI("io over flow, can not dump.");
        return TraceRet(TraceFlowCode::TRACE_DUMP_DENY);
    }
    DumpEvent dumpEvent;
    TraceRet ret = DumpTrace(dumpEvent, traceRetInfo, TraceScenario::TRACE_COMMON);
    if (!ret.IsSuccess()) {
        WriteDumpTraceHisysevent(dumpEvent);
        return ret;
    }
    if (traceRetInfo.outputFiles.empty()) {
        HIVIEW_LOGE("TraceDevStrategy outputFiles empty.");
        return TraceRet(TraceStateCode::FAIL);
    }
    const int64_t traceSize = traceRetInfo.fileSize;
    if (traceSize <= static_cast<int64_t>(INT32_MAX) * BYTE_UNIT * BYTE_UNIT) {
        dumpEvent.fileSize = traceSize / BYTE_UNIT / BYTE_UNIT;
    }
    if (traceHandler_ == nullptr) {
        HIVIEW_LOGE("traceHandler is nullptr.");
        outputFiles = traceRetInfo.outputFiles;
        return TraceRet(TraceStateCode::FAIL);
    }
    outputFiles = traceHandler_->HandleTrace(traceRetInfo.outputFiles);
    if (zipHandler_ == nullptr) {
        traceFlowController_->StoreIoSize(traceSize);
        WriteDumpTraceHisysevent(dumpEvent);
        return ret;
    }
    int64_t traceRemainingSize = traceFlowController_->GetRemainingTraceSize();
    if (traceRemainingSize <= traceSize) {
        dumpEvent.errorCode = TransFlowToUcError(TraceFlowCode::TRACE_UPLOAD_DENY);
        WriteDumpTraceHisysevent(dumpEvent);
        traceFlowController_->StoreIoSize(traceSize);
        HIVIEW_LOGI("over flow, trace generate in special dir, can not upload.");
        return TraceRet(TraceFlowCode::TRACE_UPLOAD_DENY);
    }
    outputFiles = zipHandler_->HandleTrace(traceRetInfo.outputFiles);
    WriteDumpTraceHisysevent(dumpEvent);
    traceFlowController_->StoreTraceSize(traceSize);
    return ret;
}

TraceRet TraceFlowControlStrategy::DoDump(std::vector<std::string> &outputFiles, TraceRetInfo &traceRetInfo)
{
    if (traceHandler_ == nullptr) {
        HIVIEW_LOGE("traceHandler is null");
        return TraceRet(TraceStateCode::FAIL);
    }
    if (traceFlowController_->IsZipOverFlow()) {
        HIVIEW_LOGI("trace is over flow, can not dump.");
        return TraceRet(TraceFlowCode::TRACE_DUMP_DENY);
    }
    DumpEvent dumpEvent;
    TraceRet ret = DumpTrace(dumpEvent, traceRetInfo, TraceScenario::TRACE_COMMON);
    if (!ret.IsSuccess()) {
        WriteDumpTraceHisysevent(dumpEvent);
        return ret;
    }
    if (traceRetInfo.outputFiles.empty()) {
        HIVIEW_LOGE("TraceFlowControlStrategy outputFiles empty.");
        return TraceRet(TraceStateCode::FAIL);
    }
    const int64_t traceSize = traceRetInfo.fileSize;
    if (traceSize <= static_cast<int64_t>(INT32_MAX) * BYTE_UNIT * BYTE_UNIT) {
        dumpEvent.fileSize = traceSize / BYTE_UNIT / BYTE_UNIT;
    }
    if (traceSize > traceFlowController_->GetRemainingTraceSize()) {
        dumpEvent.errorCode = TransFlowToUcError(TraceFlowCode::TRACE_UPLOAD_DENY);
        WriteDumpTraceHisysevent(dumpEvent);
        traceFlowController_->DecreaseDynamicThreshold();
        HIVIEW_LOGI("trace is over flow, can not upload.");
        return TraceRet(TraceFlowCode::TRACE_UPLOAD_DENY);
    }
    outputFiles = traceHandler_->HandleTrace(traceRetInfo.outputFiles);
    WriteDumpTraceHisysevent(dumpEvent);
    traceFlowController_->StoreTraceSize(traceSize);
    return ret;
}

TraceRet TelemetryStrategy::DoDump(std::vector<std::string> &outputFiles, TraceRetInfo &traceRetInfo)
{
    if (traceHandler_ == nullptr) {
        HIVIEW_LOGW("traceHandler is null");
        return TraceRet(TraceStateCode::FAIL);
    }
#ifndef TRACE_STRATEGY_UNITTEST
    if (auto ret = TraceStateMachine::GetInstance().DumpTraceWithFilter(maxDuration_, happenTime_, traceRetInfo);
#else
    if (TraceRet ret(traceRetInfo.errorCode);
#endif
        !ret.IsSuccess()) {
        HIVIEW_LOGE("fail stateError:%{public}d codeError:%{public}d", static_cast<int>(ret.GetStateError()),
            static_cast<int>(ret.GetCodeError()));
        return ret;
    }
    if (auto flowRet = traceFlowController_->NeedTelemetryDump(caller_); flowRet != TelemetryRet::SUCCESS) {
        HIVIEW_LOGI("trace is over flow, can not dump.");
        return TraceRet(TraceFlowCode::TRACE_DUMP_DENY);
    }
    if (traceRetInfo.outputFiles.empty()) {
        HIVIEW_LOGW("TraceFlowControlStrategy outputFiles empty.");
        return TraceRet(TraceStateCode::FAIL);
    }
    outputFiles = traceHandler_->HandleTrace(traceRetInfo.outputFiles,
        [strategy = shared_from_this()](int64_t zipTraffic) {
        strategy->traceFlowController_->TelemetryStore(strategy->caller_, zipTraffic);
        HIVIEW_LOGI("storage zipTraffic:%{public}" PRId64 "", zipTraffic);
    });
    return {};
}

TraceRet TraceAsyncStrategy::DoDump(std::vector<std::string> &outputFiles, TraceRetInfo &traceRetInfo)
{
    if (traceFlowController_->IsIoOverFlow()) {
        HIVIEW_LOGI("io over flow, can not dump.");
        return TraceRet(TraceFlowCode::TRACE_DUMP_DENY);
    }
    DumpEvent dumpEvent;
    TraceRet ret = DumpTrace(dumpEvent, traceRetInfo, TraceScenario::TRACE_COMMON);
    HIVIEW_LOGI("caller:%{public}s trace size:%{public}" PRId64 "", caller_.c_str(), traceRetInfo.fileSize);
    if (!ret.IsSuccess()) {
        WriteDumpTraceHisysevent(dumpEvent);
        return ret;
    }
    if (traceRetInfo.outputFiles.empty()) {
        HIVIEW_LOGE("TraceAsyncStrategy outputFiles empty.");
        return TraceRet(TraceStateCode::FAIL);
    }
    const int64_t traceSize = traceRetInfo.fileSize;
    if (traceSize <= static_cast<int64_t>(INT32_MAX) * BYTE_UNIT * BYTE_UNIT) {
        dumpEvent.fileSize = traceSize / BYTE_UNIT / BYTE_UNIT;
    }
    if (traceHandler_ == nullptr) {
        outputFiles = traceRetInfo.outputFiles;
        return ret;
    }
    SetResultCopyFiles(outputFiles, traceRetInfo.outputFiles);
    if (zipHandler_ == nullptr) {
        traceFlowController_->StoreIoSize(traceSize);
        WriteDumpTraceHisysevent(dumpEvent);
        return ret;
    }
    if (traceRetInfo.isOverflowControl) {
        dumpEvent.errorCode = TransFlowToUcError(TraceFlowCode::TRACE_UPLOAD_DENY);
        WriteDumpTraceHisysevent(dumpEvent);
        HIVIEW_LOGI("over flow, trace generate in special dir, can not upload.");
        traceFlowController_->StoreIoSize(traceSize);
        return TraceRet(TraceFlowCode::TRACE_UPLOAD_DENY);
    }
    SetResultZipFiles(outputFiles, traceRetInfo.outputFiles);
    WriteDumpTraceHisysevent(dumpEvent);
    traceFlowController_->StoreTraceSize(traceSize);
    return ret;
}

TraceRet TraceAsyncStrategy::DumpTrace(DumpEvent &dumpEvent, TraceRetInfo &traceRetInfo, TraceScenario scenario) const
{
    InitDumpEvent(dumpEvent, caller_, maxDuration_, happenTime_);
    DumpTraceArgs args = {scenario, maxDuration_, happenTime_};
    int64_t traceRemainingSize = traceFlowController_->GetRemainingTraceSize();
#ifndef TRACE_STRATEGY_UNITTEST
    TraceRet ret = TraceStateMachine::GetInstance().DumpTraceAsync(args, traceRemainingSize, traceRetInfo,
#else
    TraceRet ret = MockTraceStateMachine::GetInstance().DumpTraceAsync(args, traceRemainingSize, traceRetInfo,
#endif
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
    UpdateDumpEvent(dumpEvent, ret, traceRetInfo);
    return ret;
}

TraceRet TraceAppStrategy::DoDump(std::vector<std::string> &outputFiles, TraceRetInfo &traceRetInfo)
{
#ifndef TRACE_STRATEGY_UNITTEST
    auto appPid = TraceStateMachine::GetInstance().GetCurrentAppPid();
#else
    auto appPid = MockTraceStateMachine::GetInstance().GetCurrentAppPid();
#endif
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
#ifndef TRACE_STRATEGY_UNITTEST
    TraceRet ret = TraceStateMachine::GetInstance().DumpTrace(TraceScenario::TRACE_DYNAMIC, 0, 0, traceRetInfo);
    appCallerEvent_->taskBeginTime_ = static_cast<int64_t>(TraceStateMachine::GetInstance().GetTaskBeginTime());
#else
    TraceRet ret(traceRetInfo.errorCode);
#endif
    appCallerEvent_->taskEndTime_ = static_cast<int64_t>(TimeUtil::GetMilliseconds());
    if (ret.IsSuccess()) {
        if (traceRetInfo.outputFiles.empty() || traceRetInfo.outputFiles[0].empty()) {
            HIVIEW_LOGE("TraceAppStrategy dump file empty");
            return TraceRet(TraceStateCode::FAIL);
        }
        if (traceHandler_ != nullptr) {
            outputFiles = traceHandler_->HandleTrace(traceRetInfo.outputFiles, {}, appCallerEvent_);
        }
        traceFlowController_->RecordCaller(appCallerEvent_);
    }
    ShareAppEvent(appCallerEvent_);
    CleanOldAppTrace();
    ReportMainThreadJankForTrace(appCallerEvent_);
    return ret;
}

void TraceAppStrategy::ReportMainThreadJankForTrace(std::shared_ptr<AppCallerEvent> appCallerEvent)
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

void TraceAppStrategy::ShareAppEvent(std::shared_ptr<AppCallerEvent> appCallerEvent)
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
}
