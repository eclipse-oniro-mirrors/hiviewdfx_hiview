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

#include <string>
#include <sys/klog.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef KERNELSTACK_CATCHER_ENABLE
#include "dfx_kernel_stack.h"
#endif

#include "hiview_logger.h"
#include "log_catcher_utils.h"
#include "common_utils.h"
#include "ffrt.h"
#include "file_util.h"
#include "time_util.h"
#include "securec.h"

namespace OHOS {
namespace HiviewDFX {
#ifdef DMESG_CATCHER_ENABLE
using namespace std::chrono_literals;
DEFINE_LOG_LABEL(0xD002D01, "EventLogger-DmesgCatcher");
namespace {
    constexpr int SYSLOG_ACTION_READ_ALL = 3;
    constexpr int SYSLOG_ACTION_SIZE_BUFFER = 10;
    constexpr mode_t DEFAULT_LOG_FILE_MODE = 0644;
    constexpr const char* FULL_DIR = "/data/log/eventlog/";
#ifdef KERNELSTACK_CATCHER_ENABLE
    static constexpr int DECIMEL = 10;
    static constexpr int DIR_BUFFER = 256;
#endif
    constexpr const char* SYSRQ_START = "sysrq start:";
    constexpr const char* SYSRQ_END = "sysrq end:";
    constexpr const char* HGUARD_WORKER = "hguard-worker";
    constexpr const char* LMK_DEBUG = "sys-lmk-debug-t";
}
DmesgCatcher::DmesgCatcher() : EventLogCatcher()
{
    name_ = "DmesgCatcher";
}

bool DmesgCatcher::Initialize(const std::string& packageNam  __UNUSED,
    int writeNewFile __UNUSED, int writeType)
{
    writeNewFile_ = writeNewFile;
    writeType_ = writeType;
    return true;
}

bool DmesgCatcher::Init(std::shared_ptr<SysEvent> event)
{
    event_ = event;
    return true;
}

bool DmesgCatcher::DumpToFile(int fdOne, int fdTwo, const std::string& dataStr)
{
    std::string strOne;
    std::string strTwo;
    if (writeType_ == HUNG_TASK) {
        GetHungTask(dataStr, strOne);
    } else {
        GetSysrq(dataStr, strOne);
        if (writeType_ == SYSRQ_HUNGTASK) {
            if (writeNewFile_) {
                GetHungTask(dataStr, strTwo);
            } else {
                strOne += "\n";
                GetHungTask(dataStr, strOne);
            }
        }
    }

    bool res = true;
    if (fdOne > -1 && writeType_ != HUNG_TASK) {
        res = FileUtil::SaveStringToFd(fdOne, strOne);
    }
    if (fdTwo > -1 && writeType_ != SYS_RQ) {
        res = FileUtil::SaveStringToFd(fdTwo, strTwo);
    }
    return res;
}
 
void DmesgCatcher::GetSysrq(const std::string& dataStr, std::string& sysrqStr)
{
    size_t lineStart = 0;
    size_t lineEnd = dataStr.size();
    size_t sysrqStart = dataStr.find(SYSRQ_START);
    if (sysrqStart == std::string::npos) {
        return;
    }
    size_t sysrqEnd = dataStr.find(SYSRQ_END, sysrqStart);
    if (sysrqEnd == std::string::npos) {
        return;
    }
    lineStart = dataStr.rfind('\n', sysrqStart);
    lineStart = (lineStart == std::string::npos) ? 0 : lineStart + 1;
    lineEnd = dataStr.find('\n', sysrqEnd);
    lineEnd = (lineEnd == std::string::npos) ? dataStr.length() : lineEnd;
    sysrqStr = dataStr.substr(lineStart, lineEnd - lineStart + 1);
}

void DmesgCatcher::GetHungTask(const std::string& dataStr, std::string& hungtaskStr)
{
    if (writeType_ == SYSRQ_HUNGTASK && !writeNewFile_) {
        hungtaskStr.append("\nHungTaskCatcher -- \n");
    }
    size_t lineStart = 0;
    size_t lineEnd = dataStr.size();
    size_t seekPos = 0;
    while (lineStart < lineEnd) {
        seekPos = dataStr.find(HGUARD_WORKER, lineStart);
        if (seekPos == std::string::npos) {
            seekPos = dataStr.find(LMK_DEBUG, lineStart);
        }
        if (seekPos == std::string::npos) {
            break;
        }
        lineStart = dataStr.rfind("\n", seekPos);
        lineStart = (lineStart == std::string::npos) ? 0 : lineStart + 1;
        lineEnd = dataStr.find("\n", seekPos);
        lineEnd = (lineEnd == std::string::npos) ? dataStr.size() : lineEnd;

        hungtaskStr.append(dataStr, lineStart, lineEnd - lineStart + 1);
        lineStart = lineEnd + 1;
    }
}

bool DmesgCatcher::DumpDmesgLog(int fdOne, int fdTwo)
{
    if (fdOne < 0 && fdTwo < 0) {
        return false;
    }

    int size = klogctl(SYSLOG_ACTION_SIZE_BUFFER, 0, 0);
    if (size <= 0) {
        return false;
    }

    std::unique_ptr<char[]> data = std::make_unique<char[]>(size + 1);
    if (!data) {
        return false;
    }

    if (memset_s(data.get(), size + 1, 0, size + 1) != 0) {
        return false;
    }

    int readSize = klogctl(SYSLOG_ACTION_READ_ALL, data.get(), size);
    if (readSize < 0) {
        return false;
    }
    std::string dataStr(data.get(), readSize);

    bool res = false;
    if (writeType_ == DMESG) {
        res = FileUtil::SaveStringToFd(fdOne, dataStr);
    } else {
        res = DumpToFile(fdOne, fdTwo, dataStr);
    }
    return res;
}

bool DmesgCatcher::WriteSysrqTrigger()
{
    static std::atomic<bool> isWriting(false);
    bool expected = false;
    if (!isWriting.compare_exchange_strong(expected, true)) {
        HIVIEW_LOGE("other is writing sysrq trigger!");
        ffrt::this_task::sleep_for(1s);
        return true;
    }

    FILE* file = fopen("/proc/sysrq-trigger", "w");
    if (file == nullptr) {
        isWriting.store(false);
        HIVIEW_LOGE("Can't read sysrq,errno: %{public}d", errno);
        return false;
    }

    fwrite("w", 1, 1, file);
    fflush(file);

    fwrite("l", 1, 1, file);
    ffrt::this_task::sleep_for(1s);
    fclose(file);
    isWriting.store(false);
    return true;
}

int DmesgCatcher::Catch(int fd, int jsonFd)
{
    if (writeType_ && !WriteSysrqTrigger()) {
        return 0;
    }
    description_ = (writeType_ == HUNG_TASK) ? "\nHungTaskCatcher -- \n" :
        (writeType_ == DMESG) ? "\nDmesgCatcher -- \n" : "\nSysrqCatcher -- \n";
    auto originSize = GetFdSize(fd);
    FileUtil::SaveStringToFd(fd, description_);
    DumpDmesgLog(fd, -1);
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
            unsigned long tid = strtoul(dent->d_name, &endptr, DECIMAL);
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
    std::string msg;
    std::vector<pid_t> tids;
    GetTidsByPid(pid, tids);
    for (auto tid : tids) {
        std::string temp;
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

void DmesgCatcher::WriteNewFile(int pid)
{
    if (writeType_ && !WriteSysrqTrigger()) {
        return;
    }
    std::string fileTime = (writeType_ == SYS_RQ) ?  event_->GetEventValue("SYSRQ_TIME") :
        event_->GetEventValue("HUNGTASK_TIME");
    std::string fileNameOne = (writeType_ == HUNG_TASK) ? "" : "sysrq-" + fileTime + ".log";
    std::string fileNameTwo = (writeType_ == SYS_RQ) ? "" : "hungtask-" + fileTime + ".log";
    HIVIEW_LOGI("write %{public}d %{public}s %{public}s start", writeType_, fileNameOne.c_str(), fileNameTwo.c_str());

    int fdOne = -1;
    FILE* fpOne = GeFileInfoByName(fileNameOne, fdOne);
    int fdTwo = -1;
    FILE* fpTwo = GeFileInfoByName(fileNameTwo, fdTwo);
    if (!fpOne && !fpTwo) {
        return;
    }
    DumpDmesgLog(fdOne, fdTwo);
#ifdef KERNELSTACK_CATCHER_ENABLE
    if (writeType_ == SYS_RQ) {
        DumpKernelStacktrace(fd, pid);
    }
#endif // KERNELSTACK_CATCHER_ENABLE
    CloseFp(fpOne);
    CloseFp(fpTwo);
    HIVIEW_LOGI("write file %{public}s %{public}s end", fileNameOne.c_str(), fileNameTwo.c_str());
}

FILE* DmesgCatcher::GeFileInfoByName(const std::string& fileName, int& fd)
{
    if (fileName.empty()) {
        return nullptr;
    }
    std::string fullPath = std::string(FULL_DIR) + fileName;
    if (FileUtil::FileExists(fullPath)) {
        return nullptr;
    }
    FILE* fp = fopen(fullPath.c_str(), "w");
    if (!fp) {
        HIVIEW_LOGI("Fail to create %{public}s, errno: %{public}d.", fileName.c_str(), errno);
    }
    chmod(fullPath.c_str(), DEFAULT_LOG_FILE_MODE);
    fd = fileno(fp);
    return fp;
}

void DmesgCatcher::CloseFp(FILE*& fp)
{
    if (fp == nullptr) {
        return;
    }

    if (fclose(fp) != 0) {
        HIVIEW_LOGE("fclose failed, errno: %{public}d", errno);
    }
    fp = nullptr;
}
#endif // DMESG_CATCHER_ENABLE
} // namespace HiviewDFX
} // namespace OHOS
