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

#ifndef HIVIEW_THREAD_STATE_INFO_COLLECTOR_H
#define HIVIEW_THREAD_STATE_INFO_COLLECTOR_H
#include <mutex>

#include "cpu.h"
#include "collect_result.h"
#include "collect_device_client.h"
#include "cpu_calculator.h"
#include "cpu_collector.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {

struct ThreadCpuTimeInfo {
    uint64_t uUsageTime = 0;
    uint64_t sUsageTime = 0;
    uint64_t loadTime = 0;
    uint64_t collectionTime = 0;
    uint64_t collectionBootTime = 0;
};

class ThreadStateInfoCollector : public ThreadCpuCollector {
public:
    ThreadStateInfoCollector(std::shared_ptr<CollectDeviceClient> deviceClient,
         std::shared_ptr<CpuCalculator> cpuCalculator, int collectPid);
    explicit ThreadStateInfoCollector(int32_t collectPid);
    virtual ~ThreadStateInfoCollector() = default;

public:
    CollectResult<std::vector<ThreadCpuStatInfo>> CollectThreadStatInfos(bool isNeedUpdate) override;
    int GetCollectPid() override;

private:
    void InitLastThreadCpuTimeInfos();
    CalculationTimeInfo InitCalculationTimeInfo();
    std::shared_ptr<ThreadCpuData> FetchThreadCpuData();
    void UpdateLastThreadTimeInfo(const ucollection_thread_cpu_item* threadCpuItem,
        const CalculationTimeInfo& calcTimeInfo);
    void CalculateThreadCpuStatInfos(std::vector<ThreadCpuStatInfo>& ThreadCpuStatInfos,
        std::shared_ptr<ThreadCpuData> ThreadCpuData, bool isNeedUpdate);
    std::optional<ThreadCpuStatInfo> CalculateThreadCpuStatInfo(
        const ucollection_thread_cpu_item* procCpuItem, const CalculationTimeInfo& calcTimeInfo);
    void UpdateCollectionTime(const CalculationTimeInfo& calcTimeInfo);
    void TryToDeleteDeadThreadInfo();
    void InitDeviceClient();

private:
    std::mutex collectMutex_;
    uint64_t lastCollectionTime_ = 0;
    uint64_t lastCollectionBootTime_ = 0;
    std::shared_ptr<CollectDeviceClient> deviceClient_;
    std::shared_ptr<CpuCalculator> cpuCalculator_;
    std::unordered_map<int32_t, ThreadCpuTimeInfo> lastThreadCpuTimeInfos_;
    int collectPid_ = 0;
};
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif //HIVIEW_THREAD_CPU_STATE_INFO_COLLECTOR_H
