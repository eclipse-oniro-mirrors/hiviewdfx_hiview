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

#include "faultlog_cppcrash.h"
#include "faultlog_info_inner.h"
#include "faultlogcppcrashutils_fuzzer.h"
#include "fuzz_data_source.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr size_t MAX_STR_LEN = 200;

bool ReadString(FuzzDataSource& source, std::string& str)
{
    return source.GetString(str, MAX_STR_LEN);
}

void FillFaultLogInfo(FuzzDataSource& source, FaultLogInfo& info)
{
    (void)source.GetValue(info.time);
    (void)source.GetValue(info.pid);
    (void)source.GetValue(info.id);
    (void)source.GetValue(info.faultLogType);
    std::string module;
    if (ReadString(source, module)) {
        info.module = module;
    }
    std::string reason;
    if (ReadString(source, reason)) {
        info.reason = reason;
    }
}
}

void FuzzCppCrashUtils(const uint8_t* data, size_t size)
{
    FuzzDataSource source(data, size);
    std::string hilog;
    if (ReadString(source, hilog)) {
        (void)FaultLogCppCrash::GetLastLineHilogTime(hilog);
    }
    std::string content;
    if (ReadString(source, content)) {
        (void)FaultLogCppCrash::TruncateLogIfExceedsLimit(content);
    }
    FaultLogInfo info;
    FillFaultLogInfo(source, info);
    (void)FaultLogCppCrash::CheckFaultLog(info);
    bool isJsonFile = false;
    (void)source.GetValue(isJsonFile);
    (void)FaultLogCppCrash::GetCppCrashTempLogName(info, isJsonFile);
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzCppCrashUtils(data, size);
    return 0;
}
