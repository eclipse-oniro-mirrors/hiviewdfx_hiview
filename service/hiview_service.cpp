/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "hiview_service.h"

#include <cinttypes>
#include <cstdio>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

#include "bundle_mgr_client.h"
#include "collect_event.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "hiview_platform.h"
#include "hiview_service_adapter.h"
#include "sys_event.h"
#include "sys_event_dao.h"
#include "string_util.h"
#include "time_util.h"
#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
#include "trace_state_machine.h"
#include "trace_strategy.h"
#include "trace_strategy_factory.h"
#endif
namespace OHOS {
namespace HiviewDFX {
using namespace UCollect;
DEFINE_LOG_TAG("HiView-Service");
namespace {
constexpr int MIN_SUPPORT_CMD_SIZE = 1;
constexpr int32_t ERR_DEFAULT = -1;
}

HiviewService::HiviewService()
{
    cpuCollector_ = UCollectUtil::CpuCollector::Create();
    graphicMemoryCollector_ = UCollectUtil::GraphicMemoryCollector::Create();
}

void HiviewService::StartService()
{
    std::unique_ptr<HiviewServiceAdapter> adapter = std::make_unique<HiviewServiceAdapter>();
    adapter->StartService(this);
}

void HiviewService::DumpRequestDispatcher(int fd, const std::vector<std::string> &cmds)
{
    if (fd < 0) {
        HIVIEW_LOGW("invalid fd.");
        return;
    }

    if (cmds.size() == 0) {
        DumpLoadedPluginInfo(fd);
        return;
    }

    // hidumper hiviewdfx -p
    if ((cmds.size() >= MIN_SUPPORT_CMD_SIZE) && (cmds[0] == "-p")) {
        DumpPluginInfo(fd, cmds);
        return;
    }

    if ((cmds.size() >= MIN_SUPPORT_CMD_SIZE) && (cmds[0] == "--sysevent_backup")) {
        EventStore::SysEventDao::Backup();
        dprintf(fd, "System Events Backup Done.\n");
        return;
    }

    PrintUsage(fd);
    return;
}

void HiviewService::DumpPluginInfo(int fd, const std::vector<std::string> &cmds) const
{
    std::string pluginName = "";
    const int pluginNameSize = 2;
    const int pluginNamePos = 1;
    std::vector<std::string> newCmd;
    if (cmds.size() >= pluginNameSize) {
        pluginName = cmds[pluginNamePos];
        newCmd.insert(newCmd.begin(), cmds.begin() + pluginNamePos, cmds.end());
    }

    auto &platform = HiviewPlatform::GetInstance();
    auto const &curPluginMap = platform.GetPluginMap();
    for (auto const &entry : curPluginMap) {
        auto const &pluginPtr = entry.second;
        if (pluginPtr == nullptr) {
            continue;
        }

        if (pluginName.empty()) {
            pluginPtr->Dump(fd, newCmd);
            continue;
        }

        if (pluginPtr->GetName() == pluginName) {
            pluginPtr->Dump(fd, newCmd);
            break;
        }
    }
}

void HiviewService::DumpLoadedPluginInfo(int fd) const
{
    auto &platform = HiviewPlatform::GetInstance();
    auto const &curPluginMap = platform.GetPluginMap();
    dprintf(fd, "Current Loaded Plugins:\n");
    for (auto const &entry : curPluginMap) {
        auto const &pluginName = entry.first;
        if (entry.second != nullptr) {
            dprintf(fd, "PluginName:%s ", pluginName.c_str());
            dprintf(fd, "IsDynamic:%s ",
                (entry.second->GetType() == Plugin::PluginType::DYNAMIC) ? "True" : "False");
            dprintf(fd, "Version:%s ", (entry.second->GetVersion().c_str()));
            dprintf(fd,
                "ThreadName:%s\n",
                ((entry.second->GetWorkLoop() == nullptr) ? "Null" : entry.second->GetWorkLoop()->GetName().c_str()));
        }
    }
    dprintf(fd, "Dump Plugin Loaded Info Done.\n\n");
}

void HiviewService::PrintUsage(int fd) const
{
    dprintf(fd, "Hiview Plugin Platform dump options:\n");
    dprintf(fd, "hidumper hiviewdfx [-p(lugin) pluginName]\n");
    dprintf(fd, "hidumper hiviewdfx [--sysevent_backup]\n");
}

int32_t HiviewService::CopyFile(const std::string& srcFilePath, const std::string& destFilePath)
{
    int srcFd = open(srcFilePath.c_str(), O_RDONLY);
    if (srcFd == -1) {
        HIVIEW_LOGE("failed to open source file");
        return ERR_DEFAULT;
    }
    fdsan_exchange_owner_tag(srcFd, 0, logLabelDomain);
    struct stat st{};
    if (fstat(srcFd, &st) == -1) {
        HIVIEW_LOGE("failed to stat file.");
        fdsan_close_with_tag(srcFd, logLabelDomain);
        return ERR_DEFAULT;
    }
    int destFd = open(destFilePath.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IROTH);
    if (destFd == -1) {
        HIVIEW_LOGE("failed to open destination file");
        fdsan_close_with_tag(srcFd, logLabelDomain);
        return ERR_DEFAULT;
    }
    fdsan_exchange_owner_tag(destFd, 0, logLabelDomain);
    off_t offset = 0;
    int cycleNum = 0;
    while (offset < st.st_size) {
        size_t count = static_cast<size_t>((st.st_size - offset) > SSIZE_MAX ? SSIZE_MAX : st.st_size - offset);
        ssize_t ret = sendfile(destFd, srcFd, &offset, count);
        if (cycleNum > 0) {
            HIVIEW_LOGI("sendfile cycle num:%{public}d, ret:%{public}zd, offset:%{public}lld, size:%{public}lld",
                cycleNum, ret, static_cast<long long>(offset), static_cast<long long>(st.st_size));
        }
        cycleNum++;
        if (ret < 0 || offset > st.st_size) {
            HIVIEW_LOGE("sendfile fail, ret:%{public}zd, offset:%{public}lld, size:%{public}lld",
                ret, static_cast<long long>(offset), static_cast<long long>(st.st_size));
            fdsan_close_with_tag(srcFd, logLabelDomain);
            fdsan_close_with_tag(destFd, logLabelDomain);
            return ERR_DEFAULT;
        }
    }
    fdsan_close_with_tag(srcFd, logLabelDomain);
    fdsan_close_with_tag(destFd, logLabelDomain);
    return 0;
}

int32_t HiviewService::Copy(const std::string& srcFilePath, const std::string& destFilePath)
{
    return CopyFile(srcFilePath, destFilePath);
}

int32_t HiviewService::Move(const std::string& srcFilePath, const std::string& destFilePath)
{
    int copyResult = CopyFile(srcFilePath, destFilePath);
    if (copyResult != 0) {
        HIVIEW_LOGW("copy file failed, result: %{public}d", copyResult);
        return copyResult;
    }
    bool result = FileUtil::RemoveFile(srcFilePath);
    if (!result) {
        bool destResult = FileUtil::RemoveFile(destFilePath);
        HIVIEW_LOGI("failed to delete src, delete dest result: %{public}d", destResult);
        return ERR_DEFAULT;
    }
    return 0;
}

int32_t HiviewService::Remove(const std::string& filePath)
{
    bool result = FileUtil::RemoveFile(filePath);
    HIVIEW_LOGI("remove file, result:%{public}d", result);
    return 0;
}

CollectResult<int32_t> HiviewService::OpenSnapshotTrace(const std::vector<std::string>& tagGroups)
{
#ifndef UNIFIED_COLLECTOR_TRACE_ENABLE
    return {UcError::FEATURE_CLOSED};
#else
    TraceRet openRet = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMAND, tagGroups);
    return {GetUcError(openRet)};
#endif
}

CollectResult<std::vector<std::string>> HiviewService::DumpSnapshotTrace(const std::string& callerName,
    bool isNeedFlowControl)
{
#ifndef UNIFIED_COLLECTOR_TRACE_ENABLE
    return {UcError::FEATURE_CLOSED};
#else
    return UCollectUtil::TraceCollector::Create()->DumpTrace(callerName, isNeedFlowControl);
#endif
}

CollectResult<int32_t> HiviewService::OpenRecordingTrace(const std::string& tags)
{
#ifndef UNIFIED_COLLECTOR_TRACE_ENABLE
    return {UcError::FEATURE_CLOSED};
#else
    TraceRet openRet = TraceStateMachine::GetInstance().OpenTrace(TraceScenario::TRACE_COMMAND, tags);
    return {GetUcError(openRet)};
#endif
}

CollectResult<int32_t> HiviewService::RecordingTraceOn()
{
#ifndef UNIFIED_COLLECTOR_TRACE_ENABLE
    return {UcError::FEATURE_CLOSED};
#else
    TraceRet ret = TraceStateMachine::GetInstance().TraceDropOn(TraceScenario::TRACE_COMMAND);
    return {GetUcError(ret)};
#endif
}

CollectResult<std::vector<std::string>> HiviewService::RecordingTraceOff()
{
#ifndef UNIFIED_COLLECTOR_TRACE_ENABLE
    return {UcError::FEATURE_CLOSED};
#else
    TraceRetInfo traceRetInfo;
    TraceRet ret = TraceStateMachine::GetInstance().TraceDropOff(TraceScenario::TRACE_COMMAND, traceRetInfo);
    CollectResult<std::vector<std::string>> result(GetUcError(ret));
    result.data = traceRetInfo.outputFiles;
    return result;
#endif
}

CollectResult<int32_t> HiviewService::CloseTrace()
{
#ifndef UNIFIED_COLLECTOR_TRACE_ENABLE
    return {UcError::FEATURE_CLOSED};
#else
    TraceRet ret = TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_COMMAND);
    return {GetUcError(ret)};
#endif
}

#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
static std::shared_ptr<AppCallerEvent> InnerCreateAppCallerEvent(UCollectClient::AppCaller &appCaller,
    const std::string &eventName)
{
    std::shared_ptr<AppCallerEvent> appCallerEvent = std::make_shared<AppCallerEvent>("HiViewService");
    appCallerEvent->messageType_ = Event::MessageType::PLUGIN_MAINTENANCE;
    appCallerEvent->eventName_ = eventName;
    appCallerEvent->isBusinessJank_ = appCaller.isBusinessJank;
    appCallerEvent->bundleName_ = appCaller.bundleName;
    appCallerEvent->bundleVersion_ = appCaller.bundleVersion;
    appCallerEvent->uid_ = appCaller.uid;
    appCallerEvent->pid_ = appCaller.pid;
    appCallerEvent->happenTime_ = static_cast<uint64_t>(appCaller.happenTime);
    appCallerEvent->beginTime_ = appCaller.beginTime;
    appCallerEvent->endTime_ = appCaller.endTime;
    appCallerEvent->taskBeginTime_ = static_cast<int64_t>(TimeUtil::GetMilliseconds());
    appCallerEvent->taskEndTime_ = appCallerEvent->taskBeginTime_;
    appCallerEvent->resultCode_ = UCollect::UcError::SUCCESS;
    appCallerEvent->foreground_ = appCaller.foreground;
    appCallerEvent->threadName_ = appCaller.threadName;
    return appCallerEvent;
}

bool HiviewService::InnerHasCallAppTrace(std::shared_ptr<AppCallerEvent> appCallerEvent)
{
    return TraceFlowController(FlowControlName::APP).HasCallOnceToday(appCallerEvent->uid_,
        appCallerEvent->happenTime_);
}

CollectResult<int32_t> HiviewService::InnerResponseStartAppTrace(UCollectClient::AppCaller &appCaller)
{
    auto appCallerEvent = InnerCreateAppCallerEvent(appCaller, UCollectUtil::START_APP_TRACE);
    if (InnerHasCallAppTrace(appCallerEvent)) {
        HIVIEW_LOGW("deny: already capture trace uid=%{public}d pid=%{public}d", appCallerEvent->uid_,
            appCallerEvent->pid_);
        return {UCollect::UcError::HAD_CAPTURED_TRACE};
    }
    TraceRet ret = TraceStateMachine::GetInstance().OpenDynamicTrace(appCallerEvent->pid_);
    if (!ret.IsSuccess()) {
        return {GetUcError(ret)};
    }
    HiviewPlatform::GetInstance().PostAsyncEventToTarget(nullptr, UCollectUtil::UCOLLECTOR_PLUGIN, appCallerEvent);
    return {UCollect::UcError::SUCCESS};
}

CollectResult<int32_t> HiviewService::InnerResponseDumpAppTrace(UCollectClient::AppCaller &appCaller)
{
    return UCollectUtil::TraceCollector::Create()->DumpAppTrace(InnerCreateAppCallerEvent(appCaller,
        UCollectUtil::DUMP_APP_TRACE));
}
#endif

CollectResult<int32_t> HiviewService::CaptureDurationTrace(UCollectClient::AppCaller &appCaller)
{
#ifndef UNIFIED_COLLECTOR_TRACE_ENABLE
    return {UcError::FEATURE_CLOSED};
#else
    if (appCaller.actionId == UCollectClient::ACTION_ID_START_TRACE) {
        return InnerResponseStartAppTrace(appCaller);
    } else if (appCaller.actionId == UCollectClient::ACTION_ID_DUMP_TRACE) {
        return InnerResponseDumpAppTrace(appCaller);
    } else {
        HIVIEW_LOGE("invalid param %{public}d, can not capture trace for uid=%{public}d, pid=%{public}d",
            appCaller.actionId, appCaller.uid, appCaller.pid);
        return {UCollect::UcError::INVALID_ACTION_ID};
    }
#endif
}


CollectResult<double> HiviewService::GetSysCpuUsage()
{
    CollectResult<double> cpuUsageRet = cpuCollector_->GetSysCpuUsage();
    if (cpuUsageRet.retCode != UCollect::UcError::SUCCESS) {
        HIVIEW_LOGE("failed to collect system cpu usage");
    }
    return cpuUsageRet;
}

CollectResult<int32_t> HiviewService::SetAppResourceLimit(UCollectClient::MemoryCaller& memoryCaller)
{
    std::string eventName = "APP_RESOURCE_LIMIT";
    SysEventCreator sysEventCreator("HIVIEWDFX", eventName, SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("PID", memoryCaller.pid);
    sysEventCreator.SetKeyValue("RESOURCE_TYPE", memoryCaller.resourceType);
    sysEventCreator.SetKeyValue("RESOURCE_LIMIT", memoryCaller.limitValue);
    sysEventCreator.SetKeyValue("RESOURCE_DEBUG_ENABLE", memoryCaller.enabledDebugLog ? "true" : "false");
    auto sysEvent = std::make_shared<SysEvent>(eventName, nullptr, sysEventCreator);
    std::shared_ptr<Event> event = std::dynamic_pointer_cast<Event>(sysEvent);
    HiviewPlatform::GetInstance().PostAsyncEventToTarget(nullptr, "XPower", event);
    return {UCollect::UcError::SUCCESS};
}

CollectResult<int32_t> HiviewService::SetSplitMemoryValue(std::vector<UCollectClient::MemoryCaller>& memList)
{
    std::vector<int32_t> pidList;
    std::vector<int32_t> resourceList;
    if (memList.size() < 1) {
        return {UCollect::UcError::SYSTEM_ERROR};
    }
    for (auto it : memList) {
        pidList.push_back(it.pid);
        resourceList.push_back(it.limitValue);
    }
    std::string eventName = memList[0].resourceType;
    SysEventCreator sysEventCreator("HIVIEWDFX", eventName, SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("PID_LIST", pidList);
    sysEventCreator.SetKeyValue("RESOURCE_LIST", resourceList);

    auto sysEvent = std::make_shared<SysEvent>(eventName, nullptr, sysEventCreator);
    auto event = std::dynamic_pointer_cast<Event>(sysEvent);
    HiviewPlatform::GetInstance().PostAsyncEventToTarget(nullptr, "XPower", event);
    return {UCollect::UcError::SUCCESS};
}

CollectResult<UCollectClient::GraphicUsage> HiviewService::GetGraphicUsage(int32_t pid)
{
    CollectResult<UCollectClient::GraphicUsage> result;
    result.retCode = UCollect::UcError::SUCCESS;
    CollectResult<int32_t> glRet = graphicMemoryCollector_->GetGraphicUsage(pid, GraphicType::GL, true);
    if (glRet.retCode != UCollect::UcError::SUCCESS) {
        result.retCode = glRet.retCode;
        return result;
    }
    CollectResult<int32_t> graphRet = graphicMemoryCollector_->GetGraphicUsage(pid, GraphicType::GRAPH, true);
    if (graphRet.retCode != UCollect::UcError::SUCCESS) {
        result.retCode = graphRet.retCode;
        return result;
    }
    result.data.gl = glRet.data;
    result.data.graph = graphRet.data;
    return result;
}
}  // namespace HiviewDFX
}  // namespace OHOS
