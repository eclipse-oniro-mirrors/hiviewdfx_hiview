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
#include "shell_catcher.h"
#include <unistd.h>
#include <sys/wait.h>
#include "dump_client_main.h"
#include "logger.h"
#include "common_utils.h"
#include "log_catcher_utils.h"
#include "securec.h"
#include "time_util.h"
namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D01, "EventLogger-ShellCatcher");
ShellCatcher::ShellCatcher() : EventLogCatcher()
{
    name_ = "ShellCatcher";
}

bool ShellCatcher::Initialize(const std::string& cmd, int type, int intParam __UNUSED)
{
    catcherCmd = cmd;
    catcherType = CATCHER_TYPE(type);
    description_ = "catcher cmd: " + catcherCmd + " ";
    return true;
}

void ShellCatcher::DoChildProcess(int writeFd)
{
    if (writeFd < 0 || dup2(writeFd, STDOUT_FILENO) == -1 ||
        dup2(writeFd, STDIN_FILENO) == -1 || dup2(writeFd, STDERR_FILENO) == -1) {
        HIVIEW_LOGE("dup2 writeFd fail");
        _exit(-1);
    }

    int ret = 0;
    switch (catcherType) {
        case CATCHER_HILOG:
            ret = execl("/system/bin/hilog", "hilog", "-x", nullptr);
            break;
        case CATCHER_HITRACE:
            ret = execl("/system/bin/hitrace", "hitrace", nullptr);
            break;
        default:
            break;        
    }
    if (ret < 0) {
        HIVIEW_LOGE("execl %{public}d, errno: %{public}d", ret, errno);
        _exit(-1);
    }
}

bool ShellCatcher::ReadShellToFile(int writeFd, const std::string& cmd)
{
    int childPid = fork();
    if (childPid < 0) {
        HIVIEW_LOGE("fork fail");
        return false;
    } else if (childPid == 0) {
        DoChildProcess(writeFd);
    } else {
        if (waitpid(childPid, nullptr, 0) != childPid) {
            HIVIEW_LOGE("waitpid fail, pid: %{public}d, errno: %{public}d", childPid, errno);
            return false;
        }
        HIVIEW_LOGI("waitpid %{public}d success", childPid);
    }
    return true;
}

int ShellCatcher::Catch(int fd)
{
    auto originSize = GetFdSize(fd);
    if (catcherCmd.empty()) {
        HIVIEW_LOGE("catcherCmd empty");
        return -1;
    }

    ReadShellToFile(fd, catcherCmd);
    logSize_ = GetFdSize(fd) - originSize;
    return logSize_;
}
} // namespace HiviewDFX
} // namespace OHOS