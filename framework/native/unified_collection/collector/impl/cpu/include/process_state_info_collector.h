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

#ifndef HIVIEW_FRAMEWORK_NATIVE_PROCESS_STATE_INFO_COLLECTION_H
#define HIVIEW_FRAMEWORK_NATIVE_PROCESS_STATE_INFO_COLLECTION_H

#include <mutex>

#include "collect_device_client.h"
#include "cpu_calculator.h"
#include "cpu_collector.h"
#include "cpu_util.h"
#include "process_cpu_data.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
constexpr int32_t INVALID_PID = 0;
struct ProcessCpuTimeInfo {
    uint32_t minFlt = 0;
    uint32_t majFlt = 0;
    uint64_t uUsageTime = 0;
    uint64_t sUsageTime = 0;
    uint64_t loadTime = 0;
    uint64_t collectionTime = 0;
    uint64_t collectionBootTime = 0;
};

class ProcessStatInfoCollector {
public:
    ProcessStatInfoCollector(std::shared_ptr<CollectDeviceClient> deviceClient,
        std::shared_ptr<CpuCalculator> cpuCalculator);
    ~ProcessStatInfoCollector() = default;

public:
    CollectResult<ProcessCpuStatInfo> CollectProcessCpuStatInfo(int32_t pid,
        bool isNeedUpdate = false);
    CollectResult<std::vector<ProcessCpuStatInfo>> CollectProcessCpuStatInfos(
        bool isNeedUpdate = false);

private:
    void InitLastProcCpuTimeInfos();
    CalculationTimeInfo InitCalculationTimeInfo();
    CalculationTimeInfo InitCalculationTimeInfo(int32_t pid);
    std::shared_ptr<ProcessCpuData> FetchProcessCpuData(int32_t pid = INVALID_PID);
    void UpdateCollectionTime(const CalculationTimeInfo& calcTimeInfo);
    void UpdateLastProcCpuTimeInfo(const ucollection_process_cpu_item* procCpuItem,
        const CalculationTimeInfo& calcTimeInfo);
    void CalculateProcessCpuStatInfos(
        std::vector<ProcessCpuStatInfo>& processCpuStatInfos,
        std::shared_ptr<ProcessCpuData> processCpuData,
        bool isNeedUpdate);
    std::optional<ProcessCpuStatInfo> CalculateProcessCpuStatInfo(
        const ucollection_process_cpu_item* procCpuItem, const CalculationTimeInfo& calcTimeInfo);
    void TryToDeleteDeadProcessInfo();
    void TryToDeleteDeadProcessInfoByPid(int32_t pid);
    void TryToDeleteDeadProcessInfoByTime(uint64_t collectionBootTime);

private:
    std::mutex collectMutex_;
    uint64_t lastCollectionTime_ = 0;
    uint64_t lastCollectionBootTime_ = 0;
    std::shared_ptr<CollectDeviceClient> deviceClient_;
    /* map<pid, ProcessCpuTimeInfo> */
    std::unordered_map<int32_t, ProcessCpuTimeInfo> lastProcCpuTimeInfos_;
    std::shared_ptr<CpuCalculator> cpuCalculator_;
};
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_FRAMEWORK_NATIVE_PROCESS_STATE_INFO_COLLECTION_H