/*
 * Copyright (C) 2023-2024 Huawei Device Co., Ltd.
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
#include "unified_collector.h"

#include <ctime>
#include <memory>
#include <fcntl.h>
#include <sys/file.h>

#include "collect_event.h"
#include "ffrt.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "io_collector.h"
#include "memory_collector.h"
#include "parameter_ex.h"
#include "plugin_factory.h"
#include "process_status.h"
#include "sys_event.h"
#include "string_util.h"
#include "time_util.h"
#include "uc_observer_mgr.h"
#include "unified_collection_stat.h"
#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
#include "app_caller_event.h"
#include "event_publish.h"
#include "hisysevent.h"
#include "trace_worker.h"
#include "trace_utils.h"
#include "trace_state_machine.h"
#include "trace_flow_controller.h"
#include "uc_telemetry_listener.h"
#include "unified_common.h"
#endif

namespace OHOS {
namespace HiviewDFX {
REGISTER(UnifiedCollector);
DEFINE_LOG_TAG("HiView-UnifiedCollector");
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace std::literals::chrono_literals;
namespace {
const std::string HIPERF_LOG_PATH = "/data/log/hiperf";
const std::string COLLECTION_IO_PATH = "/data/log/hiview/unified_collection/io/";
const std::string HIVIEW_UCOLLECTION_STATE_TRUE = "true";
const std::string HIVIEW_UCOLLECTION_STATE_FALSE = "false";
#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
const std::string UNIFIED_SHARE_PATH = "/data/log/hiview/unified_collection/trace/share/";
const std::string UNIFIED_SPECIAL_PATH = "/data/log/hiview/unified_collection/trace/special/";
const std::string UNIFIED_TELEMETRY_PATH = "/data/log/hiview/unified_collection/trace/telemetry/";
const std::string UNIFIED_SHARE_TEMP_PATH = UNIFIED_SHARE_PATH + "temp/";
const std::string DEVELOP_TRACE_RECORDER_FALSE = "false";
constexpr char KEY_FREEZE_DETECTOR_STATE[] = "persist.hiview.freeze_detector";
const std::string OTHER = "Other";
using namespace OHOS::HiviewDFX::Hitrace;
constexpr int32_t DURATION_TRACE = 10; // unit second

void CreateTracePathInner(const std::string &filePath)
{
    if (FileUtil::FileExists(filePath)) {
        return;
    }
    if (!CreateMultiDirectory(filePath)) {
        HIVIEW_LOGE("failed to create multidirectory %{public}s.", filePath.c_str());
        return;
    }
}

void CreateTracePath()
{
    CreateTracePathInner(UNIFIED_SHARE_PATH);
    CreateTracePathInner(UNIFIED_SPECIAL_PATH);
    CreateTracePathInner(UNIFIED_TELEMETRY_PATH);
}

void RecoverTmpTrace()
{
    std::vector<std::string> traceFiles;
    FileUtil::GetDirFiles(UNIFIED_SHARE_TEMP_PATH, traceFiles, false);
    HIVIEW_LOGI("traceFiles need recover: %{public}zu", traceFiles.size());
    for (auto &filePath : traceFiles) {
        std::string fileName = FileUtil::ExtractFileName(filePath);
        HIVIEW_LOGI("unfinished trace file: %{public}s", fileName.c_str());
        std::string originTraceFile = StringUtil::ReplaceStr("/data/log/hitrace/" + fileName, ".zip", ".sys");
        if (!FileUtil::FileExists(originTraceFile)) {
            HIVIEW_LOGI("source file not exist: %{public}s", originTraceFile.c_str());
            FileUtil::RemoveFile(UNIFIED_SHARE_TEMP_PATH + fileName);
            continue;
        }
        int fd = open(originTraceFile.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd == -1) {
            HIVIEW_LOGI("open source file failed: %{public}s", originTraceFile.c_str());
            continue;
        }
        // add lock before zip trace file, in case hitrace delete origin trace file.
        if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
            HIVIEW_LOGI("get source file lock failed: %{public}s", originTraceFile.c_str());
            close(fd);
            continue;
        }
        HIVIEW_LOGI("originTraceFile path: %{public}s", originTraceFile.c_str());
        UcollectionTask traceTask = [=]() {
            ZipTraceFile(originTraceFile, UNIFIED_SHARE_PATH + fileName);
            flock(fd, LOCK_UN);
            close(fd);
        };
        TraceWorker::GetInstance().HandleUcollectionTask(traceTask);
    }
}
#endif
}

void UnifiedCollector::OnLoad()
{
    HIVIEW_LOGI("start to load UnifiedCollector plugin");
    Init();
}

void UnifiedCollector::OnUnload()
{
    HIVIEW_LOGI("start to unload UnifiedCollector plugin");
    UcObserverManager::GetInstance().UnregisterObservers();
}

#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
void UnifiedCollector::OnFreezeDetectorParamChanged(const char* key, const char* value, void* context)
{
    if (key == nullptr || value == nullptr) {
        HIVIEW_LOGW("key or value is null");
        return;
    }
    if (strncmp(key, KEY_FREEZE_DETECTOR_STATE, strlen(KEY_FREEZE_DETECTOR_STATE)) != 0) {
        HIVIEW_LOGW("key is not wanted, key: %{public}s", key);
        return;
    }
    HIVIEW_LOGI("freeze detector param changed, value: %{public}s", value);
    if (strncmp(value, "true", strlen("true")) == 0) {
        TraceStateMachine::GetInstance().SetTraceSwitchFreezeOn();
    } else {
        TraceStateMachine::GetInstance().SetTraceSwitchFreezeOff();
    }
}

void UnifiedCollector::OnSwitchRecordTraceStateChanged(const char* key, const char* value, void* context)
{
    if (key == nullptr || value == nullptr) {
        HIVIEW_LOGE("record trace switch input ptr null");
        return;
    }
    if (strncmp(key, DEVELOP_HIVIEW_TRACE_RECORDER, strlen(DEVELOP_HIVIEW_TRACE_RECORDER)) != 0) {
        HIVIEW_LOGE("record trace switch param key error");
        return;
    }
    if (strncmp(value, "true", strlen("true")) == 0) {
        TraceStateMachine::GetInstance().SetTraceSwitchDevOn();
    } else {
        TraceStateMachine::GetInstance().SetTraceSwitchDevOff();
    }
}

void UnifiedCollector::LoadTraceSwitch()
{
    if (Parameter::IsBetaVersion()) {
        TraceStateMachine::GetInstance().SetTraceVersionBeta();
    } else {
        int watchFreezeRet = Parameter::WatchParamChange(KEY_FREEZE_DETECTOR_STATE,
            OnFreezeDetectorParamChanged, nullptr);
        HIVIEW_LOGI("watchFreezeRet:%{public}d", watchFreezeRet);
    }
    if (Parameter::IsUCollectionSwitchOn()) {
        TraceStateMachine::GetInstance().SetTraceSwitchUcOn();
    }
    if (Parameter::GetBoolean(KEY_FREEZE_DETECTOR_STATE, false)) {
        TraceStateMachine::GetInstance().SetTraceSwitchFreezeOn();
    }
    if (Parameter::IsDeveloperMode()) {
        if (Parameter::IsTraceCollectionSwitchOn()) {
            TraceStateMachine::GetInstance().SetTraceSwitchDevOn();
        }
        int ret = Parameter::WatchParamChange(DEVELOP_HIVIEW_TRACE_RECORDER, OnSwitchRecordTraceStateChanged, this);
        HIVIEW_LOGI("add ucollection trace switch param watcher ret: %{public}d", ret);
    }
}

void UnifiedCollector::OnMainThreadJank(SysEvent& sysEvent)
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

bool UnifiedCollector::OnEvent(std::shared_ptr<Event>& event)
{
    if (event == nullptr || workLoop_ == nullptr) {
        return true;
    }
    HIVIEW_LOGI("Receive Event %{public}s", event->GetEventName().c_str());
    if (event->eventName_ == UCollectUtil::START_APP_TRACE) {
        event->eventName_ = UCollectUtil::STOP_APP_TRACE;
        DelayProcessEvent(event, DURATION_TRACE);
        return true;
    }
    if (event->eventName_ == UCollectUtil::STOP_APP_TRACE) {
        auto ret = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_DYNAMIC);
        if (!ret.IsSuccess()) {
            HIVIEW_LOGW("CloseTrace app trace fail");
        }
        return true;
    }
    if (event->eventName_ == TelemetryEvent::TELEMETRY_START) {
        HandleTeleMetryStart(event);
        return true;
    }
    if (event->eventName_ == TelemetryEvent::TELEMETRY_STOP) {
        HandleTeleMetryStop();
        return true;
    }
    if (event->eventName_ == TelemetryEvent::TELEMETRY_TIMEOUT) {
        HandleTeleMetryTimeout();
        return true;
    }
    return true;
}

void UnifiedCollector::HandleTeleMetryStart(std::shared_ptr<Event> &event)
{
    int32_t delay = event->GetIntValue(Telemetry::KEY_DELAY_TIME);
    if (delay > 0) {
        HIVIEW_LOGI("delay:%{public}d", delay);
        event->SetValue(Telemetry::KEY_DELAY_TIME, 0);
        auto seqId = workLoop_->AddTimerEvent(shared_from_this(), event, nullptr, static_cast<uint64_t>(delay), false);
        telemetryList_.push_back(seqId);
        return;
    }
    std::string tag = event->GetValue(Telemetry::KEY_TELEMETRY_TRACE_TAG);
    int32_t traceDuration = event->GetIntValue(Telemetry::KEY_REMAIN_TIME);
    std::string telemetryId = event->GetValue(Telemetry::KEY_ID);
    std::string bundleName = event->GetValue(Telemetry::KEY_BUNDLE_NAME);
    if (traceDuration <= 0) {
        HIVIEW_LOGE("system error traceDuration:%{public}d", traceDuration);
        return;
    }
    auto ret = TraceStateMachine::GetInstance().OpenTelemetryTrace(tag);
    HiSysEventWrite(Telemetry::TELEMETRY_DOMAIN, "TASK_INFO", HiSysEvent::EventType::STATISTIC,
        "ID", telemetryId,
        "STAGE", "TRACE_BEGIN",
        "ERROR", std::to_string(GetUcError(ret)),
        "BUNDLE_NAME", bundleName);
    event->eventName_ = TelemetryEvent::TELEMETRY_TIMEOUT;
    if (ret.IsSuccess()) {
        bool isSuccess = TraceStateMachine::GetInstance().RegisterTelemetryCallback([telemetryId, bundleName]() {
            HiSysEventWrite(Telemetry::TELEMETRY_DOMAIN, "TASK_INFO", HiSysEvent::EventType::STATISTIC,
                "ID", telemetryId,
                "STAGE", "TRACE_END",
                "ERROR", 0,
                "BUNDLE_NAME", bundleName);
        });
        HIVIEW_LOGE("RegisterTelemetryCallback:%{public}d", isSuccess);
    }
    auto seqId = workLoop_->AddTimerEvent(shared_from_this(), event, nullptr, static_cast<uint64_t>(traceDuration),
        false);
    telemetryList_.push_back(seqId);
}

void UnifiedCollector::HandleTeleMetryStop()
{
    TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    TraceFlowController controller(BusinessName::TELEMETRY);
    controller.ClearTelemetryData();
    for (auto it : telemetryList_) {
        workLoop_->RemoveEvent(it);
    }
    telemetryList_.clear();
}

void UnifiedCollector::HandleTeleMetryTimeout()
{
    TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
}

void UnifiedCollector::OnEventListeningCallback(const Event& event)
{
    SysEvent& sysEvent = static_cast<SysEvent&>(const_cast<Event&>(event));
    HIVIEW_LOGI("sysevent %{public}s", sysEvent.eventName_.c_str());

    if (sysEvent.eventName_ == UCollectUtil::MAIN_THREAD_JANK) {
        OnMainThreadJank(sysEvent);
        return;
    }
}

void UnifiedCollector::Dump(int fd, const std::vector<std::string>& cmds)
{
    dprintf(fd, "device beta state is %s.\n", Parameter::IsBetaVersion() ? "beta" : "is not beta");

    std::string remoteLogState = Parameter::GetString(HIVIEW_UCOLLECTION_STATE, HIVIEW_UCOLLECTION_STATE_FALSE);
    dprintf(fd, "remote log state is %s.\n", remoteLogState.c_str());

    std::string traceRecorderState = Parameter::GetString(DEVELOP_HIVIEW_TRACE_RECORDER, DEVELOP_TRACE_RECORDER_FALSE);
    dprintf(fd, "trace recorder state is %s.\n", traceRecorderState.c_str());

    dprintf(fd, "develop state is %s.\n", Parameter::IsDeveloperMode() ? "true" : "false");
}

#ifdef HIVIEW_LOW_MEM_THRESHOLD
void UnifiedCollector::RunCacheMonitorLoop()
{
    if (traceCacheMonitor_ == nullptr) {
        traceCacheMonitor_ = std::make_shared<TraceCacheMonitor>();
    }
    traceCacheMonitor_->RunMonitorLoop();
}

void UnifiedCollector::ExitCacheMonitorLoop()
{
    if (traceCacheMonitor_ != nullptr) {
        traceCacheMonitor_->ExitMonitorLoop();
    }
}
#endif
#endif

void UnifiedCollector::Init()
{
    auto context = GetHiviewContext();
    if (context == nullptr) {
        HIVIEW_LOGE("hiview context is null");
        return;
    }
    workLoop_ = context->GetSharedWorkLoop();
    if (workLoop_ == nullptr) {
        HIVIEW_LOGE("workLoop is null");
        return;
    }
    InitWorkPath();
#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
    CreateTracePath();
    LoadTraceSwitch();
    telemetryListener_ = std::make_shared<TelemetryListener>(shared_from_this());
    context->AddListenerInfo(Event::MessageType::TELEMETRY_EVENT, telemetryListener_->GetListenerName());
    context->RegisterUnorderedEventListener(telemetryListener_);
    RecoverTmpTrace();
#endif
    if (Parameter::IsBetaVersion() || Parameter::IsUCollectionSwitchOn()) {
        RunIoCollectionTask();
        RunUCollectionStatTask();
    }
    if (Parameter::IsBetaVersion() || Parameter::IsUCollectionSwitchOn() || Parameter::IsDeveloperMode()) {
        RunCpuCollectionTask();
#ifdef HIVIEW_LOW_MEM_THRESHOLD
        RunCacheMonitorLoop();
#endif
    }
    if (!Parameter::IsBetaVersion()) {
        int watchUcollectionRet = Parameter::WatchParamChange(HIVIEW_UCOLLECTION_STATE, OnSwitchStateChanged, this);
        HIVIEW_LOGI("watchUcollectionRet:%{public}d", watchUcollectionRet);
    }
    UcObserverManager::GetInstance().RegisterObservers();
}

void UnifiedCollector::CleanDataFiles()
{
#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
    std::vector<std::string> files;
    FileUtil::GetDirFiles(UNIFIED_SPECIAL_PATH, files);
    for (const auto& file : files) {
        if (file.find(OTHER) != std::string::npos) {
            FileUtil::RemoveFile(file);
        }
    }
#endif
    FileUtil::ForceRemoveDirectory(COLLECTION_IO_PATH, false);
    FileUtil::ForceRemoveDirectory(HIPERF_LOG_PATH, false);
}

void UnifiedCollector::OnSwitchStateChanged(const char* key, const char* value, void* context)
{
    if (context == nullptr || key == nullptr || value == nullptr) {
        HIVIEW_LOGE("input ptr null");
        return;
    }
    if (strncmp(key, HIVIEW_UCOLLECTION_STATE, strlen(HIVIEW_UCOLLECTION_STATE)) != 0) {
        HIVIEW_LOGE("param key error");
        return;
    }
    HIVIEW_LOGI("ucollection switch state changed, ret: %{public}s", value);
    auto* unifiedCollectorPtr = static_cast<UnifiedCollector*>(context);
    if (unifiedCollectorPtr == nullptr) {
        HIVIEW_LOGE("unifiedCollectorPtr is null");
        return;
    }
    if (value == HIVIEW_UCOLLECTION_STATE_TRUE) {
        unifiedCollectorPtr->RunCpuCollectionTask();
        unifiedCollectorPtr->RunIoCollectionTask();
        unifiedCollectorPtr->RunUCollectionStatTask();
#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
        TraceStateMachine::GetInstance().SetTraceSwitchUcOn();
#ifdef HIVIEW_LOW_MEM_THRESHOLD
        unifiedCollectorPtr->RunCacheMonitorLoop();
#endif
#endif
    } else {
        if (!Parameter::IsDeveloperMode()) {
            unifiedCollectorPtr->isCpuTaskRunning_ = false;
        }
        for (const auto &it : unifiedCollectorPtr->taskList_) {
            unifiedCollectorPtr->workLoop_->RemoveEvent(it);
        }
        unifiedCollectorPtr->taskList_.clear();
#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
        TraceStateMachine::GetInstance().SetTraceSwitchUcOff();
#ifdef HIVIEW_LOW_MEM_THRESHOLD
        unifiedCollectorPtr->ExitCacheMonitorLoop();
#endif
#endif
        unifiedCollectorPtr->CleanDataFiles();
    }
}

void UnifiedCollector::InitWorkPath()
{
    std::string hiviewWorkDir = GetHiviewContext()->GetHiViewDirectory(HiviewContext::DirectoryType::WORK_DIRECTORY);
    const std::string uCollectionDirName = "unified_collection";
    std::string tempWorkPath = FileUtil::IncludeTrailingPathDelimiter(hiviewWorkDir.append(uCollectionDirName));
    if (!FileUtil::IsDirectory(tempWorkPath) && !FileUtil::ForceCreateDirectory(tempWorkPath)) {
        HIVIEW_LOGE("failed to create dir=%{public}s", tempWorkPath.c_str());
        return;
    }
    workPath_ = tempWorkPath;
}

void UnifiedCollector::RunCpuCollectionTask()
{
    if (workPath_.empty() || isCpuTaskRunning_) {
        HIVIEW_LOGE("workPath is null or task is running");
        return;
    }
    isCpuTaskRunning_ = true;
    auto task = [this] { this->CpuCollectionFfrtTask(); };
    ffrt::submit(task, {}, {}, ffrt::task_attr().name("dft_uc_cpu").qos(ffrt::qos_default));
}

void UnifiedCollector::CpuCollectionFfrtTask()
{
    cpuCollectionTask_ = std::make_shared<CpuCollectionTask>(workPath_);
    while (true) {
        if (!isCpuTaskRunning_) {
            HIVIEW_LOGE("exit cpucollection task");
            break;
        }
        ffrt::this_task::sleep_for(10s); // 10s: collect period
        cpuCollectionTask_->Collect();
    }
}

void UnifiedCollector::RunIoCollectionTask()
{
    if (workLoop_ == nullptr) {
        HIVIEW_LOGE("workLoop is null");
        return;
    }
    auto ioCollectionTask = [this] { this->IoCollectionTask(); };
    const uint64_t taskInterval = 30; // 30s
    auto ioSeqId = workLoop_->AddTimerEvent(nullptr, nullptr, ioCollectionTask, taskInterval, true);
    taskList_.push_back(ioSeqId);
}

void UnifiedCollector::IoCollectionTask()
{
    auto ioCollector = UCollectUtil::IoCollector::Create();
    (void)ioCollector->CollectDiskStats([](const DiskStats &stats) { return false; }, true);
    (void)ioCollector->CollectAllProcIoStats(true);
}

void UnifiedCollector::RunUCollectionStatTask()
{
    if (workLoop_ == nullptr) {
        HIVIEW_LOGE("workLoop is null");
        return;
    }
    auto statTask = [this] { this->UCollectionStatTask(); };
    const uint64_t taskInterval = 600; // 600s
    auto statSeqId = workLoop_->AddTimerEvent(nullptr, nullptr, statTask, taskInterval, true);
    taskList_.push_back(statSeqId);
}

void UnifiedCollector::UCollectionStatTask()
{
    UnifiedCollectionStat stat;
    stat.Report();
}
} // namespace HiviewDFX
} // namespace OHOS
