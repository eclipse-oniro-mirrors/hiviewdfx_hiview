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
#include "log_catcher_utils.h"

#include <map>
#include <memory>
#include <sstream>
#include <fcntl.h>

#include "dfx_dump_catcher.h"

#include "common_utils.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "string_util.h"
#include "dfx_json_formatter.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace LogCatcherUtils {
static std::map<int, std::shared_ptr<std::pair<bool, std::string>>> dumpMap;
static std::mutex dumpMutex;
static std::condition_variable getSync;
static constexpr int DUMP_KERNEL_STACK_SUCCESS = 1;
static constexpr int DUMP_STACK_FAILED = -1;
static constexpr mode_t DEFAULT_LOG_FILE_MODE = 0644;
bool GetDump(int pid, std::string& msg)
{
    std::unique_lock lock(dumpMutex);
    auto it = dumpMap.find(pid);
    if (it == dumpMap.end()) {
        dumpMap[pid] = std::make_shared<std::pair<bool, std::string>>(
            std::pair<bool, std::string>(false, ""));
        return false;
    }
    std::shared_ptr<std::pair<bool, std::string>> tmp = it->second;
    if (!tmp->first) {
        getSync.wait_for(lock, std::chrono::seconds(10), // 10: dump stack timeout
                         [pid]() -> bool {
                                return (dumpMap.find(pid) == dumpMap.end());
                            });
        if (!tmp->first) {
            return false;
        }
    }
    msg = tmp->second;
    return true;
}

void FinshDump(int pid, const std::string& msg)
{
    std::lock_guard lock(dumpMutex);
    auto it = dumpMap.find(pid);
    if (it == dumpMap.end()) {
        return;
    }
    std::shared_ptr<std::pair<bool, std::string>> tmp = it->second;
    tmp->first = true;
    tmp->second = msg;
    dumpMap.erase(pid);
    getSync.notify_all();
}

int WriteKernelStackToFd(int originFd, const std::string& msg, int pid)
{
    std::string logPath = "/data/log/eventlog/";
    std::vector<std::string> files;
    FileUtil::GetDirFiles(logPath, files, false);
    std::string filterName = "-KernelStack-" + std::to_string(originFd);
    std::string targetPath = "";
    for (auto& fileName : files) {
        if (fileName.find(filterName) != std::string::npos) {
            targetPath = fileName;
            break;
        }
    }
    int fd = -1;
    std::string realPath = "";
    if (FileUtil::PathToRealPath(targetPath, realPath)) {
        fd = open(realPath.c_str(), O_WRONLY | O_APPEND);
    } else {
        std::string procName = CommonUtils::GetProcFullNameByPid(pid);
        if (procName.empty()) {
            return -1;
        }
        auto logTime = TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC;
        std::string formatTime = TimeUtil::TimestampFormatToDate(logTime, "%Y%m%d%H%M%S");
        std::string logName = procName + "-" + std::to_string(pid) +
            "-" + formatTime + filterName + ".log";
        realPath = logPath + logName;
        fd = open(realPath.c_str(), O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_LOG_FILE_MODE);
    }
    if (fd >= 0) {
        FileUtil::SaveStringToFd(fd, msg);
        close(fd);
        return 0;
    }
    return -1;
}

int DumpStacktrace(int fd, int pid)
{
    if (fd < 0) {
        return -1;
    }
    std::string msg = "";
    if (!GetDump(pid, msg)) {
        DfxDumpCatcher dumplog;
        std::string ret;
        auto dumpResult = dumplog.DumpCatchProcess(pid, ret);
        if (dumpResult == DUMP_STACK_FAILED) {
            msg = "Failed to dump stacktrace for " + std::to_string(pid) + "\n" + ret;
        } else if (dumpResult == DUMP_KERNEL_STACK_SUCCESS) {
            if (!DfxJsonFormatter::FormatKernelStack(ret, msg, false)) {
                msg = "Failed to format kernel stack for " + std::to_string(pid) + "\n";
            }
            WriteKernelStackToFd(fd, ret, pid);
        } else {
            msg = ret;
        }
        FinshDump(pid, "\n-repeat-\n" + msg);
    }

    if (msg == "") {
        msg = "dumpCatch return empty stack!!!!";
    }
    FileUtil::SaveStringToFd(fd, msg);
    return 0;
}
}
} // namespace HiviewDFX
} // namespace OHOS
