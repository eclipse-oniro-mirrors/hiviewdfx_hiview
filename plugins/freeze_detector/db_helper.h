/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef FREEZE_DB_HELPER_H
#define FREEZE_DB_HELPER_H

#include <list>
#include <memory>
#include <string>

#include "sys_event_dao.h"
#include "freeze_common.h"
#include "watch_point.h"

namespace OHOS {
namespace HiviewDFX {
class DBHelper {
public:
    struct WatchParams {
        long pid;
        std::string packageName;
    };

    explicit DBHelper(std::shared_ptr<FreezeCommon> fc) : freezeCommon_(fc) {};
    ~DBHelper() {};
    void GetResultMap(const struct WatchParams& watchParams, const FreezeResult& result,
        EventStore::ResultSet& set, std::map<std::string, WatchPoint>& resultMap);
    void SelectEventFromDB(unsigned long long start, unsigned long long end, std::vector<WatchPoint>& list,
        const struct WatchParams& watchParams, const FreezeResult& result);
private:
    std::shared_ptr<FreezeCommon> freezeCommon_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif
