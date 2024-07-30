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

#include <cstddef>
#include <cstdint>
#include <string>

#include "faultlogger_client.h"
#include "faultlogger_client_fuzzer.h"
#include "faultlogger_fuzzertest_common.h"

namespace OHOS {
const int FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH = 50;

void FuzzInterfaceAddFaultLog(const uint8_t* data, size_t size)
{
    FaultLogInfoInner inner;
    int32_t faultLogType {0};
    int offsetTotalLength = sizeof(inner.time) + sizeof(inner.id) + sizeof(inner.pid) + sizeof(faultLogType) +
                            (4 * FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH); // 4 : Offset by 4 string length
    if (offsetTotalLength > size) {
        return;
    }

    STREAM_TO_VALUEINFO(data, inner.time);
    STREAM_TO_VALUEINFO(data, inner.id);
    STREAM_TO_VALUEINFO(data, inner.pid);
    STREAM_TO_VALUEINFO(data, faultLogType);
    inner.faultLogType = abs(faultLogType % 10); // 10 : get the absolute value of the last digit of the number

    std::string module(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    inner.module = module;
    std::string reason(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    inner.reason = reason;
    std::string summary(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    inner.summary = summary;
    std::string logPath(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    inner.logPath = logPath;
    HiviewDFX::AddFaultLog(inner);
    HiviewDFX::AddFaultLog(inner.time, inner.faultLogType, inner.module, inner.summary);
}

void FuzzInterfaceQuerySelfFaultLog(const uint8_t* data, size_t size)
{
    int32_t faultLogType;
    int32_t count;
    int offsetTotalLength = sizeof(faultLogType) + sizeof(count);
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

void FuzzFaultloggerClientInterface(const uint8_t* data, size_t size)
{
    FuzzInterfaceAddFaultLog(data, size);
    FuzzInterfaceQuerySelfFaultLog(data, size);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzFaultloggerClientInterface(data, size);
    return 0;
}