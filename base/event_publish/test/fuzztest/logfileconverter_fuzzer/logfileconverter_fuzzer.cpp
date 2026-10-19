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

#include "json/json.h"
#include "log_file_name_converter.h"
#include "logfileconverter_fuzzer.h"
#include "securec.h"

namespace OHOS {
namespace HiviewDFX {
constexpr int FUZZTEST_MAX_STR_LEN = 128;
constexpr int CONVERT_INTERFACE_COUNT = 3;
constexpr int FUZZ_SELECTOR_BYTES = 1;

enum ConvertInterfaceSelector {
    SELECTOR_CONVERT_LOG_FILE_NAME = 0,
    SELECTOR_SHOULD_REFINED_LOG_FILE_NAME = 1,
    SELECTOR_REFINE_LOG_FILE_PATHS = 2,
};

std::string MakeStr(const uint8_t*& data, size_t& remain)
{
    std::string s;
    if (remain >= FUZZTEST_MAX_STR_LEN) {
        s = std::string(reinterpret_cast<const char*>(data), FUZZTEST_MAX_STR_LEN);
        data += FUZZTEST_MAX_STR_LEN;
        remain -= FUZZTEST_MAX_STR_LEN;
    }
    return s;
}

template <typename T>
bool ReadValue(const uint8_t*& data, size_t& remain, T& value)
{
    if (remain < sizeof(T)) {
        return false;
    }
    errno_t err = memcpy_s(&value, sizeof(T), data, sizeof(T));
    if (err != EOK) {
        return false;
    }
    data += sizeof(T);
    remain -= sizeof(T);
    return true;
}

void FuzzConvertLogFileName(const uint8_t*& data, size_t& remain)
{
    std::string oldFileName = MakeStr(data, remain);
    std::string resourceType = MakeStr(data, remain);
    int32_t pid = 0;
    ReadValue(data, remain, pid);
    std::string newFileName;
    ConvertLogFileName(oldFileName, newFileName, pid, resourceType);
}

void FuzzShouldRefinedLogFileName(const uint8_t*& data, size_t& remain)
{
    int32_t uid = 0;
    ReadValue(data, remain, uid);
    std::string pathHolder = MakeStr(data, remain);
    ShouldRefinedLogFileName(uid, pathHolder);
}

void FuzzRefineLogFilePaths(const uint8_t*& data, size_t& remain)
{
    std::string oldLogPath = MakeStr(data, remain);
    int32_t pid = 0;
    ReadValue(data, remain, pid);
    std::string resourceType = MakeStr(data, remain);
    uint8_t rawNeedRefined = 0;
    ReadValue(data, remain, rawNeedRefined);

    Json::Value eventJson;
    eventJson["params"]["pid"] = pid;
    eventJson["params"]["resource_type"] = resourceType;
    std::string newFileName;
    RefineLogFilePaths(eventJson, oldLogPath, newFileName, rawNeedRefined != 0);
}

void FuzzLogFileConverter(const uint8_t* data, size_t size)
{
    if (size < FUZZ_SELECTOR_BYTES) {
        return;
    }
    uint8_t selector = data[0] % CONVERT_INTERFACE_COUNT;
    data += FUZZ_SELECTOR_BYTES;
    size_t remain = size - FUZZ_SELECTOR_BYTES;
    switch (selector) {
        case SELECTOR_CONVERT_LOG_FILE_NAME:
            FuzzConvertLogFileName(data, remain);
            break;
        case SELECTOR_SHOULD_REFINED_LOG_FILE_NAME:
            FuzzShouldRefinedLogFileName(data, remain);
            break;
        case SELECTOR_REFINE_LOG_FILE_PATHS:
            FuzzRefineLogFilePaths(data, remain);
            break;
        default:
            break;
    }
}
} // namespace
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::HiviewDFX::FuzzLogFileConverter(data, size);
    return 0;
}
