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

#ifndef XPERF_SERVICE_XPERFSERVICE_ACTION_TYPE_H
#define XPERF_SERVICE_XPERFSERVICE_ACTION_TYPE_H

#include <stdint.h>

namespace OHOS {
namespace HiviewDFX {

enum ActionType : uint32_t {
    ACTION_TYPE_PERF,
    ACTION_TYPE_POWER,
    ACTION_TYPE_THERMAL,
    ACTION_TYPE_PERFLVL,
    ACTION_TYPE_BATTERY,
    ACTION_TYPE_MAX
};

} // namespace HiviewDFX
} // namespace OHOS

#endif
