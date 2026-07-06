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
#include "faultlogger.h"
#include "faultlogger_service_ohos.h"
#include "faultloggerservicecppcrash_fuzzer.h"
#include "constants.h"
#include "export_faultlogger_interface.h"
#include "faultlog_info_inner.h"
#include "faultlogger_fuzzertest_common.h"
#include "faultlogger_service_fuzzer_helper.h"
#include "hiview_global.h"
#include "hiview_platform.h"
#include "sys_event.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr int FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH = 50;
}

void FuzzServiceInterfaceCppCrash(const uint8_t* data, size_t size)
{
    static HiviewTestContext hiviewTestContext;
    HiviewGlobal::CreateInstance(hiviewTestContext);
    auto service = CreateFaultloggerInstance();

    int32_t pid;
    int32_t uid;
    int32_t tid;
    auto offsetTotalLength = sizeof(pid) + sizeof(uid) + sizeof(tid) +
                            (7 * FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    if (offsetTotalLength > size) {
        return;
    }

    STREAM_TO_VALUEINFO(data, pid);
    STREAM_TO_VALUEINFO(data, uid);
    STREAM_TO_VALUEINFO(data, tid);
    std::string domain(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    std::string eventName(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    SysEventCreator sysEventCreator(domain, eventName, SysEventCreator::FAULT);
    std::map<std::string, std::string> bundle;
    std::string hilog(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    bundle["HILOG"] = hilog;
    std::string keyLogFile(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    bundle["KEYLOGFILE"] = keyLogFile;
    std::string summary(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    std::string moduleName(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    sysEventCreator.SetKeyValue("name_", "CPP_CRASH");
    sysEventCreator.SetKeyValue("pid_", pid);
    sysEventCreator.SetKeyValue("uid_", uid);
    sysEventCreator.SetKeyValue("tid_", tid);
    sysEventCreator.SetKeyValue(FaultKey::MODULE_PID, pid);
    sysEventCreator.SetKeyValue(FaultKey::MODULE_NAME, moduleName);
    sysEventCreator.SetKeyValue(FaultKey::SUMMARY, summary);
    sysEventCreator.SetKeyValue("bundle_", bundle);
    std::string desc(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    auto sysEvent = std::make_shared<SysEvent>(desc, nullptr, sysEventCreator);
    auto event = std::dynamic_pointer_cast<Event>(sysEvent);
    service->OnEvent(event);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzServiceInterfaceCppCrash(data, size);
    return 0;
}
