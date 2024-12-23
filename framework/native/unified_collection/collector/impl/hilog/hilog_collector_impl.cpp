/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "hilog_collector_impl.h"
#include <sys/wait.h>
#include <unistd.h>
#include "hiview_logger.h"
#include "hilog_decorator.h"

namespace OHOS::HiviewDFX::UCollectUtil {
DEFINE_LOG_TAG("HilogCollector");

std::shared_ptr<HilogCollector> HilogCollector::Create()
{
    return std::make_shared<HilogDecorator>(std::make_shared<HilogCollectorImpl>());
}

void HilogCollectorImpl::ExecuteHilog(int32_t pid, uint32_t lineCount, int writeFd) const
{
    if (writeFd < 0 || dup2(writeFd, STDOUT_FILENO) == -1 || dup2(writeFd, STDERR_FILENO) == -1) {
        HIVIEW_LOGE("dup2 writeFd fail");
        _exit(EXIT_FAILURE);
    }

    int ret = execl("/system/bin/hilog", "hilog", "-z", std::to_string(lineCount).c_str(),
        "-P", std::to_string(pid).c_str(), nullptr);
    close(writeFd);
    if (ret < 0) {
        HIVIEW_LOGE("execl %{public}d, errno: %{public}d", ret, errno);
        _exit(EXIT_FAILURE);
    }
    _exit(EXIT_SUCCESS);
}

void HilogCollectorImpl::ReadHilog(int readFd, std::string& log) const
{
    constexpr int READ_HILOG_BUFFER_SIZE = 1024;
    while (true) {
        char buffer[READ_HILOG_BUFFER_SIZE] = {0};
        ssize_t nread = read(readFd, buffer, sizeof(buffer) - 1);
        if (nread <= 0) {
            HIVIEW_LOGI("read hilog finished");
            break;
        }
        log.append(buffer);
    }
    close(readFd);
}

CollectResult<std::string> HilogCollectorImpl::CollectLastLog(uint32_t pid, uint32_t lineNum)
{
    CollectResult<std::string> result;
    result.retCode = UCollect::SYSTEM_ERROR;
    int fds[2] = {-1, -1}; // 2: one read pipe, one write pipe
    if (pipe(fds) != 0) {
        HIVIEW_LOGE("pipe fail.");
        return result;
    }

    int childPid = fork();
    if (childPid < 0) {
        HIVIEW_LOGE("fork fail.");
        return result;
    }

    if (childPid == 0) {
        close(fds[0]);
        constexpr uint32_t MAX_LINE_NUM = 10000;
        ExecuteHilog(pid, lineNum > MAX_LINE_NUM ? MAX_LINE_NUM : lineNum, fds[1]);
    } else {
        close(fds[1]);
        ReadHilog(fds[0], result.data);
        result.retCode = UCollect::SUCCESS;
        if (waitpid(childPid, nullptr, 0) != childPid) {
            HIVIEW_LOGE("waitpid fail, pid: %{public}d, errno: %{public}d", childPid, errno);
        } else {
            HIVIEW_LOGI("waitpid %{public}d success", childPid);
        }
    }
    return result;
}
} // namespace OHOS::HiviewDFX::UCollectUtil