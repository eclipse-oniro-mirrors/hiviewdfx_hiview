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

#ifndef HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_HIEBPF_COLLECTOR_IMPL_H
#define HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_HIEBPF_COLLECTOR_IMPL_H

#include "hiebpf_collector.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
class HiebpfCollectorImpl : public HiebpfCollector {
public:
    HiebpfCollectorImpl() = default;
    virtual ~HiebpfCollectorImpl() = default;
    virtual CollectResult<bool> StartHiebpf(int duration,
        const std::string processName, const std::string outFile) override;
    virtual CollectResult<bool> StopHiebpf() override;
};
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_HIEBPF_COLLECTOR_IMPL_H