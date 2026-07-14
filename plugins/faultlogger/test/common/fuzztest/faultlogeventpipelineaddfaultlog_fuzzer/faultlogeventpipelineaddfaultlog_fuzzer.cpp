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
#include <memory>
#include <string>

#include "constants.h"
#include "faultlog_cjerror.h"
#include "faultlog_jserror.h"
#include "faultlogeventpipelineaddfaultlog_fuzzer.h"
#include "fuzz_data_source.h"
#include "sys_event.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr size_t MAX_STR_LEN = 100;
constexpr uint8_t TYPE_DIVISOR = 2;

void ReadStringAndSetKey(SysEventCreator& creator, FuzzDataSource& source, const char* const key)
{
    std::string val;
    (void)source.GetString(val, MAX_STR_LEN);
    creator.SetKeyValue(key, val);
}

void FillCreatorWithFuzzData(SysEventCreator& creator, FuzzDataSource& source)
{
    int32_t pid = 0;
    int32_t uid = 0;
    int64_t errorManagerCapture = 0;
    int64_t isUncatchFault = 0;
    (void)source.GetValue(pid);
    (void)source.GetValue(uid);
    (void)source.GetValue(errorManagerCapture);
    (void)source.GetValue(isUncatchFault);
    creator.SetKeyValue("pid_", pid);
    creator.SetKeyValue("uid_", uid);
    creator.SetKeyValue(FaultKey::MODULE_PID, pid);
    creator.SetKeyValue(FaultKey::MODULE_UID, uid);
    creator.SetKeyValue(FaultKey::ERRORMANAGER_CAPTURE, errorManagerCapture);
    creator.SetKeyValue(FaultKey::IS_UNCATCH_FAULT, isUncatchFault);
    ReadStringAndSetKey(creator, source, FaultKey::REASON);
    ReadStringAndSetKey(creator, source, FaultKey::MODULE_NAME);
    ReadStringAndSetKey(creator, source, FaultKey::PACKAGE_NAME);
    ReadStringAndSetKey(creator, source, FaultKey::SUMMARY);
    ReadStringAndSetKey(creator, source, FaultKey::HAPPEN_TIME);
    ReadStringAndSetKey(creator, source, FaultKey::PROCESS_RSS_MEMINFO);
    ReadStringAndSetKey(creator, source, FaultKey::P_NAME);
    ReadStringAndSetKey(creator, source, FaultKey::PROCESS_LIFETIME);
    ReadStringAndSetKey(creator, source, FaultKey::THREAD_NAME);
    ReadStringAndSetKey(creator, source, FaultKey::LOG_PATH);
    ReadStringAndSetKey(creator, source, FaultKey::MODULE_VERSION);
    ReadStringAndSetKey(creator, source, FaultKey::FINGERPRINT);
    ReadStringAndSetKey(creator, source, FaultKey::APP_RUNNING_UNIQUE_ID);
    ReadStringAndSetKey(creator, source, FaultKey::FOREGROUND);
    ReadStringAndSetKey(creator, source, FaultKey::IS_SYSTEM_APP);
}
}

void FuzzEventPipelineAddFaultLog(const uint8_t* data, size_t size)
{
    FuzzDataSource source(data, size);
    uint8_t typeFlag = 0;
    (void)source.GetValue(typeFlag);
    SysEventCreator creator("DOMAIN", "EVENT", SysEventCreator::FAULT);
    if (typeFlag % TYPE_DIVISOR == 0) {
        creator.SetKeyValue("name_", "JS_ERROR");
    } else {
        creator.SetKeyValue("name_", "CJ_ERROR");
    }
    FillCreatorWithFuzzData(creator, source);
    auto sysEvent = std::make_shared<SysEvent>("desc", nullptr, creator);
    std::shared_ptr<Event> event = sysEvent;
    if (typeFlag % TYPE_DIVISOR == 0) {
        FaultLogJsError jsError;
        (void)jsError.AddFaultLog(event);
    } else {
        FaultLogCjError cjError;
        (void)cjError.AddFaultLog(event);
    }
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzEventPipelineAddFaultLog(data, size);
    return 0;
}
