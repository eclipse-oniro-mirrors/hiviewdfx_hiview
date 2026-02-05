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
#include "open_stacktrace_catcher.h"

#include <ctime>
#include <string>

#include "common_utils.h"
#include "file_util.h"
#include "log_catcher_utils.h"
#include "hiview_logger.h"
#include "securec.h"
#include <sys/wait.h>
#include <unistd.h>

namespace OHOS {
namespace HiviewDFX {
#ifdef STACKTRACE_CATCHER_ENABLE
namespace {
    static const inline int SLEEP_TIME_US = 100000;
    static const inline int MAX_RETRY_COUNT = 10;
}
DEFINE_LOG_LABEL(0xD002D01, "EventLogger-OpenStacktraceCatcher");
OpenStacktraceCatcher::OpenStacktraceCatcher() : EventLogCatcher()
{
    name_ = "OpenStacktraceCatcher";
}

bool OpenStacktraceCatcher::Initialize(const std::string& packageNam, int pid, int intParam)
{
    if (pid <= 0 && packageNam.length() == 0) {
        description_ = "OpenStacktraceCatcher -- pid is null, packageName is null\n";
        return false;
    }
    packageName_ = packageNam;
    if (pid > 0) {
        pid_ = pid;
    } else {
        pid_ = CommonUtils::GetPidByName(packageNam);
    }

    if (pid_ <= 0) {
        description_ = "OpenStacktraceCatcher -- packageName is " + packageName_ + " pid is null\n";
        return false;
    }

    if (packageName_.length() == 0) {
        packageName_ = "(null)";
    }

    description_ = "OpenStacktraceCatcher -- pid==" + std::to_string(pid_) + " packageName is " + packageName_ + "\n";
    return EventLogCatcher::Initialize(packageName_, pid_, intParam);
};

// may block, run in another thread
int OpenStacktraceCatcher::Catch(int fd, int jsonFd)
{
    if (pid_ <= 0) {
        return 0;
    }
    int originSize = GetFdSize(fd);

#ifdef DUMP_STACK_IN_PROCESS
    std::string threadStack;
    LogCatcherUtils::TerminalBinderInfo binderInfo{0, 0, packageName_};
    LogCatcherUtils::DumpStacktrace(fd, pid_, threadStack, binderInfo);
#else
    ForkAndDumpStackTrace(fd);
#endif
    logSize_ = GetFdSize(fd) - originSize;
    return logSize_;
}

inline void OpenStacktraceCatcher::WaitChildPid(pid_t pid)
{
    int retryCount = 0;
    while (retryCount < MAX_RETRY_COUNT) {
        if (waitpid(pid, NULL, WNOHANG) != 0) {
            break;
        }
        retryCount++;
        usleep(SLEEP_TIME_US);
    }
}

int32_t OpenStacktraceCatcher::ForkAndDumpStackTrace(int32_t fd)
{
    int pid = -1;
    int leftTimeMicroSecond = 30000000; // 30000000us
    if ((pid = fork()) < 0) {
        HIVIEW_LOGE("Fork error, err:%{public}d", errno);
        return 0;
    }

    if (pid == 0) {
        auto newFd = dup(fd);
        std::string threadStack;
        LogCatcherUtils::TerminalBinderInfo binderInfo{0, 0, packageName_};
        int ret = LogCatcherUtils::DumpStacktrace(newFd, pid_, threadStack, binderInfo);
        HIVIEW_LOGD("LogCatcherUtils::DumpStacktrace ret %{public}d", ret);
        close(newFd);
        _exit(ret);
    }

    while (true) {
        int status = 0;
        pid_t p = waitpid(pid, &status, WNOHANG);
        if (p < 0) {
            HIVIEW_LOGW("Waitpid return p=%{public}d, err:%{public}d", p, errno);
            return -1;
        }

        if (p == pid) {
            HIVIEW_LOGD("Dump process exited status is %{public}d", status);
            return WEXITSTATUS(status);
        }

        if (needStop_ || leftTimeMicroSecond <= 0) {
            HIVIEW_LOGW("Dump stacktrace timeout, killing pid %{public}d.", pid);
            std::string str = "Dump stacktrace timeout, Catch for " + std::to_string(pid_);
            FileUtil::SaveStringToFd(fd, str);
            kill(pid, SIGKILL);
            WaitChildPid(pid);
            return -1;
        }

        usleep(SLEEP_TIME_US); // poll every 0.1 sec
        leftTimeMicroSecond -= SLEEP_TIME_US;
    }
}
#endif // STACKTRACE_CATCHER_ENABLE
} // namespace HiviewDFX
} // namespace OHOS
