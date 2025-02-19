/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_STORAGE_H
#define FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_STORAGE_H

#include <cstdint>
#include <memory>
#include <string>

#include "app_event_task_storage.h"
#include "rdb_store.h"

namespace OHOS {
namespace HiviewDFX {
struct TraceFlowRecord {
    std::string systemTime;
    std::string callerName;
    int64_t usedSize = 0;
};

class TraceStorage {
public:
    TraceStorage(std::shared_ptr<NativeRdb::RdbStore> dbStore, const std::string& caller);
    ~TraceStorage() = default;

    bool NeedDump();
    bool NeedUpload(int64_t traceSize);
    void StoreDb();

private:
    void InitTableRecord();
    void Store(const TraceFlowRecord& traceFlowRecord);
    void Query(TraceFlowRecord& traceFlowRecord);
    void InsertTable(const TraceFlowRecord& traceFlowRecord);
    void QueryTable(TraceFlowRecord& traceFlowRecord);
    void UpdateTable(const TraceFlowRecord& traceFlowRecord);
    std::string GetDate();
    bool IsLowerLimit(int64_t nowSize, int64_t traceSize, int64_t limitSize);

private:
    TraceFlowRecord traceFlowRecord_;
    std::string caller_;
    std::shared_ptr<NativeRdb::RdbStore> dbStore_;
}; // TraceStorage
} // namespace HiviewDFX
} // namespace OHOS
#endif // FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_STORAGE_H
