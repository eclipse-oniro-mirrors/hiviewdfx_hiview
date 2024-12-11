/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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
#include "peer_binder_catcher.h"

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <securec.h>
#include <sys/wait.h>

#include "common_utils.h"
#include "file_util.h"
#include "ffrt.h"
#include "freeze_json_util.h"
#include "hiview_logger.h"
#include "log_catcher_utils.h"
#include "open_stacktrace_catcher.h"
#include "parameter_ex.h"
#include "perf_collector.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
    static constexpr uint8_t ARR_SIZE = 7;
    static constexpr uint8_t DECIMAL = 10;
    static constexpr const char* const LOGGER_EVENT_PEERBINDER = "PeerBinder";
    enum {
        LOGGER_BINDER_STACK_ONE = 0,
        LOGGER_BINDER_STACK_ALL = 1,
    };
}
DEFINE_LOG_LABEL(0xD002D01, "EventLogger-PeerBinderCatcher");
#ifdef HAS_HIPERF
using namespace OHOS::HiviewDFX::UCollectUtil;
constexpr char EVENT_LOG_PATH[] = "/data/log/eventlog";
#endif
PeerBinderCatcher::PeerBinderCatcher() : EventLogCatcher()
{
    name_ = "PeerBinderCatcher";
}

bool PeerBinderCatcher::Initialize(const std::string& perfCmd, int layer, int pid)
{
    pid_ = pid;
    layer_ = layer;
    perfCmd_ = perfCmd;
    char buf[BUF_SIZE_512] = {0};
    int ret = snprintf_s(buf, BUF_SIZE_512, BUF_SIZE_512 - 1,
        "PeerBinderCatcher -- pid==%d layer_ == %d\n", pid_, layer_);
    if (ret > 0) {
        description_ = buf;
    }
    return true;
}

void PeerBinderCatcher::Init(std::shared_ptr<SysEvent> event, const std::string& filePath, std::set<int>& catchedPids)
{
    event_ = event;
    catchedPids_ = catchedPids;
    if ((filePath != "") && FileUtil::FileExists(filePath)) {
        binderPath_ = filePath;
    }
}

int PeerBinderCatcher::Catch(int fd, int jsonFd)
{
    if (pid_ <= 0) {
        return -1;
    }

    auto originSize = GetFdSize(fd);

    if (!FileUtil::FileExists(binderPath_)) {
        std::string content = binderPath_ + " : file isn't exists\r\n";
        FileUtil::SaveStringToFd(fd, content);
        return -1;
    }

    std::set<int> pids = GetBinderPeerPids(fd, jsonFd);
    if (pids.empty()) {
        std::string content = "PeerBinder pids is empty\r\n";
        FileUtil::SaveStringToFd(fd, content);
    }
#ifdef HAS_HIPERF
    ForkToDumpHiperf(pids);
#endif
    std::string pidStr = "";
    for (auto pidTemp : pids) {
        if (pidTemp == pid_ || IsAncoProc(pidTemp)) {
            HIVIEW_LOGI("Stack of pid %{public}d is catched.", pidTemp);
            continue;
        }

        if (catchedPids_.count(pidTemp) == 0) {
            CatcherStacktrace(fd, pidTemp);
            pidStr += "," + std::to_string(pidTemp);
        }
        CatcherFfrtStack(fd, pidTemp);
    }

    if (event_ != nullptr) {
        event_->SetValue(LOGGER_EVENT_PEERBINDER, StringUtil::TrimStr(pidStr, ','));
    }

    logSize_ = GetFdSize(fd) - originSize;
    return logSize_;
}

void PeerBinderCatcher::AddBinderJsonInfo(std::list<OutputBinderInfo> outputBinderInfoList, int jsonFd) const
{
    if (jsonFd < 0) {
        return;
    }
    std::map<int, std::string> processNameMap;
    for (OutputBinderInfo outputBinderInfo : outputBinderInfoList) {
        int pid = outputBinderInfo.pid;
        if (processNameMap[pid] != "") {
            continue;
        }
        std::string filePath = "/proc/" + std::to_string(pid) + "/cmdline";
        std::string realPath;
        if (!FileUtil::PathToRealPath(filePath, realPath)) {
            continue;
        }
        std::ifstream cmdLineFile(realPath);
        std::string processName;
        if (cmdLineFile) {
            std::getline(cmdLineFile, processName);
            cmdLineFile.close();
            StringUtil::FormatProcessName(processName);
            processNameMap[pid] = processName;
        } else {
            HIVIEW_LOGE("Fail to open /proc/%{public}d/cmdline", pid);
        }
    }
    std::list<std::string> infoList;
    for (auto it = outputBinderInfoList.begin(); it != outputBinderInfoList.end(); it++) {
        int pid = (*it).pid;
        std::string info = (*it).info;
        std::string lineStr = info + "    " + std::to_string(pid)
            + FreezeJsonUtil::WrapByParenthesis(processNameMap[pid]);
        infoList.push_back(lineStr);
    }
    std::string binderInfoJsonStr = FreezeJsonUtil::GetStrByList(infoList);
    HIVIEW_LOGI("Get FreezeJson PeerBinder jsonStr: %{public}s.", binderInfoJsonStr.c_str());
    FreezeJsonUtil::WriteKeyValue(jsonFd, "peer_binder", binderInfoJsonStr);
}

std::map<int, std::list<PeerBinderCatcher::BinderInfo>> PeerBinderCatcher::BinderInfoParser(
    std::ifstream& fin, int fd, int jsonFd) const
{
    std::map<int, std::list<BinderInfo>> manager;
    FileUtil::SaveStringToFd(fd, "\nBinderCatcher --\n\n");
    std::list<OutputBinderInfo> outputBinderInfoList;

    BinderInfoParser(fin, fd, manager, outputBinderInfoList);
    AddBinderJsonInfo(outputBinderInfoList, jsonFd);

    FileUtil::SaveStringToFd(fd, "\n\nPeerBinder Stacktrace --\n\n");
    HIVIEW_LOGI("manager size: %{public}zu", manager.size());
    return manager;
}

std::vector<std::string> PeerBinderCatcher::GetFileToList(std::string line) const
{
    std::vector<std::string> strList;
    std::istringstream lineStream(line);
    std::string tmpstr;
    while (lineStream >> tmpstr) {
        strList.push_back(tmpstr);
    }
    HIVIEW_LOGD("strList size: %{public}zu", strList.size());
    return strList;
}

void PeerBinderCatcher::BinderInfoParser(std::ifstream& fin, int fd,
    std::map<int, std::list<PeerBinderCatcher::BinderInfo>>& manager,
    std::list<PeerBinderCatcher::OutputBinderInfo>& outputBinderInfoList) const
{
    BinderInfoLineParser(fin, fd, manager, outputBinderInfoList);
}

void PeerBinderCatcher::BinderInfoLineParser(std::ifstream& fin, int fd,
    std::map<int, std::list<PeerBinderCatcher::BinderInfo>>& manager,
    std::list<PeerBinderCatcher::OutputBinderInfo>& outputBinderInfoList) const
{
    std::string line;
    bool isBinderMatchup = false;
    while (getline(fin, line)) {
        FileUtil::SaveStringToFd(fd, line + "\n");
        isBinderMatchup = (!isBinderMatchup && line.find("free_async_space") != line.npos) ? true : isBinderMatchup;
        if (isBinderMatchup || line.find("async\t") != std::string::npos) {
            continue;
        }

        std::vector<std::string> strList = GetFileToList(line);
        auto strSplit = [](const std::string& str, uint16_t index) -> std::string {
            std::vector<std::string> strings;
            StringUtil::SplitStr(str, ":", strings);
            return index < strings.size() ? strings[index] : "";
        };
        if (strList.size() >= ARR_SIZE) { // 7: valid array size
            // 0: binder local id,
            std::string clientPid = strSplit(strList[0], 0);
            std::string clientTid = strSplit(strList[0], 1);
            // 2: binder peer id,
            std::string serverPid = strSplit(strList[2], 0);
            std::string serverTid = strSplit(strList[2], 1);
            std::string wait = strSplit(strList[5], 1);
            if (clientPid == "" || clientTid == "" || serverPid == "" || serverTid == "" || wait == "") {
                HIVIEW_LOGI("server:%{public}s, client:%{public}s, wait:%{public}s",
                    serverPid.c_str(), clientPid.c_str(), wait.c_str());
                continue;
            }
            BinderInfo info = {
                std::strtol(clientPid.c_str(), nullptr, DECIMAL), std::strtol(clientTid.c_str(), nullptr, DECIMAL),
                std::strtol(serverPid.c_str(), nullptr, DECIMAL), std::strtol(serverTid.c_str(), nullptr, DECIMAL),
                std::strtol(wait.c_str(), nullptr, DECIMAL)};
            HIVIEW_LOGD("server:%{public}d, client:%{public}d, wait:%{public}d", info.serverPid, info.clientPid,
                info.wait);
            manager[info.clientPid].push_back(info);
            OutputBinderInfo outputInfo = {StringUtil::TrimStr(line), info.serverPid};
            outputBinderInfoList.push_back(outputInfo);
        } else {
            HIVIEW_LOGI("strList size: %{public}zu, line: %{public}s", strList.size(), line.c_str());
        }
    }
}

std::set<int> PeerBinderCatcher::GetBinderPeerPids(int fd, int jsonFd)
{
    std::set<int> pids;
    std::ifstream fin;
    std::string path = binderPath_;
    fin.open(path.c_str());
    if (!fin.is_open()) {
        HIVIEW_LOGE("open binder file failed, %{public}s.", path.c_str());
        std::string content = "open binder file failed :" + path + "\r\n";
        FileUtil::SaveStringToFd(fd, content);
        return pids;
    }

    std::map<int, std::list<PeerBinderCatcher::BinderInfo>> manager = BinderInfoParser(fin, fd, jsonFd);
    fin.close();

    if (manager.size() == 0 || manager.find(pid_) == manager.end()) {
        return pids;
    }

    if (layer_ == LOGGER_BINDER_STACK_ONE) {
        for (auto each : manager[pid_]) {
            if (!firstLayerInit_) {
                terminalBinder_.pid = each.serverPid;
                terminalBinder_.tid = each.serverTid;
                firstLayerInit_ = true;
            }
            pids.insert(each.serverPid);
        }
    } else if (layer_ == LOGGER_BINDER_STACK_ALL) {
        ParseBinderCallChain(manager, pids, pid_);
    }
    return pids;
}

void PeerBinderCatcher::ParseBinderCallChain(std::map<int, std::list<PeerBinderCatcher::BinderInfo>>& manager,
    std::set<int>& pids, int pid)
{
    bool isGetLayerBinder = false;
    for (auto& each : manager[pid]) {
        if (pids.find(each.serverPid) != pids.end()) {
            continue;
        }
        pids.insert(each.serverPid);
        if (!firstLayerInit_ || (!isGetLayerBinder && terminalBinder_.pid == each.clientPid &&
            terminalBinder_.tid == each.clientTid)) {
            terminalBinder_.pid = each.serverPid;
            terminalBinder_.tid = each.serverTid;
            firstLayerInit_ = true;
            isGetLayerBinder = true;
        }
        ParseBinderCallChain(manager, pids, each.serverPid);
    }
}

bool PeerBinderCatcher::IsAncoProc(int pid) const
{
    std::string cgroupPath = "/proc/" + std::to_string(pid) + "/cgroup";
    std::string firstLine = FileUtil::GetFirstLine(cgroupPath);
    return firstLine.find("isulad") != std::string::npos;
}

void PeerBinderCatcher::CatcherFfrtStack(int fd, int pid) const
{
    std::string content =  "PeerBinderCatcher start catcher ffrt stack for pid : " + std::to_string(pid) + "\r\n";
    FileUtil::SaveStringToFd(fd, content);

    LogCatcherUtils::DumpStackFfrt(fd, std::to_string(pid));
}

void PeerBinderCatcher::CatcherStacktrace(int fd, int pid)
{
    std::string content =  "PeerBinderCatcher start catcher stacktrace for pid : " + std::to_string(pid) + "\r\n";
    FileUtil::SaveStringToFd(fd, content);

    LogCatcherUtils::DumpStacktrace(fd, pid, terminalBinder_.threadStack, terminalBinder_.pid, terminalBinder_.tid);
}

#ifdef HAS_HIPERF
void PeerBinderCatcher::DoExecHiperf(const std::string& fileName, const std::set<int>& pids)
{
    std::shared_ptr<PerfCollector> perfCollector = PerfCollector::Create();
    perfCollector->SetOutputFilename(fileName);
    constexpr int collectTime = 1;
    perfCollector->SetTimeStopSec(collectTime);
    if (perfCmd_.find("a") == std::string::npos) {
        std::vector<pid_t> selectPids;
        selectPids.push_back(pid_);
        for (const auto& pid : pids) {
            if (pid > 0) {
                selectPids.push_back(pid);
            }
        }
        perfCollector->SetSelectPids(selectPids);
        perfCollector->SetReport(true);
    } else {
        perfCollector->SetTargetSystemWide(true);
    }
    perfCollector->SetFrequency(1000); // 1000 : 1kHz
    CollectResult<bool> ret = perfCollector->StartPerf(EVENT_LOG_PATH);
    if (ret.retCode == UCollect::UcError::SUCCESS) {
        HIVIEW_LOGI("Success to call perf result : %{public}d", ret.data);
    } else {
        HIVIEW_LOGI("Failed to  call perf result : %{public}d", ret.data);
    }
}

void PeerBinderCatcher::ForkToDumpHiperf(const std::set<int>& pids)
{
#if defined(__aarch64__)
    if (perfCmd_.empty()) {
        HIVIEW_LOGI("BinderPeer perf is not configured.");
        return;
    }

    if (!Parameter::IsBetaVersion()) {
        HIVIEW_LOGI("BinderPeer perf is only enabled in beta version.");
        return;
    }

    static ffrt::mutex lock;
    std::unique_lock<ffrt::mutex> mlock(lock);
    std::string fileName = "hiperf-" + std::to_string(pid_) + ".data";
    std::string fullPath = std::string(EVENT_LOG_PATH) + "/" + fileName;
    constexpr int perfLogExPireTime = 60;
    if (access(fullPath.c_str(), F_OK) == 0) {
        struct stat statBuf;
        auto now = time(nullptr);
        if (stat(fullPath.c_str(), &statBuf) == -1) {
            HIVIEW_LOGI("Failed to stat file, error:%{public}d.", errno);
            FileUtil::RemoveFile(fullPath);
        } else if (now - statBuf.st_mtime < perfLogExPireTime) {
            HIVIEW_LOGI("Target log has exist, reuse it.");
            return;
        } else {
            FileUtil::RemoveFile(fullPath);
        }
    }

    pid_t child = fork();
    if (child < 0) {
        // failed to fork child
        return;
    } else if (child == 0) {
        pid_t grandChild = fork();
        if (grandChild == 0) {
            DoExecHiperf(fileName, pids);
        }
        _exit(0);
    } else {
        // do not left a zombie
        waitpid(child, nullptr, 0);
    }
#endif
}
#endif
} // namespace HiviewDFX
} // namespace OHOS