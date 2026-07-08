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

#include "constants.h"
#include "faultlog_database.h"
#include "faultlog_info_inner.h"
#include "faultlogdatabasefillinfo_fuzzer.h"
#include "fuzz_data_source.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr size_t MAX_STR_LEN = 100;

bool ReadString(FuzzDataSource& source, std::string& str)
{
    return source.GetString(str, MAX_STR_LEN);
}
}

void FuzzDatabaseFillInfo(const uint8_t* data, size_t size)
{
    FuzzDataSource source(data, size);
    FaultLogInfo info;
    int32_t faultLogType = 0;
    if (source.GetValue(faultLogType)) {
        info.faultLogType = faultLogType % FaultLogType::MAX_TYPE;
    }
    std::string reason;
    if (!ReadString(source, reason)) {
        return;
    }
    info.reason = reason;
    std::string summary;
    if (!ReadString(source, summary)) {
        return;
    }
    info.summary = summary;
    std::string sectionValue;
    if (ReadString(source, sectionValue)) {
        info.sectionMap[FaultKey::IS_SIG_ACTION] = sectionValue;
    }
    FaultLogDatabase::FillInfoDefault(info);
    (void)FaultLogDatabase::UpdateFGParam(info);
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzDatabaseFillInfo(data, size);
    return 0;
}
