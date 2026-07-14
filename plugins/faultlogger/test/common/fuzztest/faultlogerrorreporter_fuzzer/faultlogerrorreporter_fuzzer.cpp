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
#include <memory>
#include <string>

#include "constants.h"
#include "faultlog_error_reporter.h"
#include "faultlogerrorreporter_fuzzer.h"
#include "fuzz_data_source.h"
#include "sys_event.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr size_t MAX_STR_LEN = 100;

void ReadStringAndSetKey(SysEventCreator& creator, FuzzDataSource& source, const char* const key)
{
    std::string val;
    (void)source.GetString(val, MAX_STR_LEN);
    creator.SetKeyValue(key, val);
}

void ReadStringAndSetMap(std::map<std::string, std::string>& sectionMap, FuzzDataSource& source,
    const char* const key)
{
    std::string val;
    (void)source.GetString(val, MAX_STR_LEN);
    sectionMap[key] = val;
}
}

void FuzzErrorReporter(const uint8_t* data, size_t size)
{
    FuzzDataSource source(data, size);
    int32_t pid = 0;
    int32_t uid = 0;
    int64_t errorManagerCapture = 0;
    int64_t isUncatchFault = 0;
    (void)source.GetValue(pid);
    (void)source.GetValue(uid);
    (void)source.GetValue(errorManagerCapture);
    (void)source.GetValue(isUncatchFault);

    SysEventCreator creator("DOMAIN", "EVENT", SysEventCreator::FAULT);
    creator.SetKeyValue("name_", "FUZZ_ERROR");
    creator.SetKeyValue("pid_", pid);
    creator.SetKeyValue("uid_", uid);
    creator.SetKeyValue(FaultKey::ERRORMANAGER_CAPTURE, errorManagerCapture);
    creator.SetKeyValue(FaultKey::IS_UNCATCH_FAULT, isUncatchFault);
    ReadStringAndSetKey(creator, source, FaultKey::SUMMARY);
    ReadStringAndSetKey(creator, source, FaultKey::PACKAGE_NAME);
    ReadStringAndSetKey(creator, source, FaultKey::P_NAME);
    ReadStringAndSetKey(creator, source, FaultKey::THREAD_NAME);
    ReadStringAndSetKey(creator, source, FaultKey::LOG_PATH);
    ReadStringAndSetKey(creator, source, FaultKey::MODULE_VERSION);
    ReadStringAndSetKey(creator, source, FaultKey::FINGERPRINT);
    ReadStringAndSetKey(creator, source, FaultKey::APP_RUNNING_UNIQUE_ID);
    ReadStringAndSetKey(creator, source, FaultKey::FOREGROUND);
    ReadStringAndSetKey(creator, source, FaultKey::PROCESS_LIFETIME);
    ReadStringAndSetKey(creator, source, FaultKey::PROCESS_RSS_MEMINFO);
    auto sysEvent = std::make_shared<SysEvent>("desc", nullptr, creator);

    std::map<std::string, std::string> sectionMap;
    ReadStringAndSetMap(sectionMap, source, FaultKey::RELEASE_TYPE);
    ReadStringAndSetMap(sectionMap, source, FaultKey::CPU_ABI);
    ReadStringAndSetMap(sectionMap, source, FaultKey::SYS_FREE_MEM);
    ReadStringAndSetMap(sectionMap, source, FaultKey::SYS_TOTAL_MEM);
    ReadStringAndSetMap(sectionMap, source, FaultKey::SYS_AVAIL_MEM);
    ReadStringAndSetMap(sectionMap, source, FaultKey::PROCESS_RSS_MEMINFO);
    uint8_t typeFlag = 0;
    (void)source.GetValue(typeFlag);
    std::string type = (typeFlag % 2 == 0) ? "JsError" : "CjError";
    std::string outputPath = "/data/test/fuzz_error_reporter_info";
    FaultLogErrorReporter::ReportErrorToAppEvent(sysEvent, type, outputPath, sectionMap);
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzErrorReporter(data, size);
    return 0;
}
