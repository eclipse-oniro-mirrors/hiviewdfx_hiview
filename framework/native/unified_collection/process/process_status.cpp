/*
 * Copyright (C) 2023-2024 Huawei Device Co., Ltd.
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
#include "process_status.h"

#include "common_utils.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "time_util.h"
#include "unified_collection_data.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
DEFINE_LOG_TAG("ProcessStatus");
namespace {
constexpr uint64_t INVALID_LAST_FOREGROUND_TIME = 0;
}

std::string ProcessStatus::GetProcessName(int32_t pid)
{
    std::unique_lock<ffrt::mutex> lock(mutex_);
    // the cleanup judgment is triggered each time
    if (NeedClearProcessInfos()) {
        ClearProcessInfos();
    }

    if (processInfos_.find(pid) != processInfos_.end() && !processInfos_[pid].name.empty()) {
        return processInfos_[pid].name;
    }
    std::string procName = CommonUtils::GetProcFullNameByPid(pid);
    if (UpdateProcessName(pid, procName)) {
        return procName;
    }
    HIVIEW_LOGD("failed to get proc name from pid=%{public}d", pid);
    return "";
}

bool ProcessStatus::NeedClearProcessInfos()
{
    if (processInfos_.size() <= capacity_) {
        return false;
    }
    static uint64_t lastClearTime = 0;
    uint64_t now = TimeUtil::GetSteadyClockTimeMs();
    uint64_t interval = now > lastClearTime ? (now - lastClearTime) : 0;
    constexpr uint32_t clearInterval = 600 * 1000; // 10min
    if (interval <= clearInterval) {
        return false;
    }
    lastClearTime = now;
    return true;
}

void ProcessStatus::ClearProcessInfos()
{
    HIVIEW_LOGI("start to clear process cache, size=%{public}zu, capacity=%{public}u", processInfos_.size(), capacity_);
    for (auto it = processInfos_.begin(); it != processInfos_.end();) {
        if (!CommonUtils::IsPidExist(it->first) || CommonUtils::GetProcFullNameByPid(it->first) != it->second.name) {
            it = processInfos_.erase(it);
            continue;
        }
        it++;
    }
    constexpr uint32_t reservedNum = 100;
    capacity_ = processInfos_.size() + reservedNum;
    HIVIEW_LOGI("end to clear process cache, size=%{public}zu, capacity=%{public}u", processInfos_.size(), capacity_);
}

bool ProcessStatus::UpdateProcessName(int32_t pid, const std::string& procName)
{
    if (procName.empty()) {
        return false;
    }

    if (processInfos_.find(pid) != processInfos_.end()) {
        processInfos_[pid].name = procName;
        return true;
    }
    processInfos_[pid] = {
        .name = procName,
        .state = BACKGROUND,
        .lastForegroundTime = INVALID_LAST_FOREGROUND_TIME,
    };
    return true;
}

ProcessState ProcessStatus::GetProcessState(int32_t pid)
{
    std::unique_lock<ffrt::mutex> lock(mutex_);
    return (processInfos_.find(pid) != processInfos_.end())
        ? processInfos_[pid].state
        : BACKGROUND;
}

uint64_t ProcessStatus::GetProcessLastForegroundTime(int32_t pid)
{
    std::unique_lock<ffrt::mutex> lock(mutex_);
    return (processInfos_.find(pid) != processInfos_.end())
        ? processInfos_[pid].lastForegroundTime
        : INVALID_LAST_FOREGROUND_TIME;
}

void ProcessStatus::NotifyProcessState(int32_t pid, ProcessState procState)
{
    std::unique_lock<ffrt::mutex> lock(mutex_);
    UpdateProcessState(pid, procState);
}

void ProcessStatus::UpdateProcessState(int32_t pid, ProcessState procState)
{
    HIVIEW_LOGD("pid=%{public}d state=%{public}d", pid, procState);
    switch (procState) {
        case FOREGROUND:
            UpdateProcessForegroundState(pid);
            break;
        case BACKGROUND:
            UpdateProcessBackgroundState(pid);
            break;
        case CREATED:
            ClearProcessInfo(pid);
            break;
        case DIED:
            ClearProcessInfo(pid);
            break;
        default:
            HIVIEW_LOGW("invalid process=%{public}d state=%{public}d", pid, procState);
    }
}

void ProcessStatus::UpdateProcessForegroundState(int32_t pid)
{
    HIVIEW_LOGI("pid=%{public}d state=FOREGROUND", pid);
    uint64_t nowTime = TimeUtil::GetMilliseconds();
    if (processInfos_.find(pid) != processInfos_.end()) {
        processInfos_[pid].state = FOREGROUND;
        processInfos_[pid].lastForegroundTime = nowTime;
        return;
    }
    processInfos_[pid] = {
        .name = CommonUtils::GetProcFullNameByPid(pid),
        .state = FOREGROUND,
        .lastForegroundTime = nowTime,
    };
}

void ProcessStatus::UpdateProcessBackgroundState(int32_t pid)
{
    HIVIEW_LOGI("pid=%{public}d state=BACKGROUND", pid);
    if (processInfos_.find(pid) != processInfos_.end()) {
        // last foreground time needs to be updated when the foreground status is switched to the background
        if (processInfos_[pid].state == FOREGROUND) {
            processInfos_[pid].lastForegroundTime = TimeUtil::GetMilliseconds();
        }
        processInfos_[pid].state = BACKGROUND;
        return;
    }
    processInfos_[pid] = {
        .name = CommonUtils::GetProcFullNameByPid(pid),
        .state = BACKGROUND,
        .lastForegroundTime = INVALID_LAST_FOREGROUND_TIME,
    };
}

void ProcessStatus::ClearProcessInfo(int32_t pid)
{
    if (processInfos_.find(pid) != processInfos_.end()) {
        processInfos_.erase(pid);
        HIVIEW_LOGD("end to clear process cache, pid=%{public}d", pid);
    }
}
} // UCollectUtil
}  // namespace HiviewDFX
}  // namespace OHOS
