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

#include "event_publish.h"
#include "eventpublish_isapplistenedevent_fuzzer.h"
#include "securec.h"

namespace OHOS {
namespace HiviewDFX {
constexpr int FUZZTEST_MAX_STRING_LENGTH = 64;

void FuzzIsAppListenedEvent(const uint8_t* data, size_t size)
{
    int32_t uid = 0;
    size_t fixedLen = sizeof(uid) + FUZZTEST_MAX_STRING_LENGTH;
    if (fixedLen > size) {
        return;
    }

    errno_t err = memcpy_s(&uid, sizeof(uid), data, sizeof(uid));
    if (err != EOK) {
        return;
    }
    data += sizeof(uid);

    std::string eventName(reinterpret_cast<const char*>(data), FUZZTEST_MAX_STRING_LENGTH);
    EventPublish::GetInstance().IsAppListenedEvent(uid, eventName);
}
} // namespace
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::HiviewDFX::FuzzIsAppListenedEvent(data, size);
    return 0;
}

