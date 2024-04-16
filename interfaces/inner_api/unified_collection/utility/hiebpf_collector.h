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
#ifndef INTERFACES_INNER_API_UNIFIED_COLLECTION_UTILITY_HIEBPF_COLLECTOR_H
#define INTERFACES_INNER_API_UNIFIED_COLLECTION_UTILITY_HIEBPF_COLLECTOR_H
#include <string>

#include "collect_result.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
class HiebpfCollector {
public:
    HiebpfCollector() = default;
    virtual ~HiebpfCollector() = default;
    virtual CollectResult<bool> StartHiebpf(int duration,
        const std::string processName, const std::string outFile) = 0;
    virtual CollectResult<bool> StopHiebpf() = 0;
    static std::shared_ptr<HiebpfCollector> Create();
};
} // UCollectUtil
} // HivewDFX
} // OHOS
#endif // INTERFACES_INNER_API_UNIFIED_COLLECTION_UTILITY_HIEBPF_COLLECTOR_H
