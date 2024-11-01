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

#ifndef FREEZE_VENDOR_H
#define FREEZE_VENDOR_H

#include <set>
#include <string>
#include <vector>

#include "faultlog_info.h"
#include "freeze_common.h"
#include "log_store_ex.h"
#include "power_mgr_client.h"
#include "smart_parser.h"
#include "watch_point.h"

namespace OHOS {
namespace HiviewDFX {
class Vendor {
public:
    explicit Vendor(std::shared_ptr<FreezeCommon> fc) : freezeCommon_(fc) {};
    ~Vendor() {};
    Vendor& operator=(const Vendor&) = delete;
    Vendor(const Vendor&) = delete;

    bool Init();
    std::string GetTimeString(unsigned long long timestamp) const;
    void DumpEventInfo(std::ostringstream& oss, const std::string& header, const WatchPoint& watchPoint) const;
    void InitLogInfo(const WatchPoint& watchPoint, std::string& type, std::string& retPath,
        std::string& tmpLogPath, std::string& tmpLogName) const;
    void InitLogBody(const std::vector<WatchPoint>& list, std::ostringstream& body,
        bool& isFileExists) const;
    std::string MergeEventLog(
        const WatchPoint &watchPoint, const std::vector<WatchPoint>& list,
        const std::vector<FreezeResult>& result) const;
    bool ReduceRelevanceEvents(std::list<WatchPoint>& list, const FreezeResult& result) const;

private:
    std::string SendFaultLog(const WatchPoint &watchPoint, const std::string& logPath, const std::string& type) const;
    void MergeFreezeJsonFile(const WatchPoint &watchPoint, const std::vector<WatchPoint>& list) const;
    static std::string GetDisPlayPowerInfo();
    static std::string GetPowerStateString(OHOS::PowerMgr::PowerState state);
    static std::string IsScbProName(std::string& processName);

    std::unique_ptr<LogStoreEx> logStore_ = nullptr;
    std::shared_ptr<FreezeCommon> freezeCommon_ = nullptr;
};
}  // namespace HiviewDFX
}  // namespace OHOS
#endif // FREEZE_VENDOR_H
