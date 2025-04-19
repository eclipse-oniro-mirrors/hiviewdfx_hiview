/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "faultlog_hilog_helper.h"

#include <unistd.h>

#include <sys/syscall.h>
#include <sys/wait.h>

#include "constants.h"
#include "hiview_logger.h"
#include "parameter_ex.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");
using namespace FaultLogger;
namespace {
constexpr int READ_HILOG_BUFFER_SIZE = 1024;
}

std::string FaultlogHilogHelper::ReadHilogTimeout(int fd)
{
    fd_set readFds;
    constexpr int readTimeout = 5;
    struct timeval timeout = {0};
    time_t startTime = time(nullptr);
    bool isReadDone = false;
    std::string log;
    while (!isReadDone) {
        time_t now = time(nullptr);
        if (now >= startTime + readTimeout) {
            HIVIEW_LOGI("read hilog timeout.");
            isReadDone = true;
            return log;
        }
        timeout.tv_sec = startTime + readTimeout - now;
        timeout.tv_usec = 0;

        FD_ZERO(&readFds);
        FD_SET(fd, &readFds);
        int ret = select(fd + 1, &readFds, nullptr, nullptr, &timeout);
        if (ret <= 0) {
            HIVIEW_LOGE("select failed: %{public}d, errno: %{public}d", ret, errno);
            if (errno == EINTR) {
                continue;
            }
            isReadDone = true;
            return log;
        }

        char buffer[READ_HILOG_BUFFER_SIZE] = {0};
        ssize_t nread = TEMP_FAILURE_RETRY(read(fd, buffer, sizeof(buffer) - 1));
        if (nread == 0) {
            HIVIEW_LOGI("read hilog finished");
            isReadDone = true;
            break;
        } else if (nread < 0) {
            HIVIEW_LOGI("read failed. errno: %{public}d", errno);
            isReadDone = true;
            break;
        }
        log.append(buffer, nread);
    }
    return log;
}

std::string FaultlogHilogHelper::GetHilogByPid(int32_t pid)
{
    if (Parameter::IsOversea() && !Parameter::IsBetaVersion()) {
        HIVIEW_LOGI("Do not get hilog in oversea commercial version.");
        return "";
    }
    int fds[2] = {-1, -1}; // 2: one read pipe, one write pipe
    if (pipe(fds) != 0) {
        HIVIEW_LOGE("Failed to create pipe for get log.");
        return "";
    }
    int childPid = fork();
    if (childPid < 0) {
        HIVIEW_LOGE("fork fail");
        return "";
    } else if (childPid == 0) {
        syscall(SYS_close, fds[0]);
        int rc = DoGetHilogProcess(pid, fds[1]);
        syscall(SYS_close, fds[1]);
        _exit(rc);
    } else {
        syscall(SYS_close, fds[1]);
        HIVIEW_LOGI("read hilog start");
        std::string log = ReadHilogTimeout(fds[0]);
        syscall(SYS_close, fds[0]);

        if (TEMP_FAILURE_RETRY(waitpid(childPid, nullptr, 0)) != childPid) {
            HIVIEW_LOGE("waitpid fail, pid: %{public}d, errno: %{public}d", childPid, errno);
            return "";
        }
        HIVIEW_LOGI("get hilog waitpid %{public}d success", childPid);
        return log;
    }
    return "";
}

int FaultlogHilogHelper::DoGetHilogProcess(int32_t pid, int writeFd)
{
    HIVIEW_LOGD("Start do get hilog process, pid:%{public}d", pid);
    if (writeFd < 0 || dup2(writeFd, STDOUT_FILENO) == -1 ||
        dup2(writeFd, STDERR_FILENO) == -1) {
        HIVIEW_LOGE("dup2 writeFd fail");
        return -1;
    }

    int ret = -1;
    ret = execl("/system/bin/hilog", "hilog", "-z", "1000", "-P", std::to_string(pid).c_str(), nullptr);
    if (ret < 0) {
        HIVIEW_LOGE("execl %{public}d, errno: %{public}d", ret, errno);
        return ret;
    }
    return 0;
}

Json::Value FaultlogHilogHelper::ParseHilogToJson(const std::string &hilogStr)
{
    Json::Value hilog(Json::arrayValue);
    if (hilogStr.empty()) {
        HIVIEW_LOGE("Get hilog is empty");
        return hilog;
    }
    std::stringstream logStream(hilogStr);
    std::string oneLine;
    for (int count = 0; count < REPORT_HILOG_LINE && getline(logStream, oneLine); count++) {
        hilog.append(oneLine);
    }
    return hilog;
}
} // namespace HiviewDFX
} // namespace OHOS
