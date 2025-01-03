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

#ifndef SYS_EVENT_REPEAT_DB_H
#define SYS_EVENT_REPEAT_DB_H

#include <mutex>
#include <string>

#include "rdb_store.h"
#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {
struct SysEventHashRecord {
    SysEventHashRecord(const std::string& domain, const std::string& name)
        : domain(domain), name(name) {}
    std::string domain;
    std::string name;
    std::string eventHash;
    int64_t happentime = 0;
};

class SysEventRepeatDb : public OHOS::DelayedRefSingleton<SysEventRepeatDb> {
public:
    SysEventRepeatDb();
    ~SysEventRepeatDb() = default;
    bool Insert(const SysEventHashRecord &sysEventHashRecord);
    int64_t QueryHappentime(SysEventHashRecord &sysEventHashRecord);
    bool Update(const SysEventHashRecord &sysEventHashRecord);
    void CheckAndClearDb(const int64_t happentime);
    void Clear(const int64_t happentime);
    void Release();

private:
    bool CheckDbStoreValid();
    void InitDbStore();
    void ClearHistory(const int64_t happentime);
    void RefreshDbCount();
    void CheckAndRepairDbFile(const int32_t errCode);

private:
    std::shared_ptr<NativeRdb::RdbStore> dbStore_;
    int64_t dbCount_ = 0;
    std::mutex dbMutex_;
};
} // HiviewDFX
} // OHOS
#endif // SYS_EVENT_REPEAT_DB_H