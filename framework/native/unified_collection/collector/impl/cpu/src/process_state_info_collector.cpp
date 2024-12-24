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

#include "process_state_info_collector.h"

#include <algorithm>
#include <cinttypes>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "common_utils.h"
#include "cpu_decorator.h"
#include "hiview_logger.h"
#include "process_status.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
DEFINE_LOG_TAG("CpuCollector");
ProcessStatInfoCollector::ProcessStatInfoCollector(std::shared_ptr<CollectDeviceClient> deviceClient,
    std::shared_ptr<CpuCalculator> cpuCalculator): deviceClient_(deviceClient), cpuCalculator_(cpuCalculator)
{
    InitLastProcCpuTimeInfos();
}

void ProcessStatInfoCollector::InitLastProcCpuTimeInfos()
{
    auto processCpuData = FetchProcessCpuData();
    if (processCpuData == nullptr) {
        return;
    }

    // init cpu time information for each process in the system
    CalculationTimeInfo calcTimeInfo = InitCalculationTimeInfo();
    auto procCpuItem = processCpuData->GetNextProcess();
    while (procCpuItem != nullptr) {
        UpdateLastProcCpuTimeInfo(procCpuItem, calcTimeInfo);
        procCpuItem = processCpuData->GetNextProcess();
    }
    UpdateCollectionTime(calcTimeInfo);
}

std::shared_ptr<ProcessCpuData> ProcessStatInfoCollector::FetchProcessCpuData(int32_t pid)
{
    if (deviceClient_ == nullptr) {
        HIVIEW_LOGW("device client is null");
        return nullptr;
    }
    return (pid == INVALID_PID)
        ? deviceClient_->FetchProcessCpuData()
        : deviceClient_->FetchProcessCpuData(pid);
}

void ProcessStatInfoCollector::UpdateCollectionTime(const CalculationTimeInfo& calcTimeInfo)
{
    lastCollectionTime_ = calcTimeInfo.endTime;
    lastCollectionBootTime_ = calcTimeInfo.endBootTime;
}

void ProcessStatInfoCollector::UpdateLastProcCpuTimeInfo(const ucollection_process_cpu_item* procCpuItem,
    const CalculationTimeInfo& calcTimeInfo)
{
    lastProcCpuTimeInfos_[procCpuItem->pid] = {
        .minFlt = procCpuItem->min_flt,
        .majFlt = procCpuItem->maj_flt,
        .uUsageTime = procCpuItem->cpu_usage_utime,
        .sUsageTime = procCpuItem->cpu_usage_stime,
        .loadTime = procCpuItem->cpu_load_time,
        .collectionTime = calcTimeInfo.endTime,
        .collectionBootTime = calcTimeInfo.endBootTime,
    };
}

CollectResult<std::vector<ProcessCpuStatInfo>> ProcessStatInfoCollector::CollectProcessCpuStatInfos(
    bool isNeedUpdate)
{
    CollectResult<std::vector<ProcessCpuStatInfo>> cpuCollectResult;
    std::unique_lock<std::mutex> lock(collectMutex_);
    auto processCpuData = FetchProcessCpuData();
    if (processCpuData == nullptr) {
        return cpuCollectResult;
    }

    CalculateProcessCpuStatInfos(cpuCollectResult.data, processCpuData, isNeedUpdate);
    HIVIEW_LOGI("collect process cpu statistics information size=%{public}zu, isNeedUpdate=%{public}d",
        cpuCollectResult.data.size(), isNeedUpdate);
    if (!cpuCollectResult.data.empty()) {
        cpuCollectResult.retCode = UCollect::UcError::SUCCESS;
        TryToDeleteDeadProcessInfo();
    }
    return cpuCollectResult;
}

void ProcessStatInfoCollector::CalculateProcessCpuStatInfos(
    std::vector<ProcessCpuStatInfo>& processCpuStatInfos,
    std::shared_ptr<ProcessCpuData> processCpuData,
    bool isNeedUpdate)
{
    CalculationTimeInfo calcTimeInfo = InitCalculationTimeInfo();
    HIVIEW_LOGI("startTime=%{public}" PRIu64 ", endTime=%{public}" PRIu64 ", startBootTime=%{public}" PRIu64
        ", endBootTime=%{public}" PRIu64 ", period=%{public}" PRIu64, calcTimeInfo.startTime,
        calcTimeInfo.endTime, calcTimeInfo.startBootTime, calcTimeInfo.endBootTime, calcTimeInfo.period);
    auto procCpuItem = processCpuData->GetNextProcess();
    while (procCpuItem != nullptr) {
        auto processCpuStatInfo = CalculateProcessCpuStatInfo(procCpuItem, calcTimeInfo);
        if (processCpuStatInfo.has_value()) {
            processCpuStatInfos.emplace_back(processCpuStatInfo.value());
        }
        if (isNeedUpdate) {
            UpdateLastProcCpuTimeInfo(procCpuItem, calcTimeInfo);
        }
        procCpuItem = processCpuData->GetNextProcess();
    }

    if (isNeedUpdate) {
        UpdateCollectionTime(calcTimeInfo);
    }
}

CalculationTimeInfo ProcessStatInfoCollector::InitCalculationTimeInfo()
{
    CalculationTimeInfo calcTimeInfo = {
        .startTime = lastCollectionTime_,
        .endTime = TimeUtil::GetMilliseconds(),
        .startBootTime = lastCollectionBootTime_,
        .endBootTime = TimeUtil::GetBootTimeMs(),
    };
    calcTimeInfo.period = calcTimeInfo.endBootTime > calcTimeInfo.startBootTime
        ? (calcTimeInfo.endBootTime - calcTimeInfo.startBootTime) : 0;
    return calcTimeInfo;
}

std::optional<ProcessCpuStatInfo> ProcessStatInfoCollector::CalculateProcessCpuStatInfo(
    const ucollection_process_cpu_item* procCpuItem, const CalculationTimeInfo& calcTimeInfo)
{
    if (cpuCalculator_ == nullptr) {
        return std::nullopt;
    }
    if (lastProcCpuTimeInfos_.find(procCpuItem->pid) == lastProcCpuTimeInfos_.end()) {
        HIVIEW_LOGD("lastProcCpuTimeInfos do not have pid:%{public}d", procCpuItem->pid);
        return std::nullopt;
    }
    ProcessCpuStatInfo processCpuStatInfo;
    processCpuStatInfo.startTime = calcTimeInfo.startTime;
    processCpuStatInfo.endTime = calcTimeInfo.endTime;
    processCpuStatInfo.pid = procCpuItem->pid;
    processCpuStatInfo.minFlt = procCpuItem->min_flt;
    processCpuStatInfo.majFlt = procCpuItem->maj_flt;
    processCpuStatInfo.procName = ProcessStatus::GetInstance().GetProcessName(procCpuItem->pid);
    processCpuStatInfo.cpuLoad = cpuCalculator_->CalculateCpuLoad(procCpuItem->cpu_load_time,
        lastProcCpuTimeInfos_[procCpuItem->pid].loadTime, calcTimeInfo.period);
    processCpuStatInfo.uCpuUsage = cpuCalculator_->CalculateCpuUsage(procCpuItem->cpu_usage_utime,
        lastProcCpuTimeInfos_[procCpuItem->pid].uUsageTime, calcTimeInfo.period);
    processCpuStatInfo.sCpuUsage = cpuCalculator_->CalculateCpuUsage(procCpuItem->cpu_usage_stime,
        lastProcCpuTimeInfos_[procCpuItem->pid].sUsageTime, calcTimeInfo.period);
    processCpuStatInfo.cpuUsage = processCpuStatInfo.uCpuUsage + processCpuStatInfo.sCpuUsage;
    processCpuStatInfo.threadCount = procCpuItem->thread_total;
    if (processCpuStatInfo.cpuLoad >= 1) { // 1: max cpu load
        HIVIEW_LOGI("invalid cpu load=%{public}f, name=%{public}s, last_load=%{public}" PRIu64
            ", curr_load=%{public}" PRIu64, processCpuStatInfo.cpuLoad, processCpuStatInfo.procName.c_str(),
            lastProcCpuTimeInfos_[procCpuItem->pid].loadTime, static_cast<uint64_t>(procCpuItem->cpu_load_time));
    }
    return std::make_optional<ProcessCpuStatInfo>(processCpuStatInfo);
}

void ProcessStatInfoCollector::TryToDeleteDeadProcessInfo()
{
    for (auto it = lastProcCpuTimeInfos_.begin(); it != lastProcCpuTimeInfos_.end();) {
        // if the latest collection operation does not update the process collection time, delete it
        if (it->second.collectionTime != lastCollectionTime_) {
            ProcessStatus::GetInstance().NotifyProcessState(it->first, DIED);
            it = lastProcCpuTimeInfos_.erase(it);
        } else {
            it++;
        }
    }
    HIVIEW_LOGD("end to delete dead process, size=%{public}zu", lastProcCpuTimeInfos_.size());
}

CollectResult<ProcessCpuStatInfo> ProcessStatInfoCollector::CollectProcessCpuStatInfo(int32_t pid, bool isNeedUpdate)
{
    CollectResult<ProcessCpuStatInfo> cpuCollectResult;

    std::unique_lock<std::mutex> lock(collectMutex_);
    if (!CommonUtils::IsPidExist(pid)) {
        HIVIEW_LOGD("pid:%{public}d not exist", pid);
        TryToDeleteDeadProcessInfoByPid(pid);
        return cpuCollectResult;
    }
    auto processCpuData = FetchProcessCpuData(pid);
    if (processCpuData == nullptr) {
        return cpuCollectResult;
    }
    auto procCpuItem = processCpuData->GetNextProcess();
    if (procCpuItem == nullptr) {
        return cpuCollectResult;
    }

    CalculationTimeInfo calcTimeInfo = InitCalculationTimeInfo(pid);
    auto processCpuStatInfo = CalculateProcessCpuStatInfo(procCpuItem, calcTimeInfo);
    if (processCpuStatInfo.has_value()) {
        cpuCollectResult.retCode = UCollect::UcError::SUCCESS;
        cpuCollectResult.data = processCpuStatInfo.value();
    }
    if (isNeedUpdate) {
        UpdateLastProcCpuTimeInfo(procCpuItem, calcTimeInfo);
        TryToDeleteDeadProcessInfoByTime(calcTimeInfo.endBootTime);
    }
    return cpuCollectResult;
}

CalculationTimeInfo ProcessStatInfoCollector::InitCalculationTimeInfo(int32_t pid)
{
    CalculationTimeInfo calcTimeInfo = {
        .startTime = lastProcCpuTimeInfos_.find(pid) != lastProcCpuTimeInfos_.end()
            ? lastProcCpuTimeInfos_[pid].collectionTime : 0,
        .endTime = TimeUtil::GetMilliseconds(),
        .startBootTime = lastProcCpuTimeInfos_.find(pid) != lastProcCpuTimeInfos_.end()
            ? lastProcCpuTimeInfos_[pid].collectionBootTime : 0,
        .endBootTime = TimeUtil::GetBootTimeMs(),
    };
    calcTimeInfo.period = calcTimeInfo.endBootTime > calcTimeInfo.startBootTime
        ? (calcTimeInfo.endBootTime - calcTimeInfo.startBootTime) : 0;
    return calcTimeInfo;
}

void ProcessStatInfoCollector::TryToDeleteDeadProcessInfoByPid(int32_t pid)
{
    if (lastProcCpuTimeInfos_.find(pid) != lastProcCpuTimeInfos_.end()) {
        lastProcCpuTimeInfos_.erase(pid);
        HIVIEW_LOGD("end to delete dead process=%{public}d", pid);
    }
}

void ProcessStatInfoCollector::TryToDeleteDeadProcessInfoByTime(uint64_t collectionBootTime)
{
    static uint64_t lastClearTime = TimeUtil::GetBootTimeMs();
    uint64_t interval = collectionBootTime > lastClearTime ? (collectionBootTime - lastClearTime) : 0;
    constexpr uint32_t clearInterval = 600 * 1000; // 10min
    if (interval < clearInterval) {
        return;
    }
    lastClearTime = collectionBootTime;
    HIVIEW_LOGD("start to delete dead processes, size=%{public}zu", lastProcCpuTimeInfos_.size());
    for (auto it = lastProcCpuTimeInfos_.begin(); it != lastProcCpuTimeInfos_.end();) {
        if (!CommonUtils::IsPidExist(it->first)) {
            it = lastProcCpuTimeInfos_.erase(it);
        } else {
            it++;
        }
    }
    HIVIEW_LOGD("end to delete dead processes, size=%{public}zu", lastProcCpuTimeInfos_.size());
}
} // UCollectUtil
} // HiViewDFX
} // OHOS
