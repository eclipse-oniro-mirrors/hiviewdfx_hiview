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

#include "eventserversocket_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <securec.h>

#include "hiview_platform.h"
#include "sysevent_source.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr size_t BUFFER_SIZE = 384 * 1024 + 1;
}
std::shared_ptr<EventRaw::RawData> ConverRawData(char* source)
{
    if (source == nullptr) {
        return nullptr;
    }
    uint32_t sourceLen = *(reinterpret_cast<uint32_t*>(source));
    uint32_t desLen = sourceLen + sizeof(uint8_t);
    uint8_t* des = reinterpret_cast<uint8_t*>(malloc(desLen));
    if (des == nullptr) {
        return nullptr;
    }
    uint32_t sourceHeaderLen = sizeof(int32_t) + sizeof(EventRaw::HiSysEventHeader) - sizeof(uint8_t);
    if (memcpy_s(des, desLen, source, sourceHeaderLen) != EOK) {
        free(des);
        return nullptr;
    }
    *(reinterpret_cast<uint8_t*>(des + sourceHeaderLen)) = 0; // init header.log flag
    uint32_t desPos = sourceHeaderLen + sizeof(uint8_t);
    if (memcpy_s(des + desPos, desLen - desPos, source + sourceHeaderLen, sourceLen - sourceHeaderLen) != EOK) {
        free(des);
        return nullptr;
    }
    *(reinterpret_cast<int32_t*>(des)) = desLen;
    auto rawData = std::make_shared<EventRaw::RawData>(des, desLen);
    free(des);
    return rawData;
}

void SysEventServerSocketFuzzTest(const uint8_t* data, size_t size)
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
    HiviewPlatform platform;
    SysEventSource source;
    source.SetHiviewContext(&platform);
    source.OnLoad();
    std::shared_ptr<SysEventReceiver> receiver = std::make_shared<SysEventReceiver>(source);
    SocketDevice device("hisysevent", 0);
    if (!device.IsValidMsg(buffer, size)) {
        delete[] buffer;
        return;
    }
    receiver->HandlerEvent(ConverRawData(buffer));
}
} // namespace HiviewDFX
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::HiviewDFX::SysEventServerSocketFuzzTest(data, size);
    return 0;
}

