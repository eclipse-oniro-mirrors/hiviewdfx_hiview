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

#ifndef HIVIEWDFX_HIVIEW_TELEMETRY_STORAGE_H
#define HIVIEWDFX_HIVIEW_TELEMETRY_STORAGE_H
#include "memory"

#include "rdb_store.h"

namespace OHOS::HiviewDFX {

struct TeleMetryFlowRecord {
    int64_t usedSize = 0;
    int64_t quotaSize = 0;
    int64_t totalUsedSize = 0;
    int64_t totalQuotaSize = 0;
};

enum class TelemetryRet {
    SUCCESS,
    OVER_FLOW,
    EMPTY_DATA,
    FAILED,
};

class TeleMetryStorage {
public:
    explicit TeleMetryStorage(std::shared_ptr<NativeRdb::RdbStore> dbStore) : dbStore_(dbStore) {}
    ~TeleMetryStorage() = default;
    TelemetryRet InitTelemetryQuota(const std::string &telemetryId,
        const std::map<std::string, int64_t> &flowControlQuotas);
    TelemetryRet NeedTelemetryDump(const std::string &module);
    void ClearTelemetryData();
    TelemetryRet QueryRunningTime(const std::string &telemetryId, int64_t &runningTime);
    TelemetryRet UpdateRunningTime(const std::string &telemetryId, int64_t runningTime);
    void TelemetryStore(const std::string &module, int64_t traceSize);

private:
    bool GetFlowRecord(const std::string &module, TeleMetryFlowRecord &flowRecord);
    bool QueryTable(const std::string &module, int64_t &usedSize, int64_t &quotaSize);
    bool UpdateTable(const std::string &module, int64_t newSize);

private:
    std::shared_ptr<NativeRdb::RdbStore> dbStore_;
};

}
#endif //HIVIEWDFX_HIVIEW_TELEMETRY_STORAGE_H
