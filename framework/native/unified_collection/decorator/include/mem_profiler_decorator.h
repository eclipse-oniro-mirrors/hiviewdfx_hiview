/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_MEM_PROFILER_DECORATOR_H
#define HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_MEM_PROFILER_DECORATOR_H

#include "mem_profiler_collector.h"
#include "decorator.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
class MemProfilerDecorator : public MemProfilerCollector, public UCDecorator {
public:
    MemProfilerDecorator(std::shared_ptr<MemProfilerCollector> collector) : memProfilerCollector_(collector) {};
    virtual ~MemProfilerDecorator() = default;
    int Start(const MemoryProfilerConfig& memoryProfilerConfig) override;
    int Stop(int pid) override;
    int Stop(const std::string& processName) override;
    int Start(int fd, const MemoryProfilerConfig& memoryProfilerConfig) override;
    int StartPrintNmd(int fd, int pid, int type) override;
    int Start(int fd, bool startup, const MemoryProfilerConfig& memoryProfilerConfig) override;
    int Prepare() override;
    int Start(int fd, pid_t pid, uint32_t duration, const SimplifiedMemConfig& config) override;
    int StartPrintSimplifiedNmd(pid_t pid, std::vector<SimplifiedMemStats>& memStats) override;
    
    void GenerateStatInfo(uint64_t startTime, const std::string& funcName, int result);
    static void SaveStatCommonInfo();
    static void ResetStatInfo();

private:
    std::shared_ptr<MemProfilerCollector> memProfilerCollector_;
    static StatInfoWrapper statInfoWrapper_;
};
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_MEM_PROFILER_DECORATOR_H
