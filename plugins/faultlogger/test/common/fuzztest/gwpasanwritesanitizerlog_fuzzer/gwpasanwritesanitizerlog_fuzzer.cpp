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
#include <vector>

#include "fuzz_data_source.h"
#include "gwpasan_collector.h"
#include "gwpasanwritesanitizerlog_fuzzer.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr size_t MAX_STR_LEN = 256;
constexpr size_t MAX_BUF_LEN = 1024;

void FuzzWriteSanitizerLog(const uint8_t* data, size_t size)
{
    FuzzDataSource source(data, size);
    size_t bufLen = 0;
    if (!source.GetValue(bufLen) || bufLen == 0 || bufLen > MAX_BUF_LEN) {
        bufLen = MAX_BUF_LEN;
    }
    size_t actualLen = source.GetRemainingLength();
    if (actualLen == 0) {
        return;
    }
    if (bufLen > actualLen) {
        bufLen = actualLen;
    }
    std::vector<char> buf(bufLen);
    const uint8_t* srcData = source.GetCurrentData();
    for (size_t i = 0; i < bufLen; i++) {
        buf[i] = static_cast<char>(srcData[i]);
    }
    source.Advance(bufLen);
    std::string pathStr;
    (void)source.GetString(pathStr, MAX_STR_LEN);
    std::vector<char> path(pathStr.size() + 1, '\0');
    for (size_t i = 0; i < pathStr.size(); i++) {
        path[i] = pathStr[i];
    }
    WriteSanitizerLog(buf.data(), bufLen, path.data());
}
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzWriteSanitizerLog(data, size);
    return 0;
}
