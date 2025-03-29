/*
 * Copyright (C) 2021-2025 Huawei Device Co., Ltd.
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
    static constexpr uint8_t FREE_ASYNC_INDEX = 6;
    static constexpr uint16_t FREE_ASYNC_MAX = 1000;
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
    std::set<int> asyncPids;
    std::set<int> syncPids = GetBinderPeerPids(fd, jsonFd, asyncPids);
    if (syncPids.empty()) {
        std::string content = "PeerBinder pids is empty\r\n";
        FileUtil::SaveStringToFd(fd, content);
    }
#ifdef HAS_HIPERF
    DumpHiperf(syncPids);
#endif
    std::string pidStr = "";
    for (auto pidTemp : syncPids) {
        if (pidTemp == pid_ || IsAncoProc(pidTemp)) {
            HIVIEW_LOGI("Stack of PeerBinder Pid %{public}d is catched.", pidTemp);
            continue;
        }

        if (catchedPids_.count(pidTemp) == 0) {
            CatcherStacktrace(fd, pidTemp);
            pidStr += "," + std::to_string(pidTemp);
        }
        CatcherFfrtStack(fd, pidTemp);
    }
    for (auto pidTemp : asyncPids) {
        if (pidTemp == pid_ || IsAncoProc(pidTemp) || syncPids.find(pidTemp) != syncPids.end() ||
            catchedPids_.count(pidTemp) != 0) {
                HIVIEW_LOGI("Stack of AsyncBinder Pid %{public}d is catched.", pidTemp);
                continue;
        }
        CatcherStacktrace(fd, pidTemp, false);
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
    std::ifstream& fin, int fd, int jsonFd, std::set<int>& asyncPids) const
{
    std::map<int, std::list<BinderInfo>> manager;
    FileUtil::SaveStringToFd(fd, "\nBinderCatcher --\n\n");
    std::list<OutputBinderInfo> outputBinderInfoList;

    BinderInfoParser(fin, fd, manager, outputBinderInfoList, asyncPids);
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
    std::list<PeerBinderCatcher::OutputBinderInfo>& outputBinderInfoList, std::set<int>& asyncPids) const
{
    std::map<uint32_t, uint32_t> asyncBinderMap;
    std::vector<std::pair<uint32_t, uint64_t>> freeAsyncSpacePairs;
    BinderInfoLineParser(fin, fd, manager, outputBinderInfoList, asyncBinderMap, freeAsyncSpacePairs);

    std::sort(freeAsyncSpacePairs.begin(), freeAsyncSpacePairs.end(),
        [] (const auto& pairOne, const auto& pairTwo) { return pairOne.second < pairTwo.second; });
    std::vector<std::pair<uint32_t, uint32_t>> asyncBinderPairs(asyncBinderMap.begin(), asyncBinderMap.end());
    std::sort(asyncBinderPairs.begin(), asyncBinderPairs.end(),
        [] (const auto& pairOne, const auto& pairTwo) { return pairOne.second > pairTwo.second; });

    size_t freeAsyncSpaceSize = freeAsyncSpacePairs.size();
    size_t asyncBinderSize = asyncBinderPairs.size();
    size_t individualMaxSize = 2;
    for (size_t i = 0; i < individualMaxSize; i++) {
        if (i < freeAsyncSpaceSize) {
            asyncPids.insert(freeAsyncSpacePairs[i].first);
        }
        if (i < asyncBinderSize) {
            asyncPids.insert(asyncBinderPairs[i].first);
        }
    }
}

void PeerBinderCatcher::BinderInfoLineParser(std::ifstream& fin, int fd,
    std::map<int, std::list<PeerBinderCatcher::BinderInfo>>& manager,
    std::list<PeerBinderCatcher::OutputBinderInfo>& outputBinderInfoList,
    std::map<uint32_t, uint32_t>& asyncBinderMap,
    std::vector<std::pair<uint32_t, uint64_t>>& freeAsyncSpacePairs) const
{
    std::string line;
    bool isBinderMatchup = false;
    while (getline(fin, line)) {
        FileUtil::SaveStringToFd(fd, line + "\n");
        isBinderMatchup = (!isBinderMatchup && line.find("free_async_space") != line.npos) ? true : isBinderMatchup;
        std::vector<std::string> strList = GetFileToList(line);
        auto strSplit = [](const std::string& str, uint16_t index) -> std::string {
            std::vector<std::string> strings;
            StringUtil::SplitStr(str, ":", strings);
            return index < strings.size() ? strings[index] : "";
        };

        if (isBinderMatchup) {
            if (line.find("free_async_space") == line.npos && strList.size() == ARR_SIZE &&
                std::atoll(strList[FREE_ASYNC_INDEX].c_str()) < FREE_ASYNC_MAX) {
                freeAsyncSpacePairs.emplace_back(
                    std::atoi(strList[0].c_str()),
                    std::atoll(strList[FREE_ASYNC_INDEX].c_str()));
            }
        } else if (line.find("async\t") != std::string::npos && strList.size() > ARR_SIZE) {
            std::string serverPid = strSplit(strList[3], 0);
            std::string serverTid = strSplit(strList[3], 1);
            if (serverPid != "" && serverTid != "" && std::atoi(serverTid.c_str()) == 0) {
                asyncBinderMap[std::atoi(serverPid.c_str())]++;
            }
        } else if (strList.size() >= ARR_SIZE) { // 7: valid array size
            BinderInfo info = {0};
            OutputBinderInfo outputInfo;
            // 2: peer id,
            std::string server = strSplit(strList[2], 0);
            // 0: local id,
            std::string client = strSplit(strList[0], 0);
            // 5: wait time, s
            std::string wait = strSplit(strList[5], 1);
            if (server == "" || client == "" || wait == "") {
                HIVIEW_LOGI("server:%{public}s, client:%{public}s, wait:%{public}s",
                    server.c_str(), client.c_str(), wait.c_str());
                continue;
            }
            info.server = std::strtol(server.c_str(), nullptr, DECIMAL);
            info.client = std::strtol(client.c_str(), nullptr, DECIMAL);
            info.wait = std::strtol(wait.c_str(), nullptr, DECIMAL);
            HIVIEW_LOGD("server:%{public}d, client:%{public}d, wait:%{public}d", info.server, info.client, info.wait);
            manager[info.client].push_back(info);
            outputInfo.info = StringUtil::TrimStr(line);
            outputInfo.pid = info.server;
            outputBinderInfoList.push_back(outputInfo);
        } else {
            HIVIEW_LOGI("strList size: %{public}zu, line: %{public}s", strList.size(), line.c_str());
        }
    }
}

std::set<int> PeerBinderCatcher::GetBinderPeerPids(int fd, int jsonFd, std::set<int>& asyncPids) const
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

    std::map<int, std::list<PeerBinderCatcher::BinderInfo>> manager = BinderInfoParser(fin, fd, jsonFd, asyncPids);
    fin.close();

    if (manager.size() == 0 || manager.find(pid_) == manager.end()) {
        return pids;
    }

    if (layer_ == LOGGER_BINDER_STACK_ONE) {
        for (auto each : manager[pid_]) {
            pids.insert(each.server);
        }
    } else if (layer_ == LOGGER_BINDER_STACK_ALL) {
        ParseBinderCallChain(manager, pids, pid_);
    }
    return pids;
}

void PeerBinderCatcher::ParseBinderCallChain(std::map<int, std::list<PeerBinderCatcher::BinderInfo>>& manager,
    std::set<int>& pids, int pid) const
{
    for (auto& each : manager[pid]) {
        if (pids.find(each.server) != pids.end()) {
            continue;
        }
        pids.insert(each.server);
        ParseBinderCallChain(manager, pids, each.server);
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
    std::string content =  "PeerBinder catcher ffrt stacktrace for pid : " + std::to_string(pid) + "\r\n";
    FileUtil::SaveStringToFd(fd, content);
 
    std::string serviceName = (LogCatcherUtils::GetFfrtDumpType(pid) == LogCatcherUtils::APP) ?
        "ApplicationManagerService" : "SystemAbilityManager";
    int count = LogCatcherUtils::WAIT_CHILD_PROCESS_COUNT;
    LogCatcherUtils::ReadShellToFile(fd, serviceName, "--ffrt " + std::to_string(pid), count);
}

void PeerBinderCatcher::CatcherStacktrace(int fd, int pid, bool sync) const
{
    std::string type = sync ? "peer" : "async";
    std::string content = "Binder catcher stacktrace, type is " + type + ", pid : " + std::to_string(pid) + "\r\n";
    FileUtil::SaveStringToFd(fd, content);

    LogCatcherUtils::DumpStacktrace(fd, pid);
}

#ifdef HAS_HIPERF
void PeerBinderCatcher::DoExecHiperf(const std::string& fileName, const std::set<int>& pids)
{
    std::shared_ptr<PerfCollector> perfCollector = PerfCollector::Create(PerfCaller::EVENTLOGGER);
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

void PeerBinderCatcher::DumpHiperf(const std::set<int>& pids)
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

    static std::mutex lock;
    std::unique_lock<std::mutex> mlock(lock);
    std::string fileName = "hiperf-" + std::to_string(pid_) + ".data";
    std::string fullPath = std::string(EVENT_LOG_PATH) + "/" + fileName;
    if (access(fullPath.c_str(), F_OK) == 0) {
        struct stat statBuf;
        auto now = time(nullptr);
        if (stat(fullPath.c_str(), &statBuf) == -1) {
            HIVIEW_LOGI("Failed to stat file, error:%{public}d.", errno);
            FileUtil::RemoveFile(fullPath);
        } else if (now - statBuf.st_mtime < PERF_LOG_EXPIRE_TIME) {
            HIVIEW_LOGI("Target log has exist, reuse it.");
            return;
        } else {
            FileUtil::RemoveFile(fullPath);
        }
    }
    DoExecHiperf(fileName, pids);
#endif
}
#endif
} // namespace HiviewDFX
} // namespace OHOS