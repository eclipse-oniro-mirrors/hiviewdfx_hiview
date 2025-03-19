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

struct TeleMetryRecord {
    std::string module;
    uint32_t usedSize = 0;
    uint32_t quota = 0;
    uint32_t threshold = 0;
};

enum class TelemetryRet {
    SUCCESS,
    OVER_FLOW,
    EXIT
};

class TeleMetryStorage {
public:
    explicit TeleMetryStorage(std::shared_ptr<NativeRdb::RdbStore> dbStore) : dbStore_(dbStore) {}
    ~TeleMetryStorage() = default;
    TelemetryRet InitTelemetryControl(const std::string &telemetryId, int64_t &beginTime,
        const std::map<std::string, int64_t> &flowControlQuotas);
    TelemetryRet NeedTelemetryDump(const std::string& module, int64_t traceSize);
    void ClearTelemetryData();

private:
    std::shared_ptr<NativeRdb::RdbStore> dbStore_;

    void InsertNewData(const std::string &telemetryId, int64_t beginTime,
        const std::map<std::string, int64_t> &flowControlQuotas);
    bool QueryTable(const std::string &module, int64_t &usedSize, int64_t &quotaSize);
    void UpdateTable(const std::string &module, int64_t newSize);
};

}
#endif //HIVIEWDFX_HIVIEW_TELEMETRY_STORAGE_H
