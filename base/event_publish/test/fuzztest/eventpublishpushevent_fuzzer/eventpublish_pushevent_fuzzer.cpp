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
#include <cstring>
#include <string>

#include "event_publish.h"
#include "eventpublish_pushevent_fuzzer.h"
#include "hisysevent.h"
#include "securec.h"

namespace OHOS {
namespace HiviewDFX {
constexpr int FUZZTEST_MAX_STRING_LENGTH = 256;
constexpr int EVENT_TYPE_COUNT = 4;

void FuzzPushEvent(const uint8_t* data, size_t size)
{
    int32_t uid = 0;
    uint8_t eventTypeRaw = 0;
    uint32_t maxFileSizeBytes = 0;
    size_t fixedLen = sizeof(uid) + sizeof(eventTypeRaw) + sizeof(maxFileSizeBytes) +
                      FUZZTEST_MAX_STRING_LENGTH;
    if (fixedLen > size) {
        return;
    }

    errno_t err = memcpy_s(&uid, sizeof(uid), data, sizeof(uid));
    if (err != EOK) {
        return;
    }
    data += sizeof(uid);

    err = memcpy_s(&eventTypeRaw, sizeof(eventTypeRaw), data, sizeof(eventTypeRaw));
    if (err != EOK) {
        return;
    }
    data += sizeof(eventTypeRaw);

    err = memcpy_s(&maxFileSizeBytes, sizeof(maxFileSizeBytes), data, sizeof(maxFileSizeBytes));
    if (err != EOK) {
        return;
    }
    data += sizeof(maxFileSizeBytes);

    std::string eventName(reinterpret_cast<const char*>(data), FUZZTEST_MAX_STRING_LENGTH);
    data += FUZZTEST_MAX_STRING_LENGTH;

    // Use the remaining bytes as paramJson payload.
    std::string paramJson(reinterpret_cast<const char*>(data), size - fixedLen);

    HiSysEvent::EventType eventType = static_cast<HiSysEvent::EventType>(eventTypeRaw % EVENT_TYPE_COUNT);
    EventPublish::GetInstance().PushEvent(uid, eventName, eventType, paramJson, maxFileSizeBytes);
}
} // namespace
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::HiviewDFX::FuzzPushEvent(data, size);
    return 0;
}

