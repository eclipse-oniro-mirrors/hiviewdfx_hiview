/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "hisysevent_util.h"

#include "sys_event_dao.h"
namespace OHOS {
namespace HiviewDFX {
namespace HisysEventUtil {

bool IsEventProcessed(const std::string& name, const std::string& key, const std::string& value)
{
    auto sysEventQuery = EventStore::SysEventDao::BuildQuery("KERNEL_VENDOR", {name});
    if (sysEventQuery == nullptr) {
        return false;
    }
    std::vector<std::string> selections{EventStore::EventCol::TS};
    EventStore::ResultSet resultSet = sysEventQuery->Select(selections).
        Where(key, EventStore::Op::EQ, value).Execute();
    return resultSet.HasNext();
}
}
}
}