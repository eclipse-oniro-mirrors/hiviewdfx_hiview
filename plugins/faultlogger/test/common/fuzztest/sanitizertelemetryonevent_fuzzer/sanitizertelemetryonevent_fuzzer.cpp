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

#include "event.h"
#include "fuzz_data_source.h"
#include "sanitizer_telemetry.h"
#include "sanitizertelemetryonevent_fuzzer.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr size_t MAX_STR_LEN = 100;

void ReadStringAndSetValue(Event& event, FuzzDataSource& source, const std::string& key)
{
    std::string val;
    (void)source.GetString(val, MAX_STR_LEN);
    event.SetValue(key, val);
}
}

void FuzzTelemetryOnEvent(const uint8_t* data, size_t size)
{
    FuzzDataSource source(data, size);
    Event event("fuzz_sender");
    event.messageType_ = Event::TELEMETRY_EVENT;
    ReadStringAndSetValue(event, source, "bundleName");
    ReadStringAndSetValue(event, source, "telemetryStatus");
    ReadStringAndSetValue(event, source, "fault");
    ReadStringAndSetValue(event, source, "telemetryId");
    ReadStringAndSetValue(event, source, "gwpEnable");
    ReadStringAndSetValue(event, source, "gwpSampleRate");
    ReadStringAndSetValue(event, source, "gwpMaxSlots");
    ReadStringAndSetValue(event, source, "minSampleSize");
    ReadStringAndSetValue(event, source, "stackParam");
    SanitizerTelemetry telemetry;
    telemetry.OnUnorderedEvent(event);
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzTelemetryOnEvent(data, size);
    return 0;
}
