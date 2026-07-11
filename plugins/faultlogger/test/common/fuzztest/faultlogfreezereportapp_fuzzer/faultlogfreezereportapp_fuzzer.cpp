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
#include "faultlog_freeze.h"
#include "faultlog_info_inner.h"
#include "faultlogfreezereportapp_fuzzer.h"
#include "fuzz_data_source.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr size_t MAX_STR_LEN = 100;
constexpr uint8_t FREEZE_TYPE_DIVISOR = 2;

void ReadStrAndSetMap(std::map<std::string, std::string>& map, FuzzDataSource& source, const char* const key)
{
    std::string val;
    (void)source.GetString(val, MAX_STR_LEN);
    map[key] = val;
}

void FillFreezeSectionMap(FaultLogFreeze& freeze, FuzzDataSource& source)
{
    auto& sm = freeze.info_.sectionMap;
    ReadStrAndSetMap(sm, source, FaultKey::FOREGROUND);
    ReadStrAndSetMap(sm, source, FaultKey::MODULE_VERSION);
    ReadStrAndSetMap(sm, source, FaultKey::VERSION_CODE);
    ReadStrAndSetMap(sm, source, FaultKey::FINGERPRINT);
    ReadStrAndSetMap(sm, source, FaultKey::PROCESS_LIFETIME);
    ReadStrAndSetMap(sm, source, FaultKey::CPU_ABI);
    ReadStrAndSetMap(sm, source, FaultKey::RELEASE_TYPE);
    ReadStrAndSetMap(sm, source, FaultKey::APP_RUNNING_UNIQUE_ID);
    ReadStrAndSetMap(sm, source, FaultKey::FAULT_MESSAGE);
    ReadStrAndSetMap(sm, source, FaultKey::HILOG);
    ReadStrAndSetMap(sm, source, "EVENT_HANDLER");
    ReadStrAndSetMap(sm, source, "EVENT_HANDLER_SIZE_3S");
    ReadStrAndSetMap(sm, source, "EVENT_HANDLER_SIZE_6S");
    ReadStrAndSetMap(sm, source, "PEER_BINDER");
    ReadStrAndSetMap(sm, source, "THREADS");
    ReadStrAndSetMap(sm, source, FaultKey::THERMAL_LEVEL);
    ReadStrAndSetMap(sm, source, FaultKey::EXTERNAL_CALLBACK_LOG);
    ReadStrAndSetMap(sm, source, FaultKey::FREEZE_INFO_PATH);
    ReadStrAndSetMap(sm, source, FaultKey::ENABLE_MAINTHREAD_SAMPLE);
    ReadStrAndSetMap(sm, source, FaultKey::PROCESS_RSS_MEMINFO);
    ReadStrAndSetMap(sm, source, FaultKey::PROCESS_VSS_MEMINFO);
    ReadStrAndSetMap(sm, source, FaultKey::SYS_FREE_MEM);
    ReadStrAndSetMap(sm, source, FaultKey::SYS_TOTAL_MEM);
    ReadStrAndSetMap(sm, source, FaultKey::SYS_AVAIL_MEM);
    ReadStrAndSetMap(sm, source, FaultKey::HEAP_TOTAL_SIZE);
    ReadStrAndSetMap(sm, source, FaultKey::HEAP_OBJECT_SIZE);
    ReadStrAndSetMap(sm, source, FaultKey::HEAP_SHARED_SIZE);
    ReadStrAndSetMap(sm, source, "TERMINAL_THREAD_STACK");
    ReadStrAndSetMap(sm, source, "PROCESS_NAME");
}
}

void FuzzFreezeReportApp(const uint8_t* data, size_t size)
{
    FuzzDataSource source(data, size);
    uint8_t typeFlag = 0;
    int32_t pid = 0;
    int32_t uid = 0;
    (void)source.GetValue(typeFlag);
    (void)source.GetValue(pid);
    (void)source.GetValue(uid);
    FaultLogFreeze freeze;
    freeze.info_.faultLogType =
        (typeFlag % FREEZE_TYPE_DIVISOR == 0) ? FaultLogType::APP_FREEZE : FaultLogType::APPFREEZE_WARNING;
    freeze.info_.pid = pid;
    freeze.info_.id = uid;
    freeze.info_.reportToAppEvent = true;
    (void)source.GetString(freeze.info_.module, MAX_STR_LEN);
    (void)source.GetString(freeze.info_.reason, MAX_STR_LEN);
    (void)source.GetString(freeze.info_.summary, MAX_STR_LEN);
    (void)source.GetString(freeze.info_.logPath, MAX_STR_LEN);
    FillFreezeSectionMap(freeze, source);
    freeze.UpdateCommonInfo();
    freeze.UpdateFaultLogInfo();
    (void)freeze.ReportEventToAppEvent();
    std::string logPath;
    std::string freezeExtPath;
    (void)source.GetString(logPath, MAX_STR_LEN);
    (void)source.GetString(freezeExtPath, MAX_STR_LEN);
    (void)freeze.MergeFreezeExtToLog(logPath, freezeExtPath, pid, uid);
    (void)freeze.GetFreezeJsonCollector(freeze.info_);
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzFreezeReportApp(data, size);
    return 0;
}
