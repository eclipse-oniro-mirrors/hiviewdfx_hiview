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
#include <map>
#include <string>

#include "faultlog_freeze.h"
#include "faultlog_info_inner.h"
#include "faultlogfreezeutils_fuzzer.h"
#include "fuzz_data_source.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr size_t MAX_STR_LEN = 100;
constexpr int32_t MAX_MAP_ENTRY = 5;

bool ReadString(FuzzDataSource& source, std::string& str)
{
    return source.GetString(str, MAX_STR_LEN);
}

std::map<std::string, std::string> ReadSectionMap(FuzzDataSource& source)
{
    std::map<std::string, std::string> sectionMap;
    int32_t count = 0;
    if (!source.GetValue(count)) {
        return sectionMap;
    }
    count = count % (MAX_MAP_ENTRY + 1);
    for (int32_t i = 0; i < count; ++i) {
        std::string key;
        if (!ReadString(source, key)) {
            break;
        }
        std::string value;
        if (!ReadString(source, value)) {
            break;
        }
        sectionMap[key] = value;
    }
    return sectionMap;
}
}

void FuzzFreezeUtils(const uint8_t* data, size_t size)
{
    FuzzDataSource source(data, size);
    std::string name;
    if (!ReadString(source, name)) {
        return;
    }
    std::string message;
    if (!ReadString(source, message)) {
        return;
    }
    int32_t faultLogType = 0;
    (void)source.GetValue(faultLogType);
    bool isAppHicollie = false;
    (void)source.GetValue(isAppHicollie);
    auto sectionMap = ReadSectionMap(source);
    bool includePss = false;
    (void)source.GetValue(includePss);

    FaultLogFreeze freeze;
    (void)freeze.GetException(name, message);
    (void)freeze.GetEventType(static_cast<FaultLogType>(faultLogType % FaultLogType::MAX_TYPE), isAppHicollie);
    (void)freeze.GetFreezeType(static_cast<FaultLogType>(faultLogType % FaultLogType::MAX_TYPE), isAppHicollie);
    (void)freeze.GetGCJsonValue(sectionMap);
    (void)freeze.GetIOJsonValue(sectionMap);
    (void)freeze.GetMemoryStrByPid(sectionMap, includePss);
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzFreezeUtils(data, size);
    return 0;
}
