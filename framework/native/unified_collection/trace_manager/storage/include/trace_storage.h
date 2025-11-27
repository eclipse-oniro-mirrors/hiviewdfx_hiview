/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include <memory>
#include <string>

#include "rdb_store.h"

namespace OHOS::HiviewDFX {
struct TraceFlowRecord {
    std::string systemTime;
    std::string callerName;
    int64_t usedIoSize = 0;
    int64_t usedZipSize = 0;
    int64_t dynamicDecrease = 0;
};

class TraceStorage {
public:
    TraceStorage(std::shared_ptr<NativeRdb::RdbStore> dbStore, const std::string& name,
        const std::string& configPath);
    ~TraceStorage() = default;

    int64_t GetRemainingTraceSize();
    bool IsZipOverFlow();
    void DecreaseDynamicThreshold();
    void StoreTraceSize(int64_t traceSize);
    bool IsIoOverFlow();
    void StoreIoSize(int64_t traceSize);
#ifdef TRACE_MANAGER_UNITTEST
    void SetTestDate(const std::string& testDate)
    {
        testDate_ = testDate;
    }
#endif

private:
    void InitTableRecord();
    void Store(const TraceFlowRecord& traceFlowRecord);
    void Query(TraceFlowRecord& traceFlowRecord);
    void InsertTable(const TraceFlowRecord& traceFlowRecord);
    void QueryTable(TraceFlowRecord& traceFlowRecord);
    void UpdateTable(const TraceFlowRecord& traceFlowRecord);
    void InitTraceQuota();
    bool IsDateChange();
    std::string GetDate();

private:
    TraceFlowRecord traceFlowRecord_;
    std::string name_;
    std::string traceQuotaConfig_;
    int64_t ioQuota_ = 0;
    int64_t zipQuota_ = 0;
    int64_t decreaseUnit_ = 0;
    std::shared_ptr<NativeRdb::RdbStore> dbStore_;
#ifdef TRACE_MANAGER_UNITTEST
    std::string testDate_;
#endif
}; // TraceStorage
} // namespace OHOS::HiviewDFX

#endif // FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_STORAGE_H
