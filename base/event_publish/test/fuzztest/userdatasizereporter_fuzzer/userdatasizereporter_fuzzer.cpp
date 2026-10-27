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

#include "securec.h"
#include "user_data_size_reporter.h"
#include "userdatasizereporter_fuzzer.h"

namespace OHOS {
namespace HiviewDFX {
constexpr int FUZZTEST_MAX_STR_LEN = 64;

void FuzzReportUserDataSize(const uint8_t* data, size_t size)
{
    int32_t uid = 0;
    size_t fixedLen = sizeof(uid) + (2 * FUZZTEST_MAX_STR_LEN); // 2 strings: pathHolder + eventName
    if (fixedLen > size) {
        return;
    }

    errno_t err = memcpy_s(&uid, sizeof(uid), data, sizeof(uid));
    if (err != EOK) {
        return;
    }
    data += sizeof(uid);

    std::string pathHolder(reinterpret_cast<const char*>(data), FUZZTEST_MAX_STR_LEN);
    data += FUZZTEST_MAX_STR_LEN;

    std::string eventName(reinterpret_cast<const char*>(data), FUZZTEST_MAX_STR_LEN);

    UserDataSizeReporter::GetInstance().ReportUserDataSize(uid, pathHolder, eventName);
}
} // namespace
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::HiviewDFX::FuzzReportUserDataSize(data, size);
    return 0;
}

