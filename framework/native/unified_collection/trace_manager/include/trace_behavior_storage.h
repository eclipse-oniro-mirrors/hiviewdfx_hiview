/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_BEHAVIOR_DB_HELPER_H
#define FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_BEHAVIOR_DB_HELPER_H

#include "rdb_store.h"

namespace OHOS::HiviewDFX {
enum BEHAVOIR_ID {
    CACHE_LOW_MEM = 0,
};

struct BehaviorRecord {
    int32_t behaviorId = 0;
    std::string dateNum;
    int32_t usedQuota = 0;
};

class TraceBehaviorStorage {
public:
    explicit TraceBehaviorStorage(std::shared_ptr<NativeRdb::RdbStore> dbStore) : dbStore_(dbStore) {}
    ~TraceBehaviorStorage() = default;

public:
    bool GetRecord(BehaviorRecord &behaviorRecord);
    bool InsertRecord(BehaviorRecord &behaviorRecord);
    bool UpdateRecord(BehaviorRecord &behaviorRecord);
private:
    std::shared_ptr<NativeRdb::RdbStore> dbStore_;
};
} // namespace OHOS::HiviewDFX

#endif // FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_BEHAVIOR_DB_HELPER_H
