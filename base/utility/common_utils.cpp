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

#include "common_utils.h"

#include <cstdint>
#include <cstdio>
#include <dirent.h>
#include <fcntl.h>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <vector>

#include "hiview_logger.h"
#include "securec.h"
#include "time_util.h"

using namespace std;
namespace OHOS {
namespace HiviewDFX {
namespace CommonUtils {
DEFINE_LOG_TAG("CommonUtils");
namespace {
constexpr int32_t UID_TRANSFORM_DIVISOR = 200000;
std::string GetProcessNameFromProcCmdline(int32_t pid)
{
    std::string procCmdlinePath = "/proc/" + std::to_string(pid) + "/cmdline";
    std::string procCmdlineContent = FileUtil::GetFirstLine(procCmdlinePath);
    if (procCmdlineContent.empty()) {
        return "";
    }

    size_t procNameStartPos = 0;
    size_t procNameEndPos = procCmdlineContent.size();
    for (size_t i = 0; i < procCmdlineContent.size(); i++) {
        if (procCmdlineContent[i] == '/') {
            // for the format '/system/bin/hiview' of the cmdline file
            procNameStartPos = i + 1; // 1 for next char
        } else if (procCmdlineContent[i] == '\0') {
            // for the format 'hiview \0 3 \0 hiview' of the cmdline file
            procNameEndPos = i;
            break;
        }
    }
    return procCmdlineContent.substr(procNameStartPos, procNameEndPos - procNameStartPos);
}

std::string GetProcessNameFromProcStat(int32_t pid)
{
    std::string procStatFilePath = "/proc/" + std::to_string(pid) + "/stat";
    std::string procStatFileContent = FileUtil::GetFirstLine(procStatFilePath);
    if (procStatFileContent.empty()) {
        return "";
    }
    // for the format '40 (hiview) I ...'
    auto procNameStartPos = procStatFileContent.find('(');
    if (procNameStartPos == std::string::npos) {
        return "";
    }
    procNameStartPos += 1; // 1: for '(' next char
    
    auto procNameEndPos = procStatFileContent.find(')');
    if (procNameEndPos == std::string::npos) {
        return "";
    }
    if (procNameEndPos <= procNameStartPos) {
        return "";
    }
    return procStatFileContent.substr(procNameStartPos, procNameEndPos - procNameStartPos);
}
}

std::string GetProcNameByPid(pid_t pid)
{
    std::string result;
    char buf[BUF_SIZE_256] = {0};
    if (snprintf_s(buf, BUF_SIZE_256, BUF_SIZE_256 - 1, "/proc/%d/comm", pid) <= 0) {
        HIVIEW_LOGE("failed to printf %{public}d", errno);
        return "";
    }
    FileUtil::LoadStringFromFile(std::string(buf, strlen(buf)), result);
    auto pos = result.find_last_not_of(" \n\r\t");
    if (pos == std::string::npos) {
        return result;
    }
    result.erase(pos + 1);
    return result;
}

std::string GetProcFullNameByPid(pid_t pid)
{
    std::string procName = GetProcessNameFromProcCmdline(pid);
    if (procName.empty() && errno != ESRCH) { // ESRCH means 'no such process'
        procName = GetProcessNameFromProcStat(pid);
    }
    return procName;
}

pid_t GetPidByName(const std::string& processName)
{
    pid_t pid = -1;
    std::string cmd = "pidof " + processName;

    FILE* fp = popen(cmd.c_str(), "r");
    if (fp != nullptr) {
        char buffer[BUF_SIZE_256] = {'\0'};
        while (fgets(buffer, sizeof(buffer) - 1, fp) != nullptr) {}
        std::istringstream istr(buffer);
        istr >> pid;
        pclose(fp);
    }
    return pid;
}

bool IsPidExist(pid_t pid)
{
    std::string procDir = "/proc/" + std::to_string(pid);
    return FileUtil::IsDirectory(procDir);
}

bool IsSpecificCmdExist(const std::string& fullPath)
{
    return access(fullPath.c_str(), X_OK) == 0;
}

int WriteCommandResultToFile(int fd, const std::string &cmd, const std::vector<std::string> &args)
{
    if (cmd.empty()) {
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        return -1;
    } else if (pid == 0) {
        // follow standard, although dup2 may handle the case of invalid oldfd
        if (fd >= 0) {
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDIN_FILENO);
            dup2(fd, STDERR_FILENO);
        }

        std::vector<char *> argv;
        for (const auto &arg : args) {
            argv.push_back(const_cast<char *>(arg.c_str()));
        }
        argv.push_back(0);
        execv(cmd.c_str(), &argv[0]);
    }

    constexpr uint64_t maxWaitingTime = 120; // 120 seconds
    uint64_t endTime = TimeUtil::GetSteadyClockTimeMs() + maxWaitingTime * TimeUtil::SEC_TO_MILLISEC;
    while (endTime > TimeUtil::GetSteadyClockTimeMs()) {
        int status = 0;
        pid_t p = waitpid(pid, &status, WNOHANG);
        if (p < 0) {
            return -1;
        }

        if (p == pid) {
            return WEXITSTATUS(status);
        }
    }

    return -1;
}

int32_t GetTransformedUid(int32_t uid)
{
    return uid / UID_TRANSFORM_DIVISOR;
}
}
} // namespace HiviewDFX
} // namespace OHOS
