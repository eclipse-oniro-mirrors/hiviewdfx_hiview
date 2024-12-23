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

#ifndef INTERFACES_INNER_API_UNIFIED_COLLECTION_UTILITY_PROCESS_COLLECTOR_H
#define INTERFACES_INNER_API_UNIFIED_COLLECTION_UTILITY_PROCESS_COLLECTOR_H

#include <memory>
#include <string>
#include <unordered_set>

#include "collect_result.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
class ProcessCollector {
public:
    ProcessCollector() = default;
    virtual ~ProcessCollector() = default;

public:
    virtual CollectResult<std::unordered_set<int32_t>> GetMemCgProcesses() = 0;
    virtual CollectResult<bool> IsMemCgProcess(int32_t pid) = 0;
    virtual CollectResult<std::string> ExportMemCgProcesses() = 0;
    static std::shared_ptr<ProcessCollector> Create();
};
} // UCollectUtil
} // HiviewDFX
} // OHOS
#endif // INTERFACES_INNER_API_UNIFIED_COLLECTION_UTILITY_PROCESS_COLLECTOR_H