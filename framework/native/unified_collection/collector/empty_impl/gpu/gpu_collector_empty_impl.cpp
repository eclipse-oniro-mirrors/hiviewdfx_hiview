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

#include "gpu_collector_empty_impl.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
std::shared_ptr<GpuCollector> GpuCollector::Create()
{
    return std::make_shared<GpuCollectorEmptyImpl>();
}

CollectResult<GpuFreq> GpuCollectorEmptyImpl::CollectGpuFrequency()
{
    return CollectResult<GpuFreq>(UCollect::UcError::FEATURE_CLOSED);
}

CollectResult<SysGpuLoad> GpuCollectorEmptyImpl::CollectSysGpuLoad()
{
    return CollectResult<SysGpuLoad>(UCollect::UcError::FEATURE_CLOSED);
}
} // UCollectUtil
} // HiViewDFX
} // OHOS
