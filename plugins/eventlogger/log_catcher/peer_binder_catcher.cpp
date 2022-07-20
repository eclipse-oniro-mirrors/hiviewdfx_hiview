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

#include <memory>
#include <string>
#include <vector>

#include <sys/stat.h>

#include "securec.h"

#include "common_utils.h"
#include "file_util.h"
#include "log_catcher_utils.h"
#include "logger.h"
#include "string_util.h"

#include "open_stacktrace_catcher.h"
namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D01, "EventLogger-PeerBinderCatcher");
PeerBinderCatcher::PeerBinderCatcher() : EventLogCatcher()
{
    name_ = "PeerBinderCatcher";
}

bool PeerBinderCatcher::Initialize(const std::string& strParam1, int pid, int layer)
{
    pid_ = pid;
    layer_ = layer;
    char buf[BUF_SIZE_512] = {0};
    int ret = snprintf_s(buf, BUF_SIZE_512, BUF_SIZE_512 - 1,
        "PeerBinderCatcher -- pid==%d layer_ == %d\n", pid_, layer_);
    if (ret > 0) {
        description_ = buf;
    }
    return true;
}

void PeerBinderCatcher::Init(std::shared_ptr<SysEvent> event, const std::string& filePath)
{
    event_ = event;
    if ((filePath != "") && FileUtil::FileExists(filePath)) {
        binderPath_ = filePath;
    }
}

int PeerBinderCatcher::Catch(int fd)
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

    std::set<int> pids = GetBinderPeerPids(fd);
    if (pids.empty()) {
        std::string content = "PeerBinder pids is empty\r\n";
        FileUtil::SaveStringToFd(fd, content);
    }

    std::string pidStr = "";
    for (auto pidTemp : pids) {
        if (pidTemp != pid_) {
            CatcherStacktrace(fd, pidTemp);
            pidStr += "," + std::to_string(pidTemp);
        }
    }

    if (event_ != nullptr) {
        event_->SetValue(LOGGER_EVENT_PEERBINDER, StringUtil::TrimStr(pidStr, ','));
    }

    logSize_ = GetFdSize(fd) - originSize;
    return logSize_;
}

std::map<int, std::list<PeerBinderCatcher::BinderInfo>> PeerBinderCatcher::BinderInfoParser(
    std::ifstream& fin, int fd) const
{
    std::map<int, std::list<BinderInfo>> manager;
    const int DECIMAL = 10;
    const int WAIT_TIME_NUM = 3;
    const int SERVCER_PID_NUM = 2;
    const int CLIENT_PID_NUM = 1;
    const int BINDER_TIMEOUT = 1;
    std::string pattern = "(\\d+):.*to\\s*(\\d+):.*code.*wait:(\\d+)\\.";
    std::regex reg(pattern);
    std::string line;
    bool findBinderHeader = false;
    FileUtil::SaveStringToFd(fd, "\nBinderCatcher --\n\n");
    while (getline(fin, line)) {
        FileUtil::SaveStringToFd(fd, line + "\n");
        if (findBinderHeader) {
            continue;
        }
        std::smatch match;
        if (!std::regex_search(line, match, reg)) {
            if (line.find("context") != line.npos) {
                findBinderHeader = true;
            }
            continue;
        }
        BinderInfo info;
        // 2: binder peer id, 10:dec
        info.server = std::strtol(std::string(match[SERVCER_PID_NUM]).c_str(), nullptr, DECIMAL);
        // 1: binder local id, 10:dec
        info.client = std::strtol(std::string(match[CLIENT_PID_NUM]).c_str(), nullptr, DECIMAL);
        // 3: binder wait time, 10:dec s
        info.wait = std::strtol(std::string(match[WAIT_TIME_NUM]).c_str(), nullptr, DECIMAL);
        HIVIEW_LOGI("server:%{public}d, client:%{public}d, wait:%{public}d", info.server, info.client, info.wait);
        if (info.wait >= BINDER_TIMEOUT) {
            manager[info.client].push_back(info);
        }
    }
    FileUtil::SaveStringToFd(fd, "\n\nPeerBinder Stacktrace --\n\n");
    HIVIEW_LOGI("manager size: %{public}zu", manager.size());
    return manager;
}

std::set<int> PeerBinderCatcher::GetBinderPeerPids(int fd) const
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

    std::map<int, std::list<PeerBinderCatcher::BinderInfo>> manager = BinderInfoParser(fin, fd);
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

void PeerBinderCatcher::CatcherStacktrace(int fd, int pid) const
{
    std::string content =  "PeerBinderCatcher start catcher stacktrace for pid : " + std::to_string(pid) + "\r\n";
    FileUtil::SaveStringToFd(fd, content);

    LogCatcherUtils::DumpStacktrace(fd, pid);
}
} // namespace HiviewDFX
} // namespace OHOS