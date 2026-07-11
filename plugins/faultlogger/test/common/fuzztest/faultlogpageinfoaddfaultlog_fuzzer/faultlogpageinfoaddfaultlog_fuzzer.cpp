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
#include "faultlog_page_info.h"
#include "faultlogpageinfoaddfaultlog_fuzzer.h"
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
}

void FuzzPageInfoAddFaultLog(const uint8_t* data, size_t size)
{
    FuzzDataSource source(data, size);
    int32_t pid = 0;
    int32_t uid = 0;
    (void)source.GetValue(pid);
    (void)source.GetValue(uid);
    SysEventCreator creator("DOMAIN", "EVENT", SysEventCreator::FAULT);
    creator.SetKeyValue("name_", "PROCESS_PAGE_INFO");
    creator.SetKeyValue("pid_", pid);
    creator.SetKeyValue("uid_", uid);
    creator.SetKeyValue(FaultKey::MODULE_PID, pid);
    creator.SetKeyValue(FaultKey::MODULE_UID, uid);
    ReadStringAndSetKey(creator, source, FaultKey::MODULE_NAME);
    ReadStringAndSetKey(creator, source, FaultKey::REASON);
    ReadStringAndSetKey(creator, source, FaultKey::SUMMARY);
    ReadStringAndSetKey(creator, source, FaultKey::HAPPEN_TIME);
    ReadStringAndSetKey(creator, source, "PROCESS_NAME");
    ReadStringAndSetKey(creator, source, FaultKey::LOG_PATH);
    auto sysEvent = std::make_shared<SysEvent>("desc", nullptr, creator);
    std::shared_ptr<Event> event = sysEvent;
    FaultLogPageInfo pageInfo;
    (void)pageInfo.AddFaultLog(event);
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzPageInfoAddFaultLog(data, size);
    return 0;
}
