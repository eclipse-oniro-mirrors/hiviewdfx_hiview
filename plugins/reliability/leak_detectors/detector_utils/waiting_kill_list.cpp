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
 * Author: zhangzhezhao
 * Create: 2025-9-25
 */

#include "waiting_kill_list.h"

#include <hiview_logger.h>
#include "fault_common.h"
#include "fault_detector_manager.h"
#include "fault_state_removal_base.h"

namespace {
constexpr int DEFAULT_WAITING_KILL_INTERVAL = 30; // 30s
constexpr int DEFAULT_WAITING_KILL_DELAY_TIME = 300; // 300s
}

using namespace std;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "WaitingKillList");

WaitingKillList::WaitingKillList() : waitingKillInterval_(DEFAULT_WAITING_KILL_INTERVAL),
    waitingKillDelayTime_(DEFAULT_WAITING_KILL_DELAY_TIME)
{}

WaitingKillList::~WaitingKillList() {}

void WaitingKillList::DoParseConfig(const cJSON *node)
{
    if (!(node)) {
        HIVIEW_LOGE("WaitingKillList node is null.\n");
        return;
    }
    LeakJsonParser parser;
    parser.OverWriteIfExist(node, "wait_kill_interval_s", waitingKillInterval_);
    parser.OverWriteIfExist(node, "wait_kill_delay_time_s", waitingKillDelayTime_);
    HIVIEW_LOGI("waitingKillInterval_: %{public}d s, waitingKillDelayTime_: %{public}d s",
        waitingKillInterval_, waitingKillDelayTime_);
}

void WaitingKillList::AddProcessIntoWaitList(pid_t pid, WaitingKillLeakInfo &leakInfo)
{
    std::lock_guard<ffrt::mutex> lock(waitKillMtx_);
    if (waitKillList_.count(pid) == 0) {
        vector<WaitingKillLeakInfo> addList;
        addList.push_back(leakInfo);
        waitKillList_.insert(make_pair(pid, addList));
    } else {
        waitKillList_[pid].push_back(leakInfo);
    }
    HIVIEW_LOGI("pid: %{public}d has added into waitkilllist", pid);
}

void WaitingKillList::KillProcessInWaitList()
{
    time_t now = time(nullptr);
    if (now - lastKillProcessInWaitList_.load() < waitingKillInterval_) {
        return;
    }
    std::lock_guard<ffrt::mutex> lock(waitKillMtx_);
    if (waitKillList_.empty()) {
        HIVIEW_LOGI("waitKillList is empty");
        return;
    }
    lastKillProcessInWaitList_.store(now);
    for (auto it = waitKillList_.begin(); it != waitKillList_.end(); it++) {
        const pid_t pid = it->first;
        auto leakInfoList = it->second;
        if (FaultDetectorManager::CheckIfAppForeGround(DfrUtil::GetInstance().GetProcessName(pid))) {
            HIVIEW_LOGI("pid: %{public}d is still in foreground", pid);
            continue;
        }
        ffrt::submit([=] {
            HIVIEW_LOGI("try kill process, pid: %{public}d", pid);
            TryKillProcess(pid, leakInfoList);
            }, {}, {},
            {ffrt::task_attr().name("ffrt_dfr_waiting_kill_list_TryKillProcess")
                              .delay(waitingKillDelayTime_ * US_PER_SECOND)});
    }
}

void WaitingKillList::TryKillProcess(pid_t pid, vector<WaitingKillLeakInfo> leakInfoList)
{
    for (auto &leakInfo : leakInfoList) {
        if (!IsInWaitingKillList(pid, leakInfo.leakType) || !leakInfo.faultInfo) {
            HIVIEW_LOGI("not in waitingkilllist or faultInfo is null, need not kill");
            break;
        }
        leakInfo.JudgeAndKillProcessCallback(leakInfo.faultInfo, *leakInfo.detectorObj);
    }
}

void WaitingKillList::DeleteProcessInWaitList(pid_t pid)
{
    std::lock_guard<ffrt::mutex> lock(waitKillMtx_);
    waitKillList_.erase(pid);
}

void WaitingKillList::DeleteLeakTypeInWaitList(pid_t pid, const string &leakType)
{
    std::lock_guard<ffrt::mutex> lock(waitKillMtx_);
    if (waitKillList_.count(pid) == 0) {
        return;
    }
    auto &leakList = waitKillList_[pid];

    for (auto it = leakList.begin(); it != leakList.end();) {
        if (it->leakType == leakType) {
            it = leakList.erase(it);
        } else {
            it++;
        }
    }
    if (waitKillList_[pid].size() == 0) {
        waitKillList_.erase(pid);
    }
    HIVIEW_LOGI("pid: %{public}d leakType: %{public}s has deleted from waitkilllist", pid, leakType.c_str());
}

bool WaitingKillList::IsInWaitingKillList(pid_t pid, string leakType)
{
    std::lock_guard<ffrt::mutex> lock(waitKillMtx_);
    auto it = waitKillList_.find(pid);
    if (it == waitKillList_.end()) {
        return false;
    }
    for (const auto &process : it->second) {
        if (process.leakType == leakType) {
            return true;
        }
    }
    return false;
}
} // namespace HiviewDFX
} // namespace OHOS
