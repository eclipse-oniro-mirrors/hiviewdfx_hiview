/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "app_trace_context.h"

#include <cinttypes>
#include <memory>
#include <mutex>

#include "app_caller_event.h"
#include "collect_event.h"
#include "event_publish.h"
#include "file_util.h"
#include "hisysevent.h"
#include "hiview_logger.h"
#include "json/json.h"
#include "time_util.h"
#include "trace_flow_controller.h"
#include "trace_manager.h"
#include "utility/trace_collector.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-AppTrace");
namespace {
std::recursive_mutex g_traceMutex;
// start => dump, start => stop, dump => stop, stop -> start
constexpr int32_t TRACE_STATE_START_APP_TRACE = 1;
constexpr int32_t TRACE_STATE_DUMP_APP_TRACE = 2;
constexpr int32_t TRACE_STATE_STOP_APP_TRACE = 3;
constexpr int32_t DURATION_TRACE = 10; // unit second
const std::string UNIFIED_SHARE_PATH = "/data/log/hiview/unified_collection/trace/share/";

std::string InnerMakeTraceFileName(std::shared_ptr<AppCallerEvent> appCallerEvent)
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

bool InnerStartAppTrace(std::shared_ptr<AppCallerEvent> appCallerEvent, bool &isOpenTrace, bool &isTraceOn)
{
    TraceManager manager;
    std::string appArgs = "tags:graphic,ace,app clockType:boot bufferSize:1024 overwrite:1 fileLimit:20 ";
    appArgs.append("appPid:").append(std::to_string(appCallerEvent->pid_));
    int32_t retCode = manager.OpenRecordingTrace(appArgs);
    if (retCode != UCollect::UcError::SUCCESS) {
        HIVIEW_LOGE("failed to open trace for uid=%{public}d, pid=%{public}d, error code %{public}d",
            appCallerEvent->uid_, appCallerEvent->pid_, retCode);
        appCallerEvent->resultCode_ = retCode;
        return false;
    }
    isOpenTrace = true;

    std::shared_ptr<UCollectUtil::TraceCollector> traceCollector = UCollectUtil::TraceCollector::Create();
    CollectResult<int32_t> result = traceCollector->TraceOn();
    if (result.retCode != UCollect::UcError::SUCCESS) {
        HIVIEW_LOGE("failed to traceon for uid=%{public}d, pid=%{public}d, error code %{public}d",
            appCallerEvent->uid_, appCallerEvent->pid_, retCode);
        appCallerEvent->resultCode_ = result.retCode;
        return false;
    }
    isTraceOn = true;
    return true;
}

void InnerDumpAppTrace(std::shared_ptr<AppCallerEvent> appCallerEvent, bool &isDumpTrace, bool &isTraceOn)
{
    std::shared_ptr<UCollectUtil::TraceCollector> traceCollector = UCollectUtil::TraceCollector::Create();
    CollectResult<std::vector<std::string>> result = traceCollector->TraceOff();
    if (result.retCode != UCollect::UcError::SUCCESS) {
        HIVIEW_LOGE("trace off for uid=%{public}d pid=%{public}d error code=%{public}d",
            appCallerEvent->uid_, appCallerEvent->pid_, result.retCode);
        appCallerEvent->resultCode_ = result.retCode;
        return;
    }

    isTraceOn = false;
    appCallerEvent->taskEndTime_ = static_cast<int64_t>(TimeUtil::GetMilliseconds());
    if (result.data.empty()) {
        HIVIEW_LOGE("failed to collect app trace for uid=%{public}d pid=%{public}d",
            appCallerEvent->uid_, appCallerEvent->pid_);
    } else {
        isDumpTrace = true;
        std::string traceFileName = InnerMakeTraceFileName(appCallerEvent);
        FileUtil::RenameFile(result.data[0], traceFileName);
        appCallerEvent->externalLog_ = traceFileName;
    }
}

void InnerShareAppEvent(std::shared_ptr<AppCallerEvent> appCallerEvent)
{
    Json::Value eventJson;
    eventJson[UCollectUtil::APP_EVENT_PARAM_UID] = appCallerEvent->uid_;
    eventJson[UCollectUtil::APP_EVENT_PARAM_PID] = appCallerEvent->pid_;
    eventJson[UCollectUtil::APP_EVENT_PARAM_TIME] = appCallerEvent->happenTime_;
    eventJson[UCollectUtil::APP_EVENT_PARAM_BUNDLE_NAME] = appCallerEvent->bundleName_;
    eventJson[UCollectUtil::APP_EVENT_PARAM_BUNDLE_VERSION] = appCallerEvent->bundleVersion_;
    eventJson[UCollectUtil::APP_EVENT_PARAM_BEGIN_TIME] = appCallerEvent->beginTime_;
    eventJson[UCollectUtil::APP_EVENT_PARAM_END_TIME] = appCallerEvent->endTime_;
    if (appCallerEvent->isBusinessJank_) {
        eventJson[UCollectUtil::APP_EVENT_PARAM_ISBUSINESSJANK] = appCallerEvent->isBusinessJank_;
    }
    Json::Value externalLog;
    externalLog.append(appCallerEvent->externalLog_);
    eventJson[UCollectUtil::APP_EVENT_PARAM_EXTERNAL_LOG] = externalLog;
    std::string param = Json::FastWriter().write(eventJson);

    HIVIEW_LOGI("send for uid=%{public}d pid=%{public}d", appCallerEvent->uid_, appCallerEvent->pid_);
    EventPublish::GetInstance().PushEvent(appCallerEvent->uid_, UCollectUtil::MAIN_THREAD_JANK,
        HiSysEvent::EventType::FAULT, param);
}

void InnerReportMainThreadJankForTrace(std::shared_ptr<AppCallerEvent> appCallerEvent)
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

bool InnerHasCallAppTrace(std::shared_ptr<AppCallerEvent> appCallerEvent)
{
    std::shared_ptr<TraceFlowController> traceController = std::make_shared<TraceFlowController>();
    if (traceController->HasCallOnceToday(appCallerEvent->uid_, appCallerEvent->happenTime_)) {
        HIVIEW_LOGE("already capture trace uid=%{public}d pid=%{public}d",
            appCallerEvent->uid_, appCallerEvent->pid_);
        appCallerEvent->resultCode_ = UCollect::UcError::HAD_CAPTURED_TRACE;
        return true;
    }
    return false;
}
}

AppTraceContext::AppTraceContext(std::shared_ptr<AppTraceState> state)
    : pid_(0), traceBegin_(0), isOpenTrace_(false), isTraceOn_(false), isDumpTrace_(false), state_(state)
{}

int32_t AppTraceContext::TransferTo(std::shared_ptr<AppTraceState> state)
{
    std::lock_guard<std::recursive_mutex> guard(g_traceMutex);
    if (state_ == nullptr) {
        state_ = state;
        return state_->CaptureTrace();
    }

    if (!state->Accept(state_)) {
        return -1;
    }

    state_ = state;
    return state_->CaptureTrace();
}

void AppTraceContext::PublishStackEvent(SysEvent& sysEvent)
{
    if (sysEvent.GetEventIntValue(UCollectUtil::SYS_EVENT_PARAM_JANK_LEVEL) <
        UCollectUtil::SYS_EVENT_JANK_LEVEL_VALUE_TRACE) {
        // hicollie capture stack in application process, only need to share app event to application by hiview
        Json::Value eventJson;
        eventJson[UCollectUtil::APP_EVENT_PARAM_UID] = sysEvent.GetUid();
        eventJson[UCollectUtil::APP_EVENT_PARAM_PID] = sysEvent.GetPid();
        eventJson[UCollectUtil::APP_EVENT_PARAM_TIME] = sysEvent.happenTime_;
        eventJson[UCollectUtil::APP_EVENT_PARAM_BUNDLE_NAME] = sysEvent.GetEventValue(
            UCollectUtil::SYS_EVENT_PARAM_BUNDLE_NAME);
        eventJson[UCollectUtil::APP_EVENT_PARAM_BUNDLE_VERSION] = sysEvent.GetEventValue(
            UCollectUtil::SYS_EVENT_PARAM_BUNDLE_VERSION);
        eventJson[UCollectUtil::APP_EVENT_PARAM_BEGIN_TIME] = sysEvent.GetEventIntValue(
            UCollectUtil::SYS_EVENT_PARAM_BEGIN_TIME);
        eventJson[UCollectUtil::APP_EVENT_PARAM_END_TIME] = sysEvent.GetEventIntValue(
            UCollectUtil::SYS_EVENT_PARAM_END_TIME);
        eventJson[UCollectUtil::APP_EVENT_PARAM_APP_START_JIFFIES_TIME] = sysEvent.GetEventIntValue(
            UCollectUtil::SYS_EVENT_PARAM_APP_START_JIFFIES_TIME);
        eventJson[UCollectUtil::APP_EVENT_PARAM_HEAVIEST_STACK] = sysEvent.GetEventValue(
            UCollectUtil::SYS_EVENT_PARAM_HEAVIEST_STACK);
        Json::Value externalLog;
        externalLog.append(sysEvent.GetEventValue(UCollectUtil::SYS_EVENT_PARAM_EXTERNAL_LOG));
        eventJson[UCollectUtil::APP_EVENT_PARAM_EXTERNAL_LOG] = externalLog;
        std::string param = Json::FastWriter().write(eventJson);

        HIVIEW_LOGI("send as stack trigger for uid=%{public}d pid=%{public}d", sysEvent.GetUid(), sysEvent.GetPid());
        EventPublish::GetInstance().PushEvent(sysEvent.GetUid(), UCollectUtil::MAIN_THREAD_JANK,
            HiSysEvent::EventType::FAULT, param);
    }
}

void AppTraceContext::Reset()
{
    pid_ = 0;
    traceBegin_ = 0;
    isOpenTrace_ = false;
    isTraceOn_ = false;
    isDumpTrace_ = false;
}

bool StartTraceState::Accept(std::shared_ptr<AppTraceState> preState)
{
    // stop -> start
    if (preState->GetState() == TRACE_STATE_STOP_APP_TRACE) {
        return true;
    }

    appCallerEvent_->resultCode_ = UCollect::UcError::INVALID_TRACE_STATE;
    return false;
}

int32_t StartTraceState::GetState()
{
    return TRACE_STATE_START_APP_TRACE;
}

int32_t StartTraceState::CaptureTrace()
{
    int32_t retCode = DoCaptureTrace();

    appCallerEvent_->eventName_ = UCollectUtil::STOP_APP_TRACE;
    if (retCode == 0) {
        // start -> stop after DURATION_TRACE seconds
        plugin_->DelayProcessEvent(appCallerEvent_, DURATION_TRACE);
    } else {
        // start -> stop right now as start trace fail
        // need to change to stop state then can wait for next start
        appTraceContext_->TransferTo(std::make_shared<StopTraceState>(appTraceContext_, appCallerEvent_));
    }

    return retCode;
}

int32_t StartTraceState::DoCaptureTrace()
{
    if (AppCallerEvent::isDynamicTraceOpen_) {
        HIVIEW_LOGE("start trace is already open uid=%{public}d, pid=%{public}d",
            appCallerEvent_->uid_, appCallerEvent_->pid_);
        appCallerEvent_->resultCode_ = UCollect::UcError::EXISTS_CAPTURE_TASK;
        return -1;
    }

    // app only has one trace file each day
    if (InnerHasCallAppTrace(appCallerEvent_)) {
        return -1;
    }

    AppCallerEvent::isDynamicTraceOpen_ = true;
    HIVIEW_LOGI("start trace serval seconds for uid=%{public}d pid=%{public}d",
        appCallerEvent_->uid_, appCallerEvent_->pid_);

    if (!InnerStartAppTrace(appCallerEvent_, appTraceContext_->isOpenTrace_, appTraceContext_->isTraceOn_)) {
        return -1;
    }

    appTraceContext_->traceBegin_ = static_cast<int64_t>(TimeUtil::GetMilliseconds());
    appCallerEvent_->resultCode_ = UCollect::UcError::SUCCESS;
    int64_t delay = appCallerEvent_->taskBeginTime_ - static_cast<int64_t>(appCallerEvent_->happenTime_);
    int64_t cost = appTraceContext_->traceBegin_ - static_cast<int64_t>(appCallerEvent_->happenTime_);
    HIVIEW_LOGI("trace is start for uid=%{public}d pid=%{public}d delay=%{public}" PRId64 ", cost=%{public}" PRId64 "",
        appCallerEvent_->uid_, appCallerEvent_->pid_, delay, cost);
    return 0;
}

bool DumpTraceState::Accept(std::shared_ptr<AppTraceState> preState)
{
    // start -> dump
    if (preState->GetState() == TRACE_STATE_START_APP_TRACE) {
        if (preState->appCallerEvent_->pid_ != appCallerEvent_->pid_) {
            appCallerEvent_->resultCode_ = UCollect::UcError::INCONSISTENT_PROCESS;
            return false;
        }
        return true;
    }

    appCallerEvent_->resultCode_ = UCollect::UcError::INVALID_TRACE_STATE;
    return false;
}

int32_t DumpTraceState::GetState()
{
    return TRACE_STATE_DUMP_APP_TRACE;
}

int32_t DumpTraceState::CaptureTrace()
{
    int32_t retCode = DoCaptureTrace();

    // dump -> stop right now
    appCallerEvent_->eventName_ = UCollectUtil::STOP_APP_TRACE;
    appTraceContext_->TransferTo(std::make_shared<StopTraceState>(appTraceContext_, appCallerEvent_));
    return retCode;
}

int32_t DumpTraceState::DoCaptureTrace()
{
    if (!AppCallerEvent::isDynamicTraceOpen_) {
        HIVIEW_LOGE("dump trace is not open uid=%{public}d, pid=%{public}d",
            appCallerEvent_->uid_, appCallerEvent_->pid_);
        appCallerEvent_->resultCode_ = UCollect::UcError::UNEXISTS_CAPTURE_TASK;
        return -1;
    }

    // app only has one trace file each day
    if (InnerHasCallAppTrace(appCallerEvent_)) {
        return -1;
    }

    int64_t delay = appCallerEvent_->taskBeginTime_ - static_cast<int64_t>(appCallerEvent_->happenTime_);
    appCallerEvent_->taskBeginTime_ = appTraceContext_->traceBegin_;
    InnerDumpAppTrace(appCallerEvent_, appTraceContext_->isDumpTrace_, appTraceContext_->isTraceOn_);

    // hicollie capture stack in application process, only need to share app event to application by hiview
    InnerShareAppEvent(appCallerEvent_);

    std::shared_ptr<TraceFlowController> traceController = std::make_shared<TraceFlowController>();
    traceController->RecordCaller(appCallerEvent_);
    traceController->CleanOldAppTrace();

    InnerReportMainThreadJankForTrace(appCallerEvent_);

    int64_t cost = appCallerEvent_->taskEndTime_ - static_cast<int64_t>(appCallerEvent_->happenTime_);
    HIVIEW_LOGI("dump trace for uid=%{public}d pid=%{public}d  delay=%{public}" PRId64 ", cost=%{public}" PRId64 "",
        appCallerEvent_->uid_, appCallerEvent_->pid_, delay, cost);
    return 0;
}

bool StopTraceState::Accept(std::shared_ptr<AppTraceState> preState)
{
    // start -> stop
    if (preState->GetState() == TRACE_STATE_START_APP_TRACE) {
        return true;
    }

    // dump -> stop
    if (preState->GetState() == TRACE_STATE_DUMP_APP_TRACE) {
        return true;
    }

    HIVIEW_LOGI("already stop for uid=%{public}d pid=%{public}d", appCallerEvent_->uid_, appCallerEvent_->pid_);
    return false;
}

int32_t StopTraceState::GetState()
{
    return TRACE_STATE_STOP_APP_TRACE;
}


int32_t StopTraceState::CaptureTrace()
{
    if (appTraceContext_ == nullptr || appCallerEvent_ == nullptr) {
        HIVIEW_LOGI("does not init state, can not run");
        return -1;
    }

    int32_t retCode = DoCaptureTrace();
    appTraceContext_->Reset();
    AppCallerEvent::isDynamicTraceOpen_ = false;
    return retCode;
}

int32_t StopTraceState::DoCaptureTrace()
{
    if (!appTraceContext_->isOpenTrace_) {
        HIVIEW_LOGI("as start trace failed, does not need stop trace for uid=%{public}d pid=%{public}d",
            appCallerEvent_->uid_, appCallerEvent_->pid_);
        return 0;
    }

    if (appTraceContext_->isTraceOn_) {
        std::shared_ptr<UCollectUtil::TraceCollector> traceCollector = UCollectUtil::TraceCollector::Create();
        CollectResult<std::vector<std::string>> result = traceCollector->TraceOff();
        if (result.retCode != UCollect::UcError::SUCCESS) {
            HIVIEW_LOGE("trace off for uid=%{public}d pid=%{public}d error code=%{public}d",
                appCallerEvent_->uid_, appCallerEvent_->pid_, result.retCode);
        }
    }

    HIVIEW_LOGI("%{public}s for uid=%{public}d pid=%{public}d",
        (appTraceContext_->isDumpTrace_ ? "stop trace" : "no dump"), appCallerEvent_->uid_, appCallerEvent_->pid_);

    TraceManager manager;
    int32_t retCode = manager.CloseTrace();
    if (retCode != UCollect::UcError::SUCCESS) {
        HIVIEW_LOGE("close trace for uid=%{public}d pid=%{public}d error code=%{public}d",
            appCallerEvent_->uid_, appCallerEvent_->pid_, retCode);
    }
    return 0;
}
}; // HiviewDFX
}; // HiviewDFX