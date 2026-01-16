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
#include <regex>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <vector>

#include "hiview_logger.h"
#include "securec.h"
#include "time_util.h"
#include "string_util.h"

using namespace std;
namespace OHOS {
namespace HiviewDFX {
namespace CommonUtils {
DEFINE_LOG_TAG("CommonUtils");
namespace {
constexpr int32_t UID_TRANSFORM_DIVISOR = 200000;
constexpr int32_t MAX_RETRY_COUNT = 20;
constexpr int32_t WAIT_CHILD_PROCESS_INTERVAL = 5 * 1000; // 5ms
constexpr char EXPORT_FILE_REGEX[] = "[0-9]{1,}(.*)";
constexpr char UNDERLINE[] = "_";
constexpr size_t FORMAT_DATE_LEN = 14;

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

pid_t KillChildPid(pid_t pid, int signal)
{
    kill(pid, signal);
    int32_t retryCount = MAX_RETRY_COUNT;
    pid_t ret = waitpid(pid, nullptr, WNOHANG);
    while (retryCount > 0 && (ret == 0)) {
        usleep(WAIT_CHILD_PROCESS_INTERVAL);
        retryCount--;
        ret = waitpid(pid, nullptr, WNOHANG);
    }
    HIVIEW_LOGI("pid = %{public}d, signal = %{public}d, ret = %{public}d", pid, signal, ret);
    return ret;
}

std::string GetTimeFromFileName(const std::string& fileName, const std::string& prefix, const std::string& pidStr)
{
    std::string fileTimeSub = StringUtil::GetRightSubstr(fileName, prefix);
    if (!pidStr.empty()) {
        // pid_yyyymmddHHMMSS.txt
        fileTimeSub = StringUtil::GetRightSubstr(fileTimeSub, UNDERLINE);
    }
    return fileTimeSub;
}

int GetFileNameNum(const std::string& fileName, const std::string& ext)
{
    int ret = 0;
    auto startPos = fileName.find(UNDERLINE);
    if (startPos == std::string::npos) {
        return ret;
    }
    auto endPos = fileName.find(ext);
    if (endPos == std::string::npos) {
        return ret;
    }
    if (endPos <= startPos + 1) {
        return ret;
    }
    return StringUtil::StrToInt(fileName.substr(startPos + 1, endPos - startPos - 1));
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
        HIVIEW_LOGE("fork failed");
        return -1;
    } else if (pid == 0) {
        // follow standard, although dup2 may handle the case of invalid oldfd
        if (fd < 0 || dup2(fd, STDOUT_FILENO) == -1 || dup2(fd, STDIN_FILENO) == -1 || dup2(fd, STDERR_FILENO) == -1) {
            HIVIEW_LOGE("dup failed %{public}d", errno);
            _exit(EXIT_FAILURE);
        }

        std::vector<char *> argv;
        for (const auto &arg : args) {
            argv.push_back(const_cast<char *>(arg.c_str()));
        }
        argv.push_back(0);
        if (execv(cmd.c_str(), &argv[0]) < 0) {
            HIVIEW_LOGE("execv failed %{public}d", errno);
            _exit(EXIT_FAILURE);
        }
        _exit(EXIT_SUCCESS);
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
    HIVIEW_LOGE("wait timeout pid = %{public}d", pid);
    pid_t ret = KillChildPid(pid, SIGUSR1);
    if (ret == pid || ret < 0) {
        return -1;
    }
    // kill SIGUSR1 child process exit timeout, use kill SIGKILL
    (void)KillChildPid(pid, SIGKILL);
    return -1;
}

int32_t GetTransformedUid(int32_t uid)
{
    return uid / UID_TRANSFORM_DIVISOR;
}

void GetDirRegexFiles(const std::string& path, const std::string& prefix,
    std::vector<std::string>& files, const std::string& pidStr)
{
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr) {
        HIVIEW_LOGE("failed to open dir=%{public}s", path.c_str());
        return;
    }
    std::regex reg = std::regex(prefix + EXPORT_FILE_REGEX);
    struct dirent* ptr = nullptr;
    while ((ptr = readdir(dir)) != nullptr) {
        if (ptr->d_type == DT_REG) {
            if (regex_match(ptr->d_name, reg)) {
                files.push_back(FileUtil::IncludeTrailingPathDelimiter(path) + std::string(ptr->d_name));
            }
        }
    }
    closedir(dir);
    std::sort(files.begin(), files.end(), [&prefix, &pidStr](const std::string& file1, const std::string& file2) {
        std::string fileTimeSub1 = GetTimeFromFileName(file1, prefix, pidStr);
        std::string fileTimeSub2 = GetTimeFromFileName(file2, prefix, pidStr);
        if (fileTimeSub1.substr(0, FORMAT_DATE_LEN) == fileTimeSub2.substr(0, FORMAT_DATE_LEN) &&
            fileTimeSub1.size() != fileTimeSub2.size()) { // compare yyyymmddHHMMSS_1.txt and yyyymmddHHMMSS_10.txt
            return fileTimeSub1.size() < fileTimeSub2.size();
        }
        return fileTimeSub1 < fileTimeSub2;
    });
}

std::string CreateExportFile(const std::string& path, int32_t maxFileNum, const std::string& prefix,
    const std::string& ext, const std::string& pidStr)
{
    if (!FileUtil::IsDirectory(path) && !FileUtil::ForceCreateDirectory(path)) {
        HIVIEW_LOGE("failed to create dir=%{public}s", path.c_str());
        return "";
    }

    std::vector<std::string> files;
    GetDirRegexFiles(path, prefix, files, pidStr);
    if (files.size() >= static_cast<size_t>(maxFileNum)) {
        for (size_t index = 0; index <= files.size() - static_cast<size_t>(maxFileNum); ++index) {
            HIVIEW_LOGI("remove file=%{public}s", FileUtil::ExtractFileName(files[index]).c_str());
            (void)FileUtil::RemoveFile(files[index]);
        }
    }

    uint64_t fileTime = TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC;
    std::string timeFormat = TimeUtil::TimestampFormatToDate(fileTime, "%Y%m%d%H%M%S");
    // file name e.g. prefix_[pid_]yyyymmddHHMMSS.txt
    std::string fileName;
    fileName.append(FileUtil::IncludeTrailingPathDelimiter(path)).append(prefix).append(pidStr).append(timeFormat);
    if (!files.empty()) {
        auto startPos = files.back().find(timeFormat);
        if (startPos != std::string::npos) {
            int fileNameNum = GetFileNameNum(files.back().substr(startPos), ext); // yyyymmddHHMMSS_1.txt
            fileName.append(UNDERLINE).append(std::to_string(++fileNameNum));
        }
    }
    fileName.append(ext);
    (void)FileUtil::CreateFile(fileName);
    HIVIEW_LOGI("create file=%{public}s", FileUtil::ExtractFileName(fileName).c_str());
    return fileName;
}
}
} // namespace HiviewDFX
} // namespace OHOS
