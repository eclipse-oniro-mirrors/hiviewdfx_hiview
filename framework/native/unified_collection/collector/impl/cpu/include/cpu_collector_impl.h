/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_CPU_COLLECTOR_IMPL_H
#define HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_CPU_COLLECTOR_IMPL_H

#include <mutex>

#include "collect_device_client.h"
#include "cpu_calculator.h"
#include "cpu_collector.h"
#include "cpu_util.h"
#include "process_state_info_collector.h"
#include "sys_cpu_usage_collector.h"
#include "thread_state_info_collector.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {

class CpuCollectorImpl : public CpuCollector {
public:
    CpuCollectorImpl();
    virtual ~CpuCollectorImpl() = default;

public:
    virtual CollectResult<SysCpuLoad> CollectSysCpuLoad() override;
    virtual CollectResult<SysCpuUsage> CollectSysCpuUsage(bool isNeedUpdate = false) override;
    virtual CollectResult<double> GetSysCpuUsage() override;
    virtual CollectResult<ProcessCpuStatInfo> CollectProcessCpuStatInfo(int32_t pid,
        bool isNeedUpdate = false) override;
    virtual CollectResult<std::vector<CpuFreq>> CollectCpuFrequency() override;
    virtual CollectResult<std::vector<ProcessCpuStatInfo>> CollectProcessCpuStatInfos(
        bool isNeedUpdate = false) override;
    virtual std::shared_ptr<ThreadCpuCollector> CreateThreadCollector(int pid) override;
private:
    bool InitDeviceClient();

private:
    std::shared_ptr<CollectDeviceClient> deviceClient_ = nullptr;
    std::shared_ptr<ProcessStatInfoCollector> statInfoCollector_ = nullptr;
    std::shared_ptr<SysCpuUsageCollector> usageCollector_ = nullptr;
    std::shared_ptr<CpuCalculator> cpuCalculator_ = nullptr;
};
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_CPU_COLLECTOR_IMPL_H