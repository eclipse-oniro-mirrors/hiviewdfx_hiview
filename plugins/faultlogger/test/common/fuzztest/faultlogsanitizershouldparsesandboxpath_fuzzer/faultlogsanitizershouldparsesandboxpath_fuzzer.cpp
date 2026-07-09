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
#include "faultlogsanitizershouldparsesandboxpath_fuzzer.h"
#include "fuzz_data_source.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr size_t MAX_STR_LEN = 200;
}

void FuzzShouldParseSandBoxPath(const uint8_t* data, size_t size)
{
    FuzzDataSource source(data, size);
    std::string line;
    if (!source.GetString(line, MAX_STR_LEN)) {
        return;
    }
    FaultLogSanitizer sanitizer;
    (void)sanitizer.ShouldParseSandBoxPath(line);
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzShouldParseSandBoxPath(data, size);
    return 0;
}
