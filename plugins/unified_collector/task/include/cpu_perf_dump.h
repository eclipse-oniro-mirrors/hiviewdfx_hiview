/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
#ifdef HAS_HIPERF
#ifndef HIVIEW_PLUGINS_UNIFIED_COLLECTOR_TASK_INCLUDE_CPU_PERF_DUMP_H
#define HIVIEW_PLUGINS_UNIFIED_COLLECTOR_TASK_INCLUDE_CPU_PERF_DUMP_H

#include "perf_collector.h"
#include "resource/cpu.h"

namespace OHOS {
namespace HiviewDFX {
class CpuPerfDump {
public:
    CpuPerfDump();
    ~CpuPerfDump() = default;
    void CheckAndDumpPerfData(std::vector<ProcessCpuStatInfo>& cpuCollectionInfos);
private:
    void DumpTopNCpuProcessPerfData();
    bool CheckRecordInterval();
    void TryToAgePerfFiles();
    bool NeedCleanPerfFiles(size_t size);
    static bool CompareCpuLoad(const ProcessCpuStatInfo &info1, const ProcessCpuStatInfo &info2);
    static bool CompareCpuUsage(const ProcessCpuStatInfo &info1, const ProcessCpuStatInfo &info2);
private:
    std::shared_ptr<UCollectUtil::PerfCollector> perfCollector_;
    std::vector<pid_t> topNProcs_;
    int64_t systemUpTime_ = 0;
    int64_t lastRecordTime_ = 0;
    bool isBetaVersion_;
}; // CpuPerfDump
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_UNIFIED_COLLECTOR_TASK_INCLUDE_CPU_PERF_DUMP_H
#endif // HAS_HIPERF
