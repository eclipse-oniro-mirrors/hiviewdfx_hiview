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
#ifndef INTERFACES_INNER_API_UNIFIED_COLLECTION_UTILITY_MEM_PROFILER_COLLECTOR_H
#define INTERFACES_INNER_API_UNIFIED_COLLECTION_UTILITY_MEM_PROFILER_COLLECTOR_H
#include <memory>
#include "collect_result.h"
#include "native_memory_profiler_sa_client_manager.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
using ProfilerType = Developtools::NativeDaemon::NativeMemoryProfilerSaClientManager::NativeMemProfilerType;
class MemProfilerCollector {
public:
    MemProfilerCollector() = default;
    virtual ~MemProfilerCollector() = default;

public:
    virtual int Start(ProfilerType type,
                      int pid, int duration, int sampleInterval) = 0;
    virtual int StartPrintNmd(int fd, int pid, int type) = 0;
    virtual int Stop(int pid) = 0;
    virtual int Stop(const std::string& processName) = 0;
    virtual int Start(int fd, ProfilerType type,
                      int pid, int duration, int sampleInterval) = 0;
    virtual int Start(int fd, ProfilerType type,
                      std::string processName, int duration, int sampleInterval, bool startup = false) = 0;
    virtual int Prepare() = 0;
    static std::shared_ptr<MemProfilerCollector> Create();
}; // MemProfilerCollector
} // UCollectUtil
} // HiviewDFX
} // OHOS
#endif // INTERFACES_INNER_API_UNIFIED_COLLECTION_UTILITY_MEM_PROFILER_COLLECTOR_H