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
#include "faultloggerserviceaddfaultlog_fuzzer.h"
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

void FuzzServiceInterfaceAddFaultLog(const uint8_t* data, size_t size)
{
    FaultloggerServiceOhos serviceOhos;
    FaultloggerServiceOhos::StartService();
    FaultLogInfoOhos info;
    int32_t faultLogType {0};
    auto offsetTotalLength = sizeof(info.time) + sizeof(info.pid) + sizeof(info.uid) + sizeof(faultLogType) +
                            (6 * FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH); // 6 : Offset by 6 string length
    if (offsetTotalLength > size) {
        return;
    }

    STREAM_TO_VALUEINFO(data, info.time);
    STREAM_TO_VALUEINFO(data, info.pid);
    STREAM_TO_VALUEINFO(data, info.uid);
    STREAM_TO_VALUEINFO(data, faultLogType);
    info.faultLogType = abs(faultLogType % 10); // 10 : get the absolute value of the last digit of the number

    std::string module(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    info.module = module;
    std::string reason(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    info.reason = reason;
    std::string logPath(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    info.logPath = logPath;
    std::string registers(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    info.registers = registers;
    std::string hilog(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    info.sectionMaps["HILOG"] = hilog;
    std::string keyLogFile(reinterpret_cast<const char*>(data), FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH);
    data += FAULTLOGGER_FUZZTEST_MAX_STRING_LENGTH;
    info.sectionMaps["KEYLOGFILE"] = keyLogFile;
    serviceOhos.AddFaultLog(info);
    serviceOhos.Destroy();
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzServiceInterfaceAddFaultLog(data, size);
    return 0;
}
