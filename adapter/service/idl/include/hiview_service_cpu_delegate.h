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

#ifndef OHOS_HIVIEWDFX_ADAPTER_SERVICE_IDL_INCLUDE_CPU_DELEGATE_H
#define OHOS_HIVIEWDFX_ADAPTER_SERVICE_IDL_INCLUDE_CPU_DELEGATE_H

#include "collect_result.h"

namespace OHOS {
namespace HiviewDFX {
class HiViewServiceCpuDelegate {
public:
    static CollectResult<double> GetSysCpuUsage();
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // OHOS_HIVIEWDFX_ADAPTER_SERVICE_IDL_INCLUDE_CPU_DELEGATE_H
