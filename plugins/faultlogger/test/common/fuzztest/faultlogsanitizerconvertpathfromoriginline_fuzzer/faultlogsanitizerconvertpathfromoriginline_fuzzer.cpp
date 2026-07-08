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

#include "faultlog_sanitizer.h"
#include "faultlogsanitizerconvertpathfromoriginline_fuzzer.h"
#include "fuzz_data_source.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr size_t MAX_STR_LEN = 200;

bool ReadString(FuzzDataSource& source, std::string& str)
{
    return source.GetString(str, MAX_STR_LEN);
}
}

void FuzzConvertPathFromOriginLine(const uint8_t* data, size_t size)
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
    std::string pathPrefix;
    FaultLogSanitizer sanitizer;
    (void)sanitizer.ConvertPathFromOriginLine(line, pathPrefix, bundleName);
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzConvertPathFromOriginLine(data, size);
    return 0;
}
