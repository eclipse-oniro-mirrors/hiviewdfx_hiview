/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "app_caller_event.h"
#include "bundle_mgr_client.h"
#include "collect_event.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "hiview_platform.h"
#include "hiview_service_adapter.h"
#include "sys_event.h"
#include "string_util.h"
#include "time_util.h"
#include "trace_manager.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-Service");
namespace {
constexpr int MIN_SUPPORT_CMD_SIZE = 1;
constexpr int32_t ERR_DEFAULT = -1;
}

HiviewService::HiviewService()
{
    traceCollector_ = UCollectUtil::TraceCollector::Create();
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

    // hidumper hiviewdfx -d
    if ((cmds.size() == MIN_SUPPORT_CMD_SIZE) && (cmds[0] == "-d")) {
        DumpDetailedInfo(fd);
        return;
    }

    // hidumper hiviewdfx -p
    if ((cmds.size() >= MIN_SUPPORT_CMD_SIZE) && (cmds[0] == "-p")) {
        DumpPluginInfo(fd, cmds);
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

void HiviewService::DumpDetailedInfo(int fd)
{
    if (parser_ != nullptr) {
        parser_.reset();
    }
    DumpLoadedPluginInfo(fd);
    parser_ = std::make_unique<AuditLogParser>();
    parser_->StartParse();
    std::string timeScope = parser_->GetAuditLogTimeScope();
    dprintf(fd, "%s\n", timeScope.c_str());
    DumpPluginUsageInfo(fd);
    DumpThreadUsageInfo(fd);
    DumpPipelineUsageInfo(fd);
    parser_.reset();
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

void HiviewService::DumpPluginUsageInfo(int fd)
{
    auto &platform = HiviewPlatform::GetInstance();
    auto const &curPluginMap = platform.GetPluginMap();
    for (auto const &entry : curPluginMap) {
        auto pluginName = entry.first;
        if (entry.second != nullptr) {
            DumpPluginUsageInfo(fd, pluginName);
        }
    }
}

void HiviewService::DumpPluginUsageInfo(int fd, const std::string &pluginName) const
{
    if (parser_ == nullptr) {
        return;
    }
    auto logList = parser_->GetPluginSummary(pluginName);
    dprintf(fd, "Following events processed By Plugin %s:\n", pluginName.c_str());
    for (auto &log : logList) {
        dprintf(fd, " %s.\n", log.c_str());
    }
    dprintf(fd, "Dump Plugin Usage Done.\n\n");
}

void HiviewService::DumpThreadUsageInfo(int fd) const
{
    auto &platform = HiviewPlatform::GetInstance();
    auto const &curThreadMap = platform.GetWorkLoopMap();
    dprintf(fd, "Start Dump ThreadInfo:\n");
    for (auto const &entry : curThreadMap) {
        if (entry.second != nullptr) {
            std::string name = entry.second->GetName();
            DumpThreadUsageInfo(fd, name);
        }
    }
    dprintf(fd, "Dump ThreadInfo Done.\n\n");
}

void HiviewService::DumpThreadUsageInfo(int fd, const std::string &threadName) const
{
    if (parser_ == nullptr) {
        return;
    }
    auto logList = parser_->GetThreadSummary(threadName);
    dprintf(fd, "Following events processed on Thread %s:\n", threadName.c_str());
    for (auto &log : logList) {
        dprintf(fd, " %s.\n", log.c_str());
    }
}

void HiviewService::DumpPipelineUsageInfo(int fd) const
{
    auto &platform = HiviewPlatform::GetInstance();
    auto const &curPipelineMap = platform.GetPipelineMap();
    dprintf(fd, "Start Dump Pipeline Info:\n");
    for (auto const &entry : curPipelineMap) {
        auto pipeline = entry.first;
        DumpPipelineUsageInfo(fd, pipeline);
    }
}

void HiviewService::DumpPipelineUsageInfo(int fd, const std::string &pipelineName) const
{
    if (parser_ == nullptr) {
        return;
    }
    auto logList = parser_->GetPipelineSummary(pipelineName);
    dprintf(fd, "Following events processed on Pipeline %s:\n", pipelineName.c_str());
    for (auto &log : logList) {
        dprintf(fd, " %s.\n", log.c_str());
    }
    dprintf(fd, "Dump Pipeline Usage Info Done.\n\n");
}

void HiviewService::PrintUsage(int fd) const
{
    dprintf(fd, "Hiview Plugin Platform dump options:\n");
    dprintf(fd, "hidumper hiviewdfx [-d(etail)]\n");
    dprintf(fd, "    [-p(lugin) pluginName]\n");
}

int32_t HiviewService::CopyFile(const std::string& srcFilePath, const std::string& destFilePath)
{
    int srcFd = open(srcFilePath.c_str(), O_RDONLY);
    if (srcFd == -1) {
        HIVIEW_LOGE("failed to open source file, src=%{public}s", StringUtil::HideSnInfo(srcFilePath).c_str());
        return ERR_DEFAULT;
    }
    struct stat st{};
    if (fstat(srcFd, &st) == -1) {
        HIVIEW_LOGE("failed to stat file.");
        close(srcFd);
        return ERR_DEFAULT;
    }
    int destFd = open(destFilePath.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IROTH);
    if (destFd == -1) {
        HIVIEW_LOGE("failed to open destination file, des=%{public}s", StringUtil::HideSnInfo(destFilePath).c_str());
        close(srcFd);
        return ERR_DEFAULT;
    }
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
            close(srcFd);
            close(destFd);
            return ERR_DEFAULT;
        }
    }
    close(srcFd);
    close(destFd);
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
    HIVIEW_LOGI("move file, delete src result: %{public}d", result);
    if (!result) {
        bool destResult = FileUtil::RemoveFile(destFilePath);
        HIVIEW_LOGI("move file, delete dest result: %{public}d", destResult);
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
    TraceManager manager;
    int32_t openRet = manager.OpenSnapshotTrace(tagGroups);
    if (openRet != UCollect::UcError::SUCCESS) {
        HIVIEW_LOGW("failed to open trace in snapshort mode.");
    }
    CollectResult<int32_t> ret;
    ret.retCode = UCollect::UcError(openRet);
    return ret;
}

CollectResult<std::vector<std::string>> HiviewService::DumpSnapshotTrace(UCollect::TraceCaller caller)
{
    HIVIEW_LOGI("caller[%{public}d] dump trace in snapshot mode.", static_cast<int32_t>(caller));
    CollectResult<std::vector<std::string>> dumpRet = traceCollector_->DumpTrace(caller);
    if (dumpRet.retCode != UCollect::UcError::SUCCESS) {
        HIVIEW_LOGE("failed to dump the trace in snapshort mode.");
    }
    return dumpRet;
}

CollectResult<int32_t> HiviewService::OpenRecordingTrace(const std::string& tags)
{
    TraceManager manager;
    int32_t openRet = manager.OpenRecordingTrace(tags);
    if (openRet != UCollect::UcError::SUCCESS) {
        HIVIEW_LOGW("failed to open trace in recording mode.");
    }
    CollectResult<int32_t> ret;
    ret.retCode = UCollect::UcError(openRet);
    return ret;
}

CollectResult<int32_t> HiviewService::RecordingTraceOn()
{
    CollectResult<int32_t> traceOnRet = traceCollector_->TraceOn();
    if (traceOnRet.retCode != UCollect::UcError::SUCCESS) {
        HIVIEW_LOGE("failed to turn on the trace in recording mode.");
    }
    return traceOnRet;
}

CollectResult<std::vector<std::string>> HiviewService::RecordingTraceOff()
{
    CollectResult<std::vector<std::string>> traceOffRet = traceCollector_->TraceOff();
    if (traceOffRet.retCode != UCollect::UcError::SUCCESS) {
        HIVIEW_LOGE("failed to turn off the trace in recording mode.");
        return traceOffRet;
    }
    TraceManager manager;
    auto recoverRet = manager.RecoverTrace();
    if (recoverRet != UCollect::UcError::SUCCESS) {
        HIVIEW_LOGE("failed to recover the trace after trace off in recording mode.");
        traceOffRet.retCode = UCollect::UcError::UNSUPPORT;
    }
    return traceOffRet;
}

CollectResult<int32_t> HiviewService::CloseTrace()
{
    TraceManager manager;
    int32_t closeRet = manager.CloseTrace();
    if (closeRet != UCollect::UcError::SUCCESS) {
        HIVIEW_LOGW("failed to close the trace. errorCode=%{public}d", closeRet);
    }
    CollectResult<int32_t> ret;
    ret.retCode = UCollect::UcError(closeRet);
    return ret;
}

CollectResult<int32_t> HiviewService::RecoverTrace()
{
    TraceManager manager;
    int32_t recoverRet = manager.RecoverTrace();
    if (recoverRet != UCollect::UcError::SUCCESS) {
        HIVIEW_LOGW("failed to recover the trace.");
    }
    CollectResult<int32_t> ret;
    ret.retCode = UCollect::UcError(recoverRet);
    return ret;
}

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

static CollectResult<int32_t> InnerResponseAppTrace(UCollectClient::AppCaller &appCaller, const std::string &eventName)
{
    CollectResult<int32_t> result;
    result.data = 0;
    result.retCode = UCollect::UcError::SUCCESS;

    std::shared_ptr<Plugin> plugin = HiviewPlatform::GetInstance().GetPluginByName(UCollectUtil::UCOLLECTOR_PLUGIN);
    if (plugin == nullptr) {
        HIVIEW_LOGE("UnifiedCollector plugin does not exists, uid=%{public}d, pid=%{public}d",
            appCaller.uid, appCaller.pid);
        result.retCode = UCollect::UcError::SYSTEM_ERROR;
        return result;
    }

    std::shared_ptr<AppCallerEvent> appCallerEvent = InnerCreateAppCallerEvent(appCaller, eventName);
    std::shared_ptr<Event> event = std::dynamic_pointer_cast<Event>(appCallerEvent);
    if (!plugin->OnEvent(event)) {
        HIVIEW_LOGE("%{public}s failed for uid=%{public}d pid=%{public}d error code=%{public}d",
            eventName.c_str(), appCaller.uid, appCaller.pid, appCallerEvent->resultCode_);
        result.retCode = UCollect::UcError(appCallerEvent->resultCode_);
        return result;
    }
    return result;
}

static CollectResult<int32_t> InnerResponseStartAppTrace(UCollectClient::AppCaller &appCaller)
{
    return InnerResponseAppTrace(appCaller, UCollectUtil::START_APP_TRACE);
}

static CollectResult<int32_t> InnerResponseDumpAppTrace(UCollectClient::AppCaller &appCaller)
{
    return InnerResponseAppTrace(appCaller, UCollectUtil::DUMP_APP_TRACE);
}

CollectResult<int32_t> HiviewService::CaptureDurationTrace(UCollectClient::AppCaller &appCaller)
{
    CollectResult<int32_t> result;
    result.data = 0;
    if (!AppCallerEvent::enableDynamicTrace_) {
        HIVIEW_LOGE("disable dynamic trace, can not capture trace for uid=%{public}d, pid=%{public}d",
            appCaller.uid, appCaller.pid);
        result.retCode = UCollect::UcError::UNSUPPORT;
        return result;
    }

    if (appCaller.actionId == UCollectClient::ACTION_ID_START_TRACE) {
        return InnerResponseStartAppTrace(appCaller);
    } else if (appCaller.actionId == UCollectClient::ACTION_ID_DUMP_TRACE) {
        return InnerResponseDumpAppTrace(appCaller);
    } else {
        HIVIEW_LOGE("invalid param %{public}d, can not capture trace for uid=%{public}d, pid=%{public}d",
            appCaller.actionId, appCaller.uid, appCaller.pid);
        result.retCode = UCollect::UcError::INVALID_ACTION_ID;
        return result;
    }

    result.retCode = UCollect::UcError::SUCCESS;
    return result;
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
    CollectResult<int32_t> result;
    result.data = 0;
    result.retCode = UCollect::UcError::SUCCESS;

    std::shared_ptr<Plugin> plugin = HiviewPlatform::GetInstance().GetPluginByName("XPower");
    if (plugin == nullptr) {
        HIVIEW_LOGE("XPower plugin does not exists. pid=%{public}d", memoryCaller.pid);
        result.retCode = UCollect::UcError::SYSTEM_ERROR;
        return result;
    }

    std::string eventName = "APP_RESOURCE_LIMIT";
    SysEventCreator sysEventCreator("HIVIEWDFX", eventName, SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("PID", memoryCaller.pid);
    sysEventCreator.SetKeyValue("RESOURCE_TYPE", memoryCaller.resourceType);
    sysEventCreator.SetKeyValue("RESOURCE_LIMIT", memoryCaller.limitValue);
    sysEventCreator.SetKeyValue("RESOURCE_DEBUG_ENABLE", memoryCaller.enabledDebugLog ? "true" : "false");
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>(eventName, nullptr, sysEventCreator);
    std::shared_ptr<Event> event = std::dynamic_pointer_cast<Event>(sysEvent);
    if (!plugin->OnEvent(event)) {
        HIVIEW_LOGE("%{public}s failed for pid=%{public}d error", eventName.c_str(), memoryCaller.pid);
        result.retCode = UCollect::UcError::SYSTEM_ERROR;
        return result;
    }
    return result;
}

CollectResult<int32_t> HiviewService::GetGraphicUsage(int32_t pid)
{
    return graphicMemoryCollector_->GetGraphicUsage(pid);
}
}  // namespace HiviewDFX
}  // namespace OHOS
