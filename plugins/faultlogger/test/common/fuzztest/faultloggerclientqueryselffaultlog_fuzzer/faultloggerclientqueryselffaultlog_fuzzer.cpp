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

#include <string>

#include "faultlogger_client.h"
#include "faultloggerclientqueryselffaultlog_fuzzer.h"
#include "faultlogger_fuzzertest_common.h"

namespace OHOS {
void FuzzInterfaceQuerySelfFaultLog(const uint8_t* data, size_t size)
{
    int32_t faultLogType;
    int32_t count;
    size_t offsetTotalLength = sizeof(faultLogType) + sizeof(count);
    if (offsetTotalLength > size) {
        return;
    }

    STREAM_TO_VALUEINFO(data, faultLogType);
    faultLogType = abs(faultLogType % 10); // 10 : get the absolute value of the last digit of the number
    STREAM_TO_VALUEINFO(data, count);

    HiviewDFX::FaultLogType type = static_cast<HiviewDFX::FaultLogType>(faultLogType);
    auto result = HiviewDFX::QuerySelfFaultLog(type, count);
    if (result != nullptr) {
        while (result->HasNext()) {
            result->Next();
        }
    }
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzInterfaceQuerySelfFaultLog(data, size);
    return 0;
}
