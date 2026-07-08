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

#include "faultlog_sanitizer.h"
#include "faultlogsanitizerextractloadinfo_fuzzer.h"
#include "fuzz_data_source.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr size_t MAX_STR_LEN = 200;
constexpr int32_t MAX_MAP_COUNT = 5;

bool ReadString(FuzzDataSource& source, std::string& str)
{
    return source.GetString(str, MAX_STR_LEN);
}

std::vector<MapInfo> ReadMaps(FuzzDataSource& source)
{
    std::vector<MapInfo> maps;
    int32_t count = 0;
    if (!source.GetValue(count)) {
        return maps;
    }
    count = count % (MAX_MAP_COUNT + 1);
    for (int32_t i = 0; i < count; ++i) {
        MapInfo mi{0, 0, ""};
        if (!source.GetValue(mi.start) || !source.GetValue(mi.end)) {
            break;
        }
        std::string fileName;
        if (!ReadString(source, fileName)) {
            break;
        }
        mi.fileName = fileName;
        maps.push_back(mi);
    }
    return maps;
}
}

void FuzzExtractLoadInfo(const uint8_t* data, size_t size)
{
    FuzzDataSource source(data, size);
    std::string line;
    if (!ReadString(source, line)) {
        return;
    }
    std::string bundleName;
    if (!ReadString(source, bundleName)) {
        return;
    }
    auto maps = ReadMaps(source);
    FaultLogSanitizer sanitizer;
    LoadInfo info{0, 0, 0, ""};
    (void)sanitizer.ExtractLoadInfo(line, maps, bundleName, info);
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzExtractLoadInfo(data, size);
    return 0;
}
