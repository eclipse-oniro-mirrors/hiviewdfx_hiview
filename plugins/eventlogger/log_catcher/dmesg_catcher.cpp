/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include "dmesg_catcher.h"

#include <cstdio>
#include <string>
#include <sys/klog.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sstream>
#include <iostream>

#ifdef KERNELSTACK_CATCHER_ENABLE
#include "dfx_kernel_stack.h"
#endif

#include "hiview_logger.h"
#include "log_catcher_utils.h"
#include "common_utils.h"
#include "file_util.h"
#include "time_util.h"
#include "securec.h"

namespace OHOS {
namespace HiviewDFX {
#ifdef DMESG_CATCHER_ENABLE
DEFINE_LOG_LABEL(0xD002D01, "EventLogger-DmesgCatcher");
namespace {
    constexpr int SYSLOG_ACTION_READ_ALL = 3;
    constexpr int SYSLOG_ACTION_SIZE_BUFFER = 10;
    constexpr mode_t DEFAULT_LOG_FILE_MODE = 0644;
    static constexpr const char* const FULL_DIR = "/data/log/eventlog/";
    static constexpr int DECIMEL = 10;
    static constexpr int DIR_BUFFER = 256;
}
DmesgCatcher::DmesgCatcher() : EventLogCatcher()
{
    event_ = nullptr;
    name_ = "DmesgCatcher";
}

bool DmesgCatcher::Initialize(const std::string& packageNam  __UNUSED,
    int isWriteNewFile __UNUSED, int needWriteSysrq)
{
    isWriteNewFile_ = isWriteNewFile;
    needWriteSysrq_ = needWriteSysrq;
    return true;
}

bool DmesgCatcher::Init(std::shared_ptr<SysEvent> event)
{
    event_ = event;
    return true;
}

bool DmesgCatcher::DumpSysrqToFile(int fd, char *data, int size)
{
    std::string dataStr = std::string(data, size);
    std::stringstream ss(dataStr);
    std::string line;
    bool res = false;
 
    while (std::getline(ss, line)) {
        if (line.find("hguard-worker") != std::string::npos) {
            line += "\n";
            res = FileUtil::SaveStringToFd(fd, line);
        }
    }
    return res;
}

bool DmesgCatcher::DumpDmesgLog(int fd)
{
    if (fd < 0) {
        return false;
    }
    int size = klogctl(SYSLOG_ACTION_SIZE_BUFFER, 0, 0);
    if (size <= 0) {
        return false;
    }
    char *data = (char *)malloc(size + 1);
    if (data == nullptr) {
        return false;
    }

    memset_s(data, size + 1, 0, size + 1);
    int readSize = klogctl(SYSLOG_ACTION_READ_ALL, data, size);
    if (readSize < 0) {
        free(data);
        return false;
    }
    bool res = needWriteSysrq_ ? DumpSysrqToFile(fd, data, size) : FileUtil::SaveStringToFd(fd, data);
    free(data);
    return res;
}

bool DmesgCatcher::WriteSysrq()
{
    FILE* file = fopen("/proc/sysrq-trigger", "w");
    if (file == nullptr) {
        HIVIEW_LOGE("Can't read sysrq,errno: %{public}d", errno);
        return false;
    }
    fwrite("w", 1, 1, file);
    fflush(file);

    fwrite("l", 1, 1, file);
    sleep(1);
    fclose(file);
    return true;
}

int DmesgCatcher::Catch(int fd, int jsonFd)
{
    if (needWriteSysrq_ && !WriteSysrq()) {
        return 0;
    }
    description_ = needWriteSysrq_ ? "\nSysrqCatcher -- \n" : "DmesgCatcher -- \n";
    auto originSize = GetFdSize(fd);
    FileUtil::SaveStringToFd(fd, description_);
    DumpDmesgLog(fd);
    logSize_ = GetFdSize(fd) - originSize;
    return logSize_;
}
#ifdef KERNELSTACK_CATCHER_ENABLE
void DmesgCatcher::GetTidsByPid(int pid, std::vector<pid_t>& tids)
{
    char taskDir[DIR_BUFFER];
    if (pid < 0 || sprintf_s(taskDir, sizeof(taskDir), "/proc/%d/task", pid) < 0) {
        HIVIEW_LOGW("get tids failed, pid: %{public}d", pid);
        return;
    }

    DIR* dir = opendir(taskDir);
    if (dir != nullptr) {
        struct dirent* dent;
        while ((dent = readdir(dir)) != nullptr) {
            char* endptr;
            unsigned long tid = strtoul(dent->d_name, &endptr, DECIMEL);
            if (tid == ULONG_MAX || *endptr) {
                continue;
            }
            tids.push_back(tid);
        }
        closedir(dir);
    }
}

int DmesgCatcher::DumpKernelStacktrace(int fd, int pid)
{
    if (fd < 0 || pid < 0) {
        return -1;
    }
    std::string msg = "";
    std::vector<pid_t> tids;
    GetTidsByPid(pid, tids);
    for (auto tid : tids) {
        std::string temp = "";
        if (DfxGetKernelStack(tid, temp) != 0) {
            msg = "Failed to format kernel stack for " + std::to_string(tid) + temp + "\n";
            continue;
        }
        msg += temp + "\n";
    }
    if (msg == "") {
        msg = "dumpCatch return empty stack!!!!";
    }
    FileUtil::SaveStringToFd(fd, msg);
    return 0;
}
#endif

void DmesgCatcher::WriteNewSysrq(int pid)
{
    if (needWriteSysrq_ && !WriteSysrq()) {
        return;
    }
    std::string sysrqTime = event_->GetEventValue("SYSRQ_TIME");
    std::string fullPath = std::string(FULL_DIR) + "sysrq-" + sysrqTime + ".log";
    HIVIEW_LOGI("write new sysrq start, fullPath : %{public}s", fullPath.c_str());
    if (FileUtil::FileExists(fullPath)) {
        HIVIEW_LOGW("filename: %{public}s is existed, direct use.", fullPath.c_str());
        return;
    }
    FILE* fp = fopen(fullPath.c_str(), "w");
    chmod(fullPath.c_str(), DEFAULT_LOG_FILE_MODE);
    if (fp == nullptr) {
        HIVIEW_LOGI("Fail to create %{public}s, errno: %{public}d.", fullPath.c_str(), errno);
        return;
    }
    auto fd = fileno(fp);
    DumpDmesgLog(fd);
#ifdef KERNELSTACK_CATCHER_ENABLE
    DumpKernelStacktrace(fd, pid);
#endif // KERNELSTACK_CATCHER_ENABLE
    if (fclose(fp)) {
        HIVIEW_LOGE("fclose is failed");
    }
    fp = nullptr;
    HIVIEW_LOGI("write new sysrq end, fullPath : %{public}s", fullPath.c_str());
}
#endif // DMESG_CATCHER_ENABLE
} // namespace HiviewDFX
} // namespace OHOS
