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
 */

#include "dump_trace_controller.h"

#include "common_utils.h"
#include "cpu_collector.h"
#include "hiview_logger.h"
#include "time_util.h"
#include "trace_collector.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("DumpTraceController");

void DumpTraceController::CheckAndDumpTrace()
{
    for (auto& cpuThresholdItem : cpuThresholdItems_) {
        if (!IsTimeOver(cpuThresholdItem)) {
            continue;
        }
        if (!IsCpuLoadBackToNormal(cpuThresholdItem)) {
            continue;
        }
        DumpTrace(cpuThresholdItem);
    }
}

bool DumpTraceController::IsTimeOver(const CpuThresholdItem& item)
{
    uint64_t currentTime = static_cast<uint64_t>(TimeUtil::GetSeconds());
    return (currentTime - item.lastDumpTraceTime > item.dumpTraceInterval ||
        currentTime - item.lastDumpTraceTime < 0);
}

// only when cpu load restore form high to normal, meet the condition for capturing trace
bool DumpTraceController::IsCpuLoadBackToNormal(CpuThresholdItem& item)
{
    int32_t pid = CommonUtils::GetPidByName(item.processName);
    if (pid <= 0) {
        HIVIEW_LOGW("get pid failed, process:%{public}s", item.processName.c_str());
        return false;
    }
    auto cpuCollector = UCollectUtil::CpuCollector::Create();
    auto processCpuStatInfo = cpuCollector->CollectProcessCpuStatInfo(pid);
    double cpuLoad = processCpuStatInfo.data.cpuLoad;
    if (cpuLoad >= item.cpuLoadThreshold) {
        item.hasOverThreshold = true;
        return false;
    }
    if (item.hasOverThreshold && cpuLoad < item.cpuLoadThreshold) {
        return true;
    }
    return false;
}

void DumpTraceController::DumpTrace(CpuThresholdItem& item)
{
    auto traceCollector = UCollectUtil::TraceCollector::Create();
    traceCollector->DumpTrace(item.caller);
    item.hasOverThreshold = false;
    item.lastDumpTraceTime = static_cast<uint64_t>(TimeUtil::GetSeconds());
}
}  // namespace HiviewDFX
}  // namespace OHOS
