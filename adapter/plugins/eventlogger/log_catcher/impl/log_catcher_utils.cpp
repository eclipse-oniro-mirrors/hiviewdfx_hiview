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

#include <cstdio>
#include <fcntl.h>
#include <map>
#include <memory>
#include <sstream>
#include <sys/wait.h>

#include "common_utils.h"
#include "dfx_dump_catcher.h"
#include "dfx_json_formatter.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "iservice_registry.h"
#include "string_util.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace LogCatcherUtils {
static std::map<int, std::shared_ptr<std::pair<bool, std::string>>> dumpMap;
static std::mutex dumpMutex;
static std::condition_variable getSync;
static constexpr int DUMP_KERNEL_STACK_SUCCESS = 1;
static constexpr int DUMP_STACK_FAILED = -1;
static constexpr int MAX_RETRY_COUNT = 20;
static constexpr int WAIT_CHILD_PROCESS_INTERVAL = 5 * 1000;
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
    std::string filterName = "-KernelStack-" + std::to_string(originFd) + ".log";
    std::string targetPath = "";
    for (auto& fileName : files) {
        if (fileName.find(filterName) != std::string::npos) {
            targetPath = fileName;
            break;
        }
    }
    FILE* fp = nullptr;
    std::string realPath = "";
    if (FileUtil::PathToRealPath(targetPath, realPath)) {
        fp = fopen(realPath.c_str(), "a");
    } else {
        std::string procName = CommonUtils::GetProcFullNameByPid(pid);
        if (procName.empty()) {
            return -1;
        }
        StringUtil::FormatProcessName(procName);
        auto logTime = TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC;
        std::string formatTime = TimeUtil::TimestampFormatToDate(logTime, "%Y%m%d%H%M%S");
        std::string logName = procName + "-" + formatTime + filterName;
        realPath = logPath + logName;
        fp = fopen(realPath.c_str(), "w+");
        chmod(realPath.c_str(), DEFAULT_LOG_FILE_MODE);
    }
    if (fp != nullptr) {
        FileUtil::SaveStringToFile(realPath, msg);
        (void)fclose(fp);
        fp = nullptr;
        return 0;
    }
    return -1;
}

int DumpStacktrace(int fd, int pid, std::string& terminalBinderStack, int terminalBinderPid, int terminalBinderTid)
{
    if (fd < 0) {
        return -1;
    }
    std::string msg = "";
    if (!GetDump(pid, msg)) {
        DfxDumpCatcher dumplog;
        std::string ret;
        std::pair<int, std::string> dumpResult = dumplog.DumpCatchWithTimeout(pid, ret);
        if (dumpResult.first == DUMP_STACK_FAILED) {
            msg = "Failed to dump stacktrace for " + std::to_string(pid) + "\n" + dumpResult.second + "\n" + ret;
        } else if (dumpResult.first == DUMP_KERNEL_STACK_SUCCESS) {
            std::string failInfo = "Failed to dump normal stacktrace for " + std::to_string(pid) + "\n" +
                dumpResult.second;
            msg = failInfo + (DfxJsonFormatter::FormatKernelStack(ret, msg, false) ? msg :
                "Failed to format kernel stack for " + std::to_string(pid) + "\n");
            WriteKernelStackToFd(fd, ret, pid);
        } else {
            msg = ret;
        }
        FinshDump(pid, "\n-repeat-\n" + msg);
    }

    if (msg == "") {
        msg = "dumpCatch return empty stack!!!!";
    }
    if (terminalBinderPid > 0 && pid == terminalBinderPid) {
        terminalBinderTid  = (terminalBinderTid > 0) ? terminalBinderTid : terminalBinderPid;
        GetThreadStack(msg, terminalBinderStack, terminalBinderTid);
    }

    FileUtil::SaveStringToFd(fd, msg);
    return 0;
}

void GetThreadStack(const std::string& processStack, std::string& stack, int tid)
{
    if (tid <= 0) {
        return;
    }

    std::istringstream issStack(processStack);
    std::string regTidString = "^Tid:" + std::to_string(tid) + ", Name:(.{0,32})$";
    std::regex regTid(regTidString);
    std::regex regStack(R"(^#\d{2,3} (pc|at) .{0,1024}$)");
    std::string line;
    while (std::getline(issStack, line)) {
        if (!issStack.good()) {
            break;
        }

        if (!std::regex_match(line, regTid)) {
            continue;
        }

        while (std::getline(issStack, line) && std::regex_match(line, regStack)) {
            stack.append(line + "\n");
            if (!issStack.good()) {
                break;
            }
        };
        break;
    }
}

int DumpStackFfrt(int fd, const std::string& pid)
{
    std::list<SystemProcessInfo> systemProcessInfos;
    sptr<ISystemAbilityManager> sam = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    sam->GetRunningSystemProcess(systemProcessInfos);
    std::string serviceName = std::any_of(systemProcessInfos.begin(), systemProcessInfos.end(),
        [pid](auto& systemProcessInfo) { return pid == std::to_string(systemProcessInfo.pid); }) ?
        "SystemAbilityManager" : "ApplicationManagerService";
    int count = WAIT_CHILD_PROCESS_COUNT;

    ReadShellToFile(fd, serviceName, "--ffrt " + pid, count);
    return 0;
}

void ReadShellToFile(int fd, const std::string& serviceName, const std::string& cmd, int& count)
{
    int childPid = fork();
    if (childPid < 0) {
        return;
    }
    if (childPid == 0) {
        if (fd < 0 || dup2(fd, STDOUT_FILENO) == -1 || dup2(fd, STDIN_FILENO) == -1 || dup2(fd, STDERR_FILENO) == -1) {
            _exit(-1);
        }
        int ret = execl("/system/bin/hidumper", "hidumper", "-s", serviceName.c_str(), "-a", cmd.c_str(), nullptr);
        if (ret < 0) {
            _exit(-1);
        }
    } else {
        int ret = waitpid(childPid, nullptr, WNOHANG);
        while (count > 0 && (ret == 0)) {
            usleep(WAIT_CHILD_PROCESS_INTERVAL);
            count--;
            ret = waitpid(childPid, nullptr, WNOHANG);
        }

        if (ret == childPid || ret < 0) {
            return;
        }

        kill(childPid, SIGKILL);
        int retryCount = MAX_RETRY_COUNT;
        while (retryCount > 0 && waitpid(childPid, nullptr, WNOHANG) == 0) {
            usleep(WAIT_CHILD_PROCESS_INTERVAL);
            retryCount--;
        }
    }
}
}
} // namespace HiviewDFX
} // namespace OHOS
