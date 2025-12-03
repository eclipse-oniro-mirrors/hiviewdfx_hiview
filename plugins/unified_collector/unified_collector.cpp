/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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
#include <sys/file.h>

#include "collect_event.h"
#include "ffrt.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "io_collector.h"
#include "parameter_ex.h"
#ifdef HAS_HIPERF
#include "perf_collect_config.h"
#endif
#include "plugin_factory.h"
#include "process_status.h"
#include "sys_event.h"
#include "string_util.h"
#include "uc_observer_mgr.h"
#include "unified_collection_stat.h"
#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
#include "event_publish.h"
#include "hisysevent.h"
#include "json/json.h"
#include "trace_state_machine.h"
#include "uc_telemetry_listener.h"
#include "trace_collector.h"
#endif

namespace OHOS {
namespace HiviewDFX {
REGISTER(UnifiedCollector);
DEFINE_LOG_TAG("HiView-UnifiedCollector");
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace std::literals::chrono_literals;
namespace {
constexpr char HIPERF_LOG_PATH[] = "/data/log/hiperf";
constexpr char COLLECTION_IO_PATH[] = "/data/log/hiview/unified_collection/io/";
constexpr char HIVIEW_UCOLLECTION_STATE_TRUE[] = "true";
constexpr char HIVIEW_UCOLLECTION_STATE_FALSE[] = "false";
#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
constexpr char DEVELOP_TRACE_RECORDER_FALSE[] = "false";
constexpr char KEY_FREEZE_DETECTOR_STATE[] = "persist.hiview.freeze_detector";
constexpr char OTHER[] = "Other";
using namespace OHOS::HiviewDFX::Hitrace;
constexpr char UNIFIED_SPECIAL_PATH[] = "/data/log/hiview/unified_collection/trace/special/";
constexpr char UNIFIED_TELEMETRY_PATH[] = "/data/log/hiview/unified_collection/trace/telemetry/";
constexpr char UNIFIED_SHARE_TEMP_PATH[] = "/data/log/hiview/unified_collection/trace/share/temp/";
constexpr int32_t LOG_GID = 1007;

void CreateTracePathInner(const std::string &filePath)
{
    if (FileUtil::FileExists(filePath)) {
        return;
    }
    if (!FileUtil::CreateMultiDirectory(filePath)) {
        HIVIEW_LOGE("failed to create multidirectory %{public}s.", filePath.c_str());
        return;
    }
    if (chown(filePath.c_str(), -1, LOG_GID) != 0) {
        HIVIEW_LOGE("failed to change owner %{public}s.", filePath.c_str());
    }
}

void CreateTracePath()
{
    CreateTracePathInner(UNIFIED_SHARE_TEMP_PATH);
    CreateTracePathInner(UNIFIED_SPECIAL_PATH);
    CreateTracePathInner(UNIFIED_TELEMETRY_PATH);
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

#ifdef HAS_HIPERF
    std::string configPath = PerfCollectConfig::GetConfigPath();
    for (const auto& item : PerfCollectConfig::GetPerfCount(configPath)) {
        dprintf(fd, "perf caller : %s, concurrent counts : %d.\n",
            PerfCollectConfig::MapPerfCallerToString(item.first).c_str(), item.second);
    }
    dprintf(fd, "minimum memory allowed for perf collect : %d.\n",
        PerfCollectConfig::GetAllowMemory(configPath));
#endif
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
    telemetryListener_ = std::make_shared<TelemetryListener>();
    context->AddListenerInfo(Event::MessageType::TELEMETRY_EVENT, telemetryListener_->GetListenerName());
    context->RegisterUnorderedEventListener(telemetryListener_);
    TraceCollector::Create()->PrepareTrace();
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
    if (strncmp(value, HIVIEW_UCOLLECTION_STATE_TRUE, strlen(HIVIEW_UCOLLECTION_STATE_TRUE)) == 0) {
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
