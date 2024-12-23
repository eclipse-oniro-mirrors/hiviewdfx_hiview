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

#ifndef HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_MEMPROFILER_COLLECTOR_IMPL_H
#define HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_MEMPROFILER_COLLECTOR_IMPL_H

#include "mem_profiler_collector.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
class MemProfilerCollectorImpl : public MemProfilerCollector {
public:
    MemProfilerCollectorImpl() = default;
    virtual ~MemProfilerCollectorImpl() = default;

public:
    int Start(ProfilerType type, int pid, int duration, int sampleInterval) override;
    int StartPrintNmd(int fd, int pid, int type) override;
    int Stop(int pid) override;
    int Stop(const std::string& processName) override;
    int Start(int fd, ProfilerType type, int pid, int duration, int sampleInterval) override;
    int Start(int fd, ProfilerType type, std::string processName, int duration, int sampleInterval,
              bool startup = false) override;
    int Prepare() override;
    enum ErrorType {
        RET_FAIL = -1,
        RET_SUCC = 0,
    };
};
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_MEMPROFILER_COLLECTOR_IMPL_H