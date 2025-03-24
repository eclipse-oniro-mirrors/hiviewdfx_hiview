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

#ifndef FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_DUMP_TRACE_CONTROLLER_H
#define FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_DUMP_TRACE_CONTROLLER_H

#include <cstdint>
#include <string>
#include <vector>

#include "trace_caller.h"

namespace OHOS {
namespace HiviewDFX {
struct CpuThresholdItem {
    bool hasOverThreshold = false;
    UCollect::TraceCaller caller;
    uint32_t dumpTraceInterval = 0;
    uint64_t lastDumpTraceTime = 0;
    double cpuLoadThreshold = 0.0;
    std::string processName;
};

class DumpTraceController {
public:
    DumpTraceController(std::vector<CpuThresholdItem> cpuThresholdItems) : cpuThresholdItems_(cpuThresholdItems) {}
    ~DumpTraceController() {}
    void CheckAndDumpTrace();

private:
    bool IsTimeOver(const CpuThresholdItem& item);
    bool IsCpuLoadBackToNormal(CpuThresholdItem& item);
    void DumpTrace(CpuThresholdItem& item);

private:
    std::vector<CpuThresholdItem> cpuThresholdItems_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_DUMP_TRACE_CONTROLLER_H
