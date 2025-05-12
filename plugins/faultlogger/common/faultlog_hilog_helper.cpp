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

#include <poll.h>
#include <sys/syscall.h>
#include <sys/wait.h>

#include "constants.h"
#include "hiview_logger.h"
#include "parameter_ex.h"

// define Fdsan Domain
#ifndef FDSAN_DOMAIN
#undef FDSAN_DOMAIN
#endif
#define FDSAN_DOMAIN 0xD002D11

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");
using namespace FaultLogger;
namespace {
constexpr int READ_HILOG_BUFFER_SIZE = 1024;
constexpr int NUMBER_ONE_THOUSAND = 1000;
constexpr int NUMBER_ONE_MILLION = 1000 * 1000;
constexpr uint64_t MAX_HILOG_TIMEOUT = 15 * 1000;

uint64_t GetTimeMilliseconds(void)
{
    struct timespec ts;
    (void)clock_gettime(CLOCK_REALTIME, &ts);
    return (static_cast<uint64_t>(ts.tv_sec) * NUMBER_ONE_THOUSAND) +
        (static_cast<uint64_t>(ts.tv_nsec) / NUMBER_ONE_MILLION);
}
}

std::string FaultlogHilogHelper::ReadHilogTimeout(int fd, uint64_t timeout)
{
    if (fd < 0 || timeout > MAX_HILOG_TIMEOUT) {
        HIVIEW_LOGE("Invalid fd or timeout");
        return "";
    }
    uint64_t startTime = GetTimeMilliseconds();
    uint64_t endTime = startTime + timeout;

    struct pollfd pfds[1];
    pfds[0].fd = fd;
    pfds[0].events = POLLIN;

    std::string log;
    int pollRet = -1;
    bool isReadDone = false;
    while (!isReadDone) {
        uint64_t now = GetTimeMilliseconds();
        if (now >= endTime || now < startTime) {
            HIVIEW_LOGI("read hilog timeout.");
            return log;
        } else {
            timeout = endTime - now;
        }

        pollRet = poll(pfds, 1, timeout);
        if (pollRet < 0) {
            if (errno == EINTR) {
                continue;
            }
            return log;
        }

        if ((pollRet > 0) && (pfds[0].revents & POLLIN)) {
            char buffer[READ_HILOG_BUFFER_SIZE] = {0};
            ssize_t nread = TEMP_FAILURE_RETRY(read(fd, buffer, sizeof(buffer) - 1));
            if (nread < 0) {
                HIVIEW_LOGI("read failed. errno: %{public}d", errno);
                return log;
            }
            if (nread == 0) {
                isReadDone = true;
                HIVIEW_LOGI("read hilog finished");
                return log;
            }
            log.append(buffer, nread);
        }
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
    uint64_t ownerTag = fdsan_create_owner_tag(FDSAN_OWNER_TYPE_FILE, FDSAN_DOMAIN);
    fdsan_exchange_owner_tag(fds[0], 0, ownerTag);
    fdsan_exchange_owner_tag(fds[1], 0, ownerTag);

    int childPid = fork();
    if (childPid < 0) {
        HIVIEW_LOGE("fork fail");
        return "";
    } else if (childPid == 0) {
        syscall(SYS_close, fds[0]);
        int rc = DoGetHilogProcess(pid, fds[1]);
        fdsan_close_with_tag(fds[1], ownerTag);
        _exit(rc);
    } else {
        fdsan_close_with_tag(fds[1], ownerTag);
        HIVIEW_LOGI("read hilog start");
        std::string log = ReadHilogTimeout(fds[0]);
        fdsan_close_with_tag(fds[0], ownerTag);

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
