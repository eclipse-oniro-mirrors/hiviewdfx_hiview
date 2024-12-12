/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_BASE_EVENT_STORE_INCLUDE_SYS_EVENT_DAO_H
#define HIVIEW_BASE_EVENT_STORE_INCLUDE_SYS_EVENT_DAO_H

#include <memory>
#include <string>
#include <vector>

#include "sys_event.h"
#include "sys_event_query.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventStore {
class SysEventDao {
public:
    /* for internal query */
    static std::shared_ptr<SysEventQuery> BuildQuery(const std::string& domain, const std::vector<std::string>& names);

    /* for external query */
    static std::shared_ptr<SysEventQuery> BuildQuery(const std::string& domain,
        const std::vector<std::string>& names, uint32_t type, int64_t toSeq, int64_t fromSeq);

    static int Insert(std::shared_ptr<SysEvent> sysEvent);
    static void CheckRepeat(SysEvent& event);
    static void Backup();
    static void Restore();
    static std::string GetDatabaseDir();
    static void Clear();
}; // SysEventDao
} // EventStore
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_BASE_EVENT_STORE_INCLUDE_SYS_EVENT_DAO_H
