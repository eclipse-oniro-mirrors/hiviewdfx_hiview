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

void ShellCatcher::DoChildProcess(int writeFd)
{
    if (writeFd < 0 || dup2(writeFd, STDOUT_FILENO) == -1 ||
        dup2(writeFd, STDIN_FILENO) == -1 || dup2(writeFd, STDERR_FILENO) == -1) {
        HIVIEW_LOGE("dup2 writeFd fail");
        _exit(-1);
    }

    int ret = -1;
#ifdef HILOG_CATCHER_ENABLE
    ret = DoHilogCatcher(writeFd);
#endif // HILOG_CATCHER_ENABLE

#ifdef USAGE_CATCHER_ENABLE
    ret = DoUsageCatcher(writeFd);
#endif // USAGE_CATCHER_ENABLE

#ifdef SCB_CATCHER_ENABLE
    ret = DoScbCatcher(writeFd);
#endif // SCB_CATCHER_ENABLE

#ifdef OTHER_CATCHER_ENABLE
    ret = DoOtherCatcher(writeFd);
#endif // OTHER_CATCHER_ENABLE
    if (ret < 0) {
        HIVIEW_LOGE("execl %{public}d, errno: %{public}d", ret, errno);
        _exit(-1);
    }
}

#ifdef HILOG_CATCHER_ENABLE
int ShellCatcher::DoHilogCatcher(int writeFd)
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
        case CATCHER_INPUT_EVENT_HILOG:
            ret = execl("/system/bin/hilog", "hilog", "-T", "InputKeyFlow", "-e",
                std::to_string(pid_).c_str(), "-x", nullptr);
            break;
        case CATCHER_INPUT_HILOG:
            ret = execl("/system/bin/hilog", "hilog", "-T", "InputKeyFlow", "-x", nullptr);
            break;
        case CATCHER_TAGHILOG:
            ret = execl("/system/bin/hilog",
                "hilog",
                "-T",
                "PowerState,PowerSuspend,PowerInput,DisplayState,DfxFaultLogger",
                "-x",
                nullptr);
            break;
        default:
            break;
    }
    return ret;
}
#endif // HILOG_CATCHER_ENABLE

#ifdef USAGE_CATCHER_ENABLE
int ShellCatcher::DoUsageCatcher(int writeFd)
{
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
        case CATCHER_PMS:
            ret = execl("/system/bin/hidumper", "hidumper", "-s", "PowerManagerService", "-a", "-s", nullptr);
            break;
        case CATCHER_DPMS:
            ret = execl("/system/bin/hidumper", "hidumper", "-s", "DisplayPowerManagerService", nullptr);
            break;
        case CATCHER_RS:
            ret = execl("/system/bin/hidumper", "hidumper", "-s", "RenderService", "-a", "allInfo", nullptr);
            break;
        case CATCHER_DAM:
            ret = execl("/system/bin/hidumper", "hidumper", "-s", "1910", "-a", "DumpAppMap", nullptr);
            break;
        default:
            break;
    }
    return ret;
}
#endif // USAGE_CATCHER_ENABLE

#ifdef SCB_CATCHER_ENABLE
int ShellCatcher::DoScbCatcher(int writeFd)
{
    int ret = -1;
    switch (catcherType_) {
        case CATCHER_SCBWMS:
        case CATCHER_SCBWMSEVT:
            {
                std::string cmdSuffix = (catcherType_ == CATCHER_SCBWMS) ? " -simplify" : " -event";
                std::string cmd = "-w " + focusWindowId_ + cmdSuffix;
                ret = execl("/system/bin/hidumper", "hidumper", "-s", "WindowManagerService", "-a",
                    cmd.c_str(), nullptr);
            }
            break;
        case CATCHER_SCBSESSION:
        case CATCHER_SCBVIEWPARAM:
            {
                std::string cmd = (catcherType_ == CATCHER_SCBSESSION) ? "-b SCBScenePanel getContainerSession" :
                    "-b SCBScenePanel getViewParam";
                ret = execl("/system/bin/hidumper", "hidumper", "-s", "4606", "-a", cmd.c_str(), nullptr);
            }
            break;
        case CATCHER_SCBWMSV:
            {
                std::string cmd = "-v all -simplify";
                ret = execl("/system/bin/hidumper", "hidumper", "-s", "WindowManagerService", "-a",
                    cmd.c_str(), nullptr);
            }
        default:
            break;
    }
    return ret;
}
#endif // SCB_CATCHER_ENABLE

#ifdef OTHER_CATCHER_ENABLE
int ShellCatcher::DoOtherCatcher(int writeFd)
{
    int ret = -1;
    switch (catcherType_) {
        case CATCHER_SNAPSHOT:
            {
                std::string path = "/data/log/eventlog/snapshot_display_";
                path += TimeUtil::TimestampFormatToDate(TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC,
                    "%Y%m%d%H%M%S");
                path += ".jpeg";
                ret = execl("/system/bin/snapshot_display", "snapshot_display", "-f", path.c_str(), nullptr);
            }
            break;
        case CATCHER_EEC:
            {
                std::string cmd = "-b EventExclusiveCommander getAllEventExclusiveCaller";
                ret = execl("/system/bin/hidumper", "hidumper", "-s", "4606", "-a", cmd.c_str(), nullptr);
            }
            break;
        case CATCHER_GEC:
            {
                std::string cmd = "-b SCBGestureManager getAllGestureEnableCaller";
                ret = execl("/system/bin/hidumper", "hidumper", "-s", "4606", "-a", cmd.c_str(), nullptr);
            }
            break;
        case CATCHER_UI:
            {
                std::string cmd = "-p 0";
                ret = execl("/system/bin/hidumper", "hidumper", "-s", "4606", "-a", cmd.c_str(), nullptr);
            }
            break;
        case CATCHER_MMI:
            ret = execl("/system/bin/hidumper", "hidumper", "-s", "MultimodalInput", "-a", "-w", nullptr);
            break;
        case CATCHER_DMS:
            ret = execl("/system/bin/hidumper", "hidumper", "-s", "DisplayManagerService", "-a", "-a", nullptr);
            break;
        default:
            ret = DoOtherChildProcesscatcher(writeFd);
            break;
    }
    return ret;
}
#endif // OTHER_CATCHER_ENABLE

int ShellCatcher::DoOtherChildProcesscatcher(int writeFd)
{
    int ret = -1;
    switch (catcherType_) {
        case CATCHER_TAGHILOG:
            ret = execl("/system/bin/hilog",
                "hilog",
                "-T",
                "PowerState,PowerSuspend,PowerInput,DisplayState,DfxFaultLogger",
                nullptr);
            break;
        default:
            break;
    }
    return ret;
}

void ShellCatcher::SetFocusWindowId(const std::string& focusWindowId)
{
    focusWindowId_ = focusWindowId;
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
