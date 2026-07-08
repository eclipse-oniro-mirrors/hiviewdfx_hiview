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

#include "faultlog_formatter.h"
#include "faultlogformatterformatjson_fuzzer.h"
#include "fuzz_data_source.h"
#include "json/json.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr size_t MAX_STR_LEN = 500;
}

void FuzzFormatJson(const uint8_t* data, size_t size)
{
    FuzzDataSource source(data, size);
    std::string jsonStr;
    if (!source.GetString(jsonStr, MAX_STR_LEN)) {
        return;
    }
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(jsonStr, root)) {
        return;
    }
    if (root.isObject()) {
        (void)FaultLogger::FormatThreadInfo(root);
        (void)FaultLogger::FormatAppLogConfig(root);
        std::map<std::string, std::string> sectionMap;
        FaultLogger::FillSectionMapFromJson(root, sectionMap);
    }
    if (root.isArray()) {
        (void)FaultLogger::FormatOtherThreadInfo(root);
    }
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzFormatJson(data, size);
    return 0;
}
