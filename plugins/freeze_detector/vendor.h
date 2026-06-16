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
#include "power_mgr_client.h"
#include "smart_parser.h"
#include "watch_point.h"
#include "faultlogger_client_msg.h"
#include "db_helper.h"

namespace OHOS {
namespace HiviewDFX {
class Vendor {
public:
    explicit Vendor(std::shared_ptr<FreezeCommon> fc, std::shared_ptr<DBHelper> dBHelper_ = nullptr)
        : freezeCommon_(fc), dBHelper_(dBHelper_) {};
    ~Vendor() {};
    Vendor& operator=(const Vendor&) = delete;
    Vendor(const Vendor&) = delete;

    bool Init();
    std::string GetTimeString(unsigned long long timestamp) const;
    void DumpEventInfo(std::ostringstream& oss, const std::string& header, const WatchPoint& watchPoint) const;
    void InitLogInfo(const WatchPoint& watchPoint, std::string& type, std::string& pubLogPathName,
        std::string& processName, std::string& isScbPro) const;
    void InitLogBody(const std::vector<WatchPoint>& list, std::ostringstream& body,
        bool& isFileExists, WatchPoint &watchPoint, std::string& halfFreezeExtFile) const;
    bool JudgeSysWarningEvent(const std::string& stringId, std::string& type, const std::string& processName,
        const std::vector<WatchPoint>& list, const std::vector<FreezeResult>& result) const;
    void CovertHighLoadToWarning(std::string& type, WatchPoint& watchPoint) const;
    std::string MergeEventLog(WatchPoint &watchPoint, const std::vector<WatchPoint>& list,
        const std::vector<FreezeResult>& result) const;

private:
    std::string SendFaultLog(const WatchPoint &watchPoint, const std::string& logPath, const std::string& type,
        const std::string& processName, const std::string& isScbPro) const;
    void MergeFreezeJsonFile(const WatchPoint &watchPoint, const std::vector<WatchPoint>& list) const;
    std::string MergeFreezeExtFile(const WatchPoint &watchPoint, const std::string& halfFreezeExtFile,
        const std::string& type) const;
    static std::string GetDisPlayPowerInfo();
    static std::string GetPowerStateString(OHOS::PowerMgr::PowerState state);
    static void CheckProcessName(std::string& processName, std::string& isScbPro);
    void CovertFreezeToWarning(std::string& type, const std::vector<WatchPoint>& list,
        const std::string& stringId) const;
    void CovertSysfreezeToAppfreeze(const std::vector<WatchPoint>& list,
        std::string& type, const WatchPoint& watchPoint) const;
    bool GetIfStreamByFilePath(std::string& filePath, std::ifstream& ifs, std::ostringstream& body,
        const WatchPoint& node) const;
    void FillSummaryInfo(FaultLogInfoInner &info, const WatchPoint& watchPoint, const std::string& logPath,
        const std::string& type, const std::string& processName) const;
    void FillSectionMaps(FaultLogInfoInner &info, const WatchPoint& watchPoint, const std::string& isScbPro) const;
    void InitHalfFreezeExtFile(WatchPoint node, const std::string name, std::string& halfFreezeExtFile) const;
    std::string CheckNoteInfo(const WatchPoint& watchPoint) const;

    std::shared_ptr<FreezeCommon> freezeCommon_ = nullptr;
    std::shared_ptr<DBHelper> dBHelper_ = nullptr;
};
}  // namespace HiviewDFX
}  // namespace OHOS
#endif // FREEZE_VENDOR_H
