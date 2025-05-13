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

#ifndef PERF_UTILS_H
#define PERF_UTILS_H

#include <string>
#include "perf_constants.h"
#include "perf_model.h"

namespace OHOS {
namespace HiviewDFX {

int64_t GetCurrentRealTimeNs();

int64_t GetCurrentSystimeMs();

void ConvertRealtimeToSystime(int64_t realTime, int64_t& sysTime);

std::string GetSourceTypeName(PerfSourceType sourceType);

}
}

#endif // PERF_UTILS_H
