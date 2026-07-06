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
#include "faultloggerclientgwpasangrayscale_fuzzer.h"
#include "faultlogger_fuzzertest_common.h"

namespace OHOS {
void FuzzInterfaceGwpAsanGrayscale(const uint8_t* data, size_t size)
{
    bool alwaysEnabled;
    double sampleRate;
    double maxSimutaneousAllocations;
    int32_t duration;
    auto offsetTotalLength = sizeof(alwaysEnabled) + sizeof(sampleRate) +
        sizeof(maxSimutaneousAllocations) + sizeof(duration);
    if (offsetTotalLength > size) {
        return;
    }

    STREAM_TO_VALUEINFO(data, alwaysEnabled);
    STREAM_TO_VALUEINFO(data, sampleRate);
    STREAM_TO_VALUEINFO(data, maxSimutaneousAllocations);
    STREAM_TO_VALUEINFO(data, duration);

    HiviewDFX::EnableGwpAsanGrayscale(alwaysEnabled, sampleRate, maxSimutaneousAllocations, duration, true);
    HiviewDFX::DisableGwpAsanGrayscale();
    HiviewDFX::GetGwpAsanGrayscaleState();
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzInterfaceGwpAsanGrayscale(data, size);
    return 0;
}
