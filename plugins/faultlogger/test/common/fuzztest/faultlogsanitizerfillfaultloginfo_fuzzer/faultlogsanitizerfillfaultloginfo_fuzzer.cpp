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
#include "faultlog_sanitizer.h"
#include "faultlogsanitizerfillfaultloginfo_fuzzer.h"
#include "fuzz_data_source.h"
#include "sys_event.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr size_t MAX_STR_LEN = 100;

bool ReadString(FuzzDataSource& source, std::string& str)
{
    return source.GetString(str, MAX_STR_LEN);
}
}

void FuzzFillFaultLogInfo(const uint8_t* data, size_t size)
{
    FuzzDataSource source(data, size);
    int32_t pid = 0;
    int32_t uid = 0;
    (void)source.GetValue(pid);
    (void)source.GetValue(uid);
    std::string reason;
    if (!ReadString(source, reason)) {
        return;
    }
    std::string moduleName;
    if (!ReadString(source, moduleName)) {
        return;
    }
    std::string faultType;
    if (!ReadString(source, faultType)) {
        return;
    }
    std::string happenTime;
    if (!ReadString(source, happenTime)) {
        return;
    }

    SysEventCreator creator("DOMAIN", "EVENT", SysEventCreator::FAULT);
    creator.SetKeyValue("name_", "ADDRSAN");
    creator.SetKeyValue("pid_", pid);
    creator.SetKeyValue("uid_", uid);
    creator.SetKeyValue(FaultKey::MODULE_PID, pid);
    creator.SetKeyValue(FaultKey::REASON, reason);
    creator.SetKeyValue(FaultKey::MODULE_NAME, moduleName);
    creator.SetKeyValue(FaultKey::FAULT_TYPE, faultType);
    creator.SetKeyValue(FaultKey::HAPPEN_TIME, happenTime);
    auto sysEvent = std::make_shared<SysEvent>("desc", nullptr, creator);

    FaultLogSanitizer sanitizer;
    (void)sanitizer.FillFaultLogInfo(*sysEvent);
    sanitizer.info_.reason = reason;
    sanitizer.info_.logPath = "/data/test/fuzz_log";
    sanitizer.UpdateFaultLogInfo();
    (void)sanitizer.GetFaultModule(*sysEvent);
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzFillFaultLogInfo(data, size);
    return 0;
}
