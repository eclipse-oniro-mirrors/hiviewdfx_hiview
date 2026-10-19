/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <cstddef>
#include <cstdint>
#include <string>

#include "app_event_elapsed_time.h"
#include "appeventelapsedtime_fuzzer.h"
#include "securec.h"

namespace OHOS {
namespace HiviewDFX {
constexpr int FUZZTEST_MAX_STR_LEN = 64;

void FuzzElapsedTime(const uint8_t* data, size_t size)
{
    uint64_t limitCostMilliseconds = 0;
    size_t fixedLen = sizeof(limitCostMilliseconds) + FUZZTEST_MAX_STR_LEN;
    if (fixedLen > size) {
        return;
    }

    errno_t err = memcpy_s(&limitCostMilliseconds, sizeof(limitCostMilliseconds), data,
                           sizeof(limitCostMilliseconds));
    if (err != EOK) {
        return;
    }
    data += sizeof(limitCostMilliseconds);

    std::string printContent(reinterpret_cast<const char*>(data), FUZZTEST_MAX_STR_LEN);
    data += FUZZTEST_MAX_STR_LEN;

    // exercise default constructor
    ElapsedTime defaultTime;
    // exercise parameterized constructor and MarkElapsedTime
    ElapsedTime elapsedTime(limitCostMilliseconds, printContent);
    elapsedTime.MarkElapsedTime(printContent);
}
} // namespace
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::HiviewDFX::FuzzElapsedTime(data, size);
    return 0;
}

