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

#include "eventserver_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <securec.h>

#include "decoded/decoded_event.h"
#include "event_server.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr size_t BUFFER_SIZE = 384 * 1024 + 1;
}
static void SysEventServerFuzzTest(const uint8_t* data, size_t size)
{
    if (size < static_cast<int32_t>(EventRaw::GetValidDataMinimumByteCount())) {
        return;
    }
    char* buffer = new char[BUFFER_SIZE]();
    if (memcpy_s(buffer, BUFFER_SIZE, data, size) != EOK) {
        delete[] buffer;
        return;
    }
    buffer[size] = 0;
    int32_t dataByteCnt = *(reinterpret_cast<int32_t*>(buffer));
    if (dataByteCnt != static_cast<int32_t>(size)) {
        delete[] buffer;
        return;
    }
    EventRaw::DecodedEvent event(reinterpret_cast<uint8_t*>(buffer), BUFFER_SIZE);
    (void)event.AsJsonStr();
    delete[] buffer;
}
} // namespace HiviewDFX
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::HiviewDFX::SysEventServerFuzzTest(data, size);
    return 0;
}

