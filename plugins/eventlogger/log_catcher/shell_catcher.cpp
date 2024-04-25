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
#include "hiview_logger.h"
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

bool ShellCatcher::Initialize(const std::string& cmd, int type, int catcherPid)
{
    catcherCmd_ = cmd;
    catcherType_ = CATCHER_TYPE(type);
    pid_ = catcherPid;
    description_ = "catcher cmd: " + catcherCmd_ + " ";
    return true;
}

void ShellCatcher::SetEvent(std::shared_ptr<SysEvent> event)
{
    event_ = event;
}

int ShellCatcher::CaDoInChildProcesscatcher(int writeFd)
{
    int ret = -1;
    switch (catcherType_) {
        case CATCHER_HILOG:
            ret = execl("/system/bin/hilog", "hilog", "-x", nullptr);
            break;
        case CATCHER_LIGHT_HILOG:
            ret = execl("/system/bin/hilog", "hilog", "-z", "1000", "-P", std::to_string(pid_).c_str(),
                nullptr);
            break;
        case CATCHER_DAM:
            ret = execl("/system/bin/hidumper", "hidumper", "-s", "1910", "-a", "DumpAppMap", nullptr);
            break;
        case CATCHER_SCBWMS:
            {
                if (event_ == nullptr) {
                    HIVIEW_LOGI("event is null");
                    break;
                }
                int32_t windowId = event_->GetEventIntValue("FOCUS_WINDOW");
                std::string cmd = "-w " + std::to_string(windowId) + " -default";
                ret = execl("/system/bin/hidumper", "hidumper", "-s", "WindowManagerService", "-a",
                    cmd.c_str(), nullptr);
            }
            break;
        case CATCHER_SNAPSHOT:
            {
                std::string path = "/data/log/eventlog/snapshot_display_";
                path += TimeUtil::TimestampFormatToDate(TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC,
                    "%Y%m%d%H%M%S");
                path += ".jpeg";
                ret = execl("/system/bin/snapshot_display", "snapshot_display", "-f", path.c_str(), nullptr);
            }
            break;
        case CATCHER_SCBSESSION:
            ret = execl("/system/bin/scb_debug", "scb_debug", "SCBScenePanel", "getContainerSession", nullptr);
            break;
        case CATCHER_SCBVIEWPARAM:
            ret = execl("/system/bin/scb_debug", "scb_debug", "SCBScenePanel", "getViewParam", nullptr);
            break;
        default:
            break;
    }
    return ret;
}

void ShellCatcher::DoChildProcess(int writeFd)
{
    if (writeFd < 0 || dup2(writeFd, STDOUT_FILENO) == -1 ||
        dup2(writeFd, STDIN_FILENO) == -1 || dup2(writeFd, STDERR_FILENO) == -1) {
        HIVIEW_LOGE("dup2 writeFd fail");
        _exit(-1);
    }

    int ret = -1;
    switch (catcherType_) {
        case CATCHER_AMS:
            ret = execl("/system/bin/hidumper", "hidumper", "-s", "AbilityManagerService", "-a", "-a", nullptr);
            break;
        case CATCHER_WMS:
            ret = execl("/system/bin/hidumper", "hidumper", "-s", "WindowManagerService", "-a", "-a", nullptr);
            break;
        case CATCHER_CPU:
            ret = execl("/system/bin/hidumper", "hidumper", "--cpuusage", nullptr);
            break;
        case CATCHER_MEM:
            ret = execl("/system/bin/hidumper", "hidumper", "--mem", std::to_string(pid_).c_str(), nullptr);
            break;
        case CATCHER_PMS:
            ret = execl("/system/bin/hidumper", "hidumper", "-s", "PowerManagerService", "-a", "-s", nullptr);
            break;
        case CATCHER_DPMS:
            ret = execl("/system/bin/hidumper", "hidumper", "-s", "DisplayPowerManagerService", nullptr);
            break;
        case CATCHER_RS:
            ret = execl("/system/bin/hidumper", "hidumper", "-s", "RenderService", "-a", "allInfo", nullptr);
            break;
        default:
            ret = CaDoInChildProcesscatcher(writeFd);
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

int ShellCatcher::Catch(int fd, int jsonFd)
{
    auto originSize = GetFdSize(fd);
    if (catcherCmd_.empty()) {
        HIVIEW_LOGE("catcherCmd empty");
        return -1;
    }

    ReadShellToFile(fd, catcherCmd_);
    logSize_ = GetFdSize(fd) - originSize;
    return logSize_;
}
} // namespace HiviewDFX
} // namespace OHOS
