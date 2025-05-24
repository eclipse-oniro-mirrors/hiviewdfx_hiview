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

#include <memory>

#include "app_caller_event.h"
#include "app_trace_context.h"
#include "collect_event.h"
#include "event_publish.h"
#include "ffrt.h"
#include "file_util.h"
#include "hitrace_dump.h"
#include "hiview_logger.h"
#include "io_collector.h"
#include "parameter_ex.h"
#include "plugin_factory.h"
#include "process_status.h"
#include "sys_event.h"
#include "time_util.h"
#include "trace_flow_controller.h"
#include "trace_manager.h"
#include "uc_observer_mgr.h"
#include "unified_collection_stat.h"

namespace OHOS {
namespace HiviewDFX {
REGISTER(UnifiedCollector);
DEFINE_LOG_TAG("HiView-UnifiedCollector");
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace std::literals::chrono_literals;
using namespace OHOS::HiviewDFX::Hitrace;
namespace {
std::mutex g_traceLock;
const std::string HIPERF_LOG_PATH = "/data/log/hiperf";
const std::string COLLECTION_IO_PATH = "/data/log/hiview/unified_collection/io/";
const std::string UNIFIED_SPECIAL_PATH = "/data/log/hiview/unified_collection/trace/special/";
const std::string DEVELOPER_MODE_TRACE_ARGS = "tags:sched, freq, disk, sync, binder, mmc, membus, load, pagecache, \
    workq, ipa, hdf, virse, net, dsched, graphic, multimodalinput, dinput, ark, ace, window, zaudio, daudio, zmedia, \
    dcamera, zcamera, dhfwk, app, gresource, ability, power, samgr, ffrt clockType:boot1 bufferSize:32768 overwrite:0 \
    fileLimit:20 fileSize:102400";
const std::string OTHER = "Other";
const std::string HIVIEW_UCOLLECTION_STATE_TRUE = "true";
const std::string HIVIEW_UCOLLECTION_STATE_FALSE = "false";
const std::string DEVELOP_TRACE_RECORDER_TRUE = "true";
const std::string DEVELOP_TRACE_RECORDER_FALSE = "false";
const std::string HIVIEW_UCOLLECTION_TEST_APP_TRACE_STATE_TRUE = "true";

const int8_t STATE_COUNT = 2;
const int8_t COML_STATE = 0;
// [coml:0][is remote log][is trace recorder]
// [beta:1][is remote log][is trace recorder]
const bool DYNAMIC_TRACE_FSM[STATE_COUNT][STATE_COUNT][STATE_COUNT] = {
    {{true,  false}, {false, false}},
    {{false, false}, {false, false}},
};

// [dev mode:0][test app trace state]
// [dev mode:1][test app trace state]
const bool CHECK_DYNAMIC_TRACE_FSM[STATE_COUNT][STATE_COUNT] = {
    {true, true}, {false, true}
};

#if PC_APP_STATE_COLLECT_ENABLE
const std::string RSS_APP_STATE_EVENT = "APP_CGROUP_CHANGE";
const int NAP_BACKGROUND_GROUP = 11;

ProcessState GetProcessStateByGroup(SysEvent& sysEvent)
{
    if (sysEvent.GetEventName() != RSS_APP_STATE_EVENT) {
        return INVALID;
    }
    int32_t procGroup = sysEvent.GetEventIntValue("PROCESS_NEWGROUP");
    if (procGroup == NAP_BACKGROUND_GROUP) {
        return BACKGROUND;
    }
    // else - The app is in the foreground group
    return FOREGROUND;
}
#endif // PC_APP_STATE_COLLECT_ENABLE

void OnTestAppTraceStateChanged(const char* key, const char* value, void* context)
{
    if (key == nullptr || value == nullptr) {
        return;
    }

    if (!(std::string(HIVIEW_UCOLLECTION_TEST_APP_TRACE_STATE) == key)) {
        return;
    }

    bool isBetaVersion = Parameter::IsBetaVersion();
    bool isUCollectionSwitchOn = Parameter::IsUCollectionSwitchOn();
    bool isTraceCollectionSwitchOn = Parameter::IsTraceCollectionSwitchOn();

    bool isDeveloperMode = Parameter::IsDeveloperMode();
    bool isTraceStateActive = std::string(HIVIEW_UCOLLECTION_TEST_APP_TRACE_STATE_TRUE) == value;
    AppCallerEvent::enableDynamicTrace_ = CHECK_DYNAMIC_TRACE_FSM[isDeveloperMode][isTraceStateActive] &&
        DYNAMIC_TRACE_FSM[isBetaVersion][isUCollectionSwitchOn][isTraceCollectionSwitchOn];
    HIVIEW_LOGI("dynamic trace change to:%{public}d as test trace state", AppCallerEvent::enableDynamicTrace_);
}

void InitDynamicTrace()
{
    bool isBetaVersion = Parameter::IsBetaVersion();
    bool isUCollectionSwitchOn = Parameter::IsUCollectionSwitchOn();
    bool isTraceCollectionSwitchOn = Parameter::IsTraceCollectionSwitchOn();
    HIVIEW_LOGI("IsBetaVersion=%{public}d, IsUCollectionSwitchOn=%{public}d, IsTraceCollectionSwitchOn=%{public}d",
        isBetaVersion, isUCollectionSwitchOn, isTraceCollectionSwitchOn);

    bool isDeveloperMode = Parameter::IsDeveloperMode();
    bool isTestAppTraceOn = Parameter::IsTestAppTraceOn();
    HIVIEW_LOGI("IsDeveloperMode=%{public}d, IsTestAppTraceOn=%{public}d", isDeveloperMode, isTestAppTraceOn);
    AppCallerEvent::enableDynamicTrace_ = CHECK_DYNAMIC_TRACE_FSM[isDeveloperMode][isTestAppTraceOn] &&
        DYNAMIC_TRACE_FSM[isBetaVersion][isUCollectionSwitchOn][isTraceCollectionSwitchOn];
    HIVIEW_LOGI("dynamic trace open:%{public}d", AppCallerEvent::enableDynamicTrace_);

    int ret = Parameter::WatchParamChange(HIVIEW_UCOLLECTION_TEST_APP_TRACE_STATE, OnTestAppTraceStateChanged, nullptr);
    HIVIEW_LOGI("add ucollection test app trace param watcher ret: %{public}d", ret);
}

void OnHiViewTraceRecorderChanged(const char* key, const char* value, void* context)
{
    if (key == nullptr || value == nullptr) {
        return;
    }

    if (!(std::string(DEVELOP_HIVIEW_TRACE_RECORDER) == key)) {
        return;
    }

    bool isBetaVersion = Parameter::IsBetaVersion();
    bool isUCollectionSwitchOn = Parameter::IsUCollectionSwitchOn();
    bool isTraceCollectionSwitchOn = std::string(DEVELOP_TRACE_RECORDER_TRUE) == value;

    bool isDeveloperMode = Parameter::IsDeveloperMode();
    bool isTestAppTraceOn = Parameter::IsTestAppTraceOn();
    AppCallerEvent::enableDynamicTrace_ = CHECK_DYNAMIC_TRACE_FSM[isDeveloperMode][isTestAppTraceOn] &&
        DYNAMIC_TRACE_FSM[isBetaVersion][isUCollectionSwitchOn][isTraceCollectionSwitchOn];
    HIVIEW_LOGI("dynamic trace change to:%{public}d as trace recorder state", AppCallerEvent::enableDynamicTrace_);
}

void OnSwitchRecordTraceStateChanged(const char* key, const char* value, void* context)
{
    OnHiViewTraceRecorderChanged(key, value, context);
    HIVIEW_LOGI("record trace state changed, ret: %{public}s", value);
    if (key == nullptr || value == nullptr) {
        HIVIEW_LOGE("record trace switch input ptr null");
        return;
    }
    if (strncmp(key, DEVELOP_HIVIEW_TRACE_RECORDER, strlen(DEVELOP_HIVIEW_TRACE_RECORDER)) != 0) {
        HIVIEW_LOGE("record trace switch param key error");
        return;
    }

    if (UnifiedCollector::IsEnableRecordTrace() == false && strncmp(value, "true", strlen("true")) == 0) {
        UnifiedCollector::SetRecordTraceStatus(true);
        TraceManager traceManager;
        int32_t resultOpenTrace = traceManager.OpenRecordingTrace(DEVELOPER_MODE_TRACE_ARGS);
        if (resultOpenTrace != 0) {
            HIVIEW_LOGE("failed to start trace service");
        }

        std::shared_ptr<UCollectUtil::TraceCollector> traceCollector = UCollectUtil::TraceCollector::Create();
        CollectResult<int32_t> resultTraceOn = traceCollector->TraceOn();
        if (resultTraceOn.retCode != UCollect::UcError::SUCCESS) {
            HIVIEW_LOGE("failed to start collection trace");
        }
    } else if (UnifiedCollector::IsEnableRecordTrace() == true && strncmp(value, "false", strlen("false")) == 0) {
        UnifiedCollector::SetRecordTraceStatus(false);
        std::shared_ptr<UCollectUtil::TraceCollector> traceCollector = UCollectUtil::TraceCollector::Create();
        CollectResult<std::vector<std::string>> resultTraceOff = traceCollector->TraceOff();
        if (resultTraceOff.retCode != UCollect::UcError::SUCCESS) {
            HIVIEW_LOGE("failed to stop collection trace");
        }
        TraceManager traceManager;
        int32_t resultCloseTrace = traceManager.CloseTrace();
        if (resultCloseTrace != 0) {
            HIVIEW_LOGE("failed to stop trace service");
        }
    }
}
}

bool UnifiedCollector::isEnableRecordTrace_ = false;

void UnifiedCollector::OnLoad()
{
    HIVIEW_LOGI("start to load UnifiedCollector plugin");
    TraceManager::RecoverTmpTrace();
    Init();
}

void UnifiedCollector::OnUnload()
{
    HIVIEW_LOGI("start to unload UnifiedCollector plugin");
    UcObserverManager::GetInstance().UnregisterObservers();
}

bool UnifiedCollector::OnStartAppTrace(std::shared_ptr<AppCallerEvent> appCallerEvent)
{
    auto nextState = std::make_shared<StartTraceState>(appTraceContext_, appCallerEvent, shared_from_this());
    return appTraceContext_->TransferTo(nextState) == 0;
}

bool UnifiedCollector::OnDumpAppTrace(std::shared_ptr<AppCallerEvent> appCallerEvent)
{
    auto nextState = std::make_shared<DumpTraceState>(appTraceContext_, appCallerEvent, shared_from_this());
    return appTraceContext_->TransferTo(nextState) == 0;
}

bool UnifiedCollector::OnStopAppTrace(std::shared_ptr<AppCallerEvent> appCallerEvent)
{
    auto nextState = std::make_shared<StopTraceState>(appTraceContext_, appCallerEvent);
    return appTraceContext_->TransferTo(nextState) == 0;
}

bool UnifiedCollector::OnEvent(std::shared_ptr<Event>& event)
{
    if (event == nullptr) {
        return true;
    }

    HIVIEW_LOGI("Receive Event %{public}s", event->GetEventName().c_str());
    if (event->messageType_ == Event::MessageType::PLUGIN_MAINTENANCE) {
        std::shared_ptr<AppCallerEvent> appCallerEvent = Event::DownCastTo<AppCallerEvent>(event);
        if (event->eventName_ == UCollectUtil::START_APP_TRACE) {
            return OnStartAppTrace(appCallerEvent);
        }

        if (event->eventName_ == UCollectUtil::DUMP_APP_TRACE) {
            return OnDumpAppTrace(appCallerEvent);
        }

        if (event->eventName_ == UCollectUtil::STOP_APP_TRACE) {
            return OnStopAppTrace(appCallerEvent);
        }
    }

    return true;
}

void UnifiedCollector::OnMainThreadJank(SysEvent& sysEvent)
{
    appTraceContext_->PublishStackEvent(sysEvent);
}

void UnifiedCollector::OnEventListeningCallback(const Event& event)
{
    SysEvent& sysEvent = static_cast<SysEvent&>(const_cast<Event&>(event));
    HIVIEW_LOGI("sysevent %{public}s", sysEvent.eventName_.c_str());

    if (sysEvent.eventName_ == UCollectUtil::MAIN_THREAD_JANK) {
        OnMainThreadJank(sysEvent);
        return;
    }
#if PC_APP_STATE_COLLECT_ENABLE
    int32_t procId = sysEvent.GetEventIntValue("APP_PID");
    if (procId <= 0) {
        HIVIEW_LOGW("invalid process id=%{public}d", procId);
        return;
    }
    ProcessState procState = GetProcessStateByGroup(sysEvent);
    if (procState == INVALID) {
        HIVIEW_LOGD("invalid process state=%{public}d", procState);
        return;
    }
    ProcessStatus::GetInstance().NotifyProcessState(procId, procState);
#endif // PC_APP_STATE_COLLECT_ENABLE
}

void UnifiedCollector::Dump(int fd, const std::vector<std::string>& cmds)
{
    dprintf(fd, "device beta state is %s.\n", Parameter::IsBetaVersion() ? "beta" : "is not beta");

    std::string remoteLogState = Parameter::GetString(HIVIEW_UCOLLECTION_STATE, HIVIEW_UCOLLECTION_STATE_FALSE);
    dprintf(fd, "remote log state is %s.\n", remoteLogState.c_str());

    std::string traceRecorderState = Parameter::GetString(DEVELOP_HIVIEW_TRACE_RECORDER, DEVELOP_TRACE_RECORDER_FALSE);
    dprintf(fd, "trace recorder state is %s.\n", traceRecorderState.c_str());

    dprintf(fd, "develop state is %s.\n", Parameter::IsDeveloperMode() ? "true" : "false");
    dprintf(fd, "test app trace state is %s.\n", Parameter::IsTestAppTraceOn() ? "true" : "false");

    dprintf(fd, "dynamic trace state is %s.\n", AppCallerEvent::enableDynamicTrace_ ? "true" : "false");

    TraceManager traceManager;
    dprintf(fd, "trace mode is %d.\n", traceManager.GetTraceMode());
}

void UnifiedCollector::Init()
{
    if (GetHiviewContext() == nullptr) {
        HIVIEW_LOGE("hiview context is null");
        return;
    }

    appTraceContext_ = std::make_shared<AppTraceContext>(std::make_shared<StopTraceState>(nullptr, nullptr));

    InitWorkLoop();
    InitWorkPath();
    bool isAllowCollect = Parameter::IsBetaVersion() || Parameter::IsUCollectionSwitchOn();
    if (isAllowCollect) {
        RunIoCollectionTask();
        RunUCollectionStatTask();
        LoadHitraceService();
    }
    if (isAllowCollect || Parameter::IsDeveloperMode()) {
        RunCpuCollectionTask();
    }
    if (!Parameter::IsBetaVersion()) {
        int ret = Parameter::WatchParamChange(HIVIEW_UCOLLECTION_STATE, OnSwitchStateChanged, this);
        HIVIEW_LOGI("add ucollection switch param watcher ret: %{public}d", ret);
    }
    UcObserverManager::GetInstance().RegisterObservers();

    InitDynamicTrace();

    if (Parameter::IsDeveloperMode()) {
        RunRecordTraceTask();
    }
}

void UnifiedCollector::CleanDataFiles()
{
    std::vector<std::string> files;
    FileUtil::GetDirFiles(UNIFIED_SPECIAL_PATH, files);
    for (const auto& file : files) {
        if (file.find(OTHER) != std::string::npos) {
            FileUtil::RemoveFile(file);
        }
    }
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
    UnifiedCollector* unifiedCollectorPtr = static_cast<UnifiedCollector*>(context);
    if (unifiedCollectorPtr == nullptr) {
        HIVIEW_LOGE("unifiedCollectorPtr is null");
        return;
    }

    bool isUCollectionSwitchOn;
    if (value == HIVIEW_UCOLLECTION_STATE_TRUE) {
        isUCollectionSwitchOn = true;
        unifiedCollectorPtr->RunCpuCollectionTask();
        unifiedCollectorPtr->RunIoCollectionTask();
        unifiedCollectorPtr->RunUCollectionStatTask();
        unifiedCollectorPtr->LoadHitraceService();
    } else {
        isUCollectionSwitchOn = false;
        if (!Parameter::IsDeveloperMode()) {
            unifiedCollectorPtr->isCpuTaskRunning_ = false;
        }
        for (const auto &it : unifiedCollectorPtr->taskList_) {
            unifiedCollectorPtr->workLoop_->RemoveEvent(it);
        }
        unifiedCollectorPtr->taskList_.clear();
        unifiedCollectorPtr->ExitHitraceService();
        unifiedCollectorPtr->CleanDataFiles();
    }

    bool isTraceCollectionSwitchOn = Parameter::IsTraceCollectionSwitchOn();

    bool isDeveloperMode = Parameter::IsDeveloperMode();
    bool isTestAppTraceOn = Parameter::IsTestAppTraceOn();
    AppCallerEvent::enableDynamicTrace_ = CHECK_DYNAMIC_TRACE_FSM[isDeveloperMode][isTestAppTraceOn] &&
        DYNAMIC_TRACE_FSM[COML_STATE][isUCollectionSwitchOn][isTraceCollectionSwitchOn];
}

void UnifiedCollector::LoadHitraceService()
{
    std::lock_guard<std::mutex> lock(g_traceLock);
    HIVIEW_LOGI("start to load hitrace service.");
    uint8_t mode = GetTraceMode();
    if (mode != TraceMode::CLOSE) {
        HIVIEW_LOGE("service is running, mode=%{public}u.", mode);
        return;
    }
    const std::vector<std::string> tagGroups = {"scene_performance"};
    TraceErrorCode ret = OpenTrace(tagGroups);
    if (ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("OpenTrace fail.");
    }
}

void UnifiedCollector::ExitHitraceService()
{
    std::lock_guard<std::mutex> lock(g_traceLock);
    HIVIEW_LOGI("exit hitrace service.");
    uint8_t mode = GetTraceMode();
    if (mode == TraceMode::CLOSE) {
        HIVIEW_LOGE("service is close mode.");
        return;
    }
    CloseTrace();
}

void UnifiedCollector::InitWorkLoop()
{
    workLoop_ = GetHiviewContext()->GetSharedWorkLoop();
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

void UnifiedCollector::RunRecordTraceTask()
{
    if (IsEnableRecordTrace() == false && Parameter::IsTraceCollectionSwitchOn()) {
        SetRecordTraceStatus(true);
        TraceManager traceManager;
        int32_t resultOpenTrace = traceManager.OpenRecordingTrace(DEVELOPER_MODE_TRACE_ARGS);
        if (resultOpenTrace != 0) {
            HIVIEW_LOGE("failed to start trace service");
        }

        std::shared_ptr<UCollectUtil::TraceCollector> traceCollector = UCollectUtil::TraceCollector::Create();
        CollectResult<int32_t> resultTraceOn = traceCollector->TraceOn();
        if (resultTraceOn.retCode != UCollect::UcError::SUCCESS) {
            HIVIEW_LOGE("failed to start collection trace");
        }
    }
    int ret = Parameter::WatchParamChange(DEVELOP_HIVIEW_TRACE_RECORDER, OnSwitchRecordTraceStateChanged, this);
    HIVIEW_LOGI("add ucollection trace switch param watcher ret: %{public}d", ret);
}
} // namespace HiviewDFX
} // namespace OHOS
