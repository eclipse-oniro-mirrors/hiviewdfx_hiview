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
#include <vector>

#include "securec.h"
#define private public
#define protected public
#include "user_data_size_reporter.cpp"
#undef private
#undef protected
#include "userdatasizereporter_fuzzer.h"

namespace OHOS {
namespace HiviewDFX {
constexpr int FUZZTEST_MAX_STR_LEN = 64;
constexpr int FUZZ_SELECTOR_BYTES = 1;

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

std::string MakeEventName(const uint8_t*& data, size_t& remain)
{
    std::string s;
    for (size_t i = 0; i < FUZZTEST_MAX_STR_LEN && remain > 0; ++i) {
        char c = static_cast<char>(*data);
        data++;
        remain--;
        if (c == '\0') {
            break;
        }
        s.push_back(c);
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

void FuzzReportUserDataSize(const uint8_t*& data, size_t& remain)
{
    int32_t uid = 0;
    if (!ReadValue(data, remain, uid)) {
        return;
    }
    std::string pathHolder = MakeStr(data, remain);
    std::string eventName = MakeEventName(data, remain);
    UserDataSizeReporter::GetInstance().ReportUserDataSize(uid, pathHolder, eventName);
}

void FuzzShouldReport(const uint8_t*& data, size_t& remain)
{
    std::string pathLimmitKey = MakeStr(data, remain);
    UserDataSizeReporter::GetInstance().ShouldReport(pathLimmitKey);
}

void FuzzClearOverTimeRecord(const uint8_t*& data, size_t& remain)
{
    UserDataSizeReporter::GetInstance().ClearOverTimeRecord();
}

void FuzzMayPushBackPath(const uint8_t*& data, size_t& remain)
{
    std::string path = MakeStr(data, remain);
    std::vector<std::string> paths;
    MayPushBackPath(paths, path);
}

void FuzzGetReportPath(const uint8_t*& data, size_t& remain)
{
    int32_t uid = 0;
    if (!ReadValue(data, remain, uid)) {
        return;
    }
    std::string pathHolder = MakeStr(data, remain);
    std::string eventName = MakeEventName(data, remain);
    GetReportPath(uid, pathHolder, eventName);
}

void FuzzGetFoldersSize(const uint8_t*& data, size_t& remain)
{
    std::vector<std::string> dirs;
    dirs.push_back(MakeStr(data, remain));
    GetFoldersSize(dirs);
}

void FuzzDoReport(const uint8_t*& data, size_t& remain)
{
    int32_t uid = 0;
    if (!ReadValue(data, remain, uid)) {
        return;
    }
    std::string pathHolder = MakeStr(data, remain);
    std::string eventName = MakeEventName(data, remain);
    DoReport(uid, pathHolder, eventName);
}

using FuzzFunc = void(*)(const uint8_t*&, size_t&);

const FuzzFunc FUZZ_FUNCS[] = {
    FuzzReportUserDataSize,
    FuzzShouldReport,
    FuzzClearOverTimeRecord,
    FuzzMayPushBackPath,
    FuzzGetReportPath,
    FuzzGetFoldersSize,
    FuzzDoReport,
};

void FuzzUserDataSizeReporter(const uint8_t* data, size_t size)
{
    if (size < FUZZ_SELECTOR_BYTES) {
        return;
    }
    constexpr size_t funcCount = sizeof(FUZZ_FUNCS) / sizeof(FUZZ_FUNCS[0]);
    uint8_t selector = data[0] % funcCount;
    data += FUZZ_SELECTOR_BYTES;
    size_t remain = size - FUZZ_SELECTOR_BYTES;
    FUZZ_FUNCS[selector](data, remain);
}
} // namespace HiviewDFX
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::HiviewDFX::FuzzUserDataSizeReporter(data, size);
    return 0;
}
