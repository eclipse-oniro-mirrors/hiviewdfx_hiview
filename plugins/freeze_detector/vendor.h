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
    void InitLogInfo(const WatchPoint& watchPoint, std::string& type, std::string& pubLogPathName,
        std::string& processName, std::string& isScbPro) const;
    void InitLogBody(const std::vector<WatchPoint>& list, std::ostringstream& body,
        bool& isFileExists) const;
    bool JudgeSysWarningEvent(const std::string& stringId, std::string& type, const std::string& processName,
        const std::vector<WatchPoint>& list, const std::vector<FreezeResult>& result) const;
    std::string MergeEventLog(
        const WatchPoint &watchPoint, const std::vector<WatchPoint>& list,
        const std::vector<FreezeResult>& result) const;
    bool ReduceRelevanceEvents(std::list<WatchPoint>& list, const FreezeResult& result) const;

private:
    static const int MAX_LINE_NUM = 100;
    static const int TIME_STRING_LEN = 16;
    static const int MAX_FILE_NUM = 5;
    static const int MAX_FOLDER_SIZE = 5 * 1024 * 1024;
    static const inline std::string TRIGGER_HEADER = ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>";
    static const inline std::string HEADER = "*******************************************";
    static const inline std::string HYPHEN = "-";
    static const inline std::string NEW_LINE = "\n";
    static const inline std::string EVENT_SUMMARY = "SUMMARY";
    static const inline std::string POSTFIX = ".tmp";
    static const inline std::string APPFREEZE = "appfreeze";
    static const inline std::string SYSFREEZE = "sysfreeze";
    static const inline std::string SP_SYSTEMHUNGFAULT = "SystemHungFault";
    static const inline std::string SP_APPFREEZE = "AppFreeze";
    static const inline std::string SP_ENDSTACK = "END_STACK";
    static const inline std::string FREEZE_DETECTOR_PATH = "/data/log/faultlog/freeze/";
    static const inline std::string FAULT_LOGGER_PATH = "/data/log/faultlog/faultlogger/";
    static const inline std::string SMART_PARSER_PATH = "/system/etc/hiview/";

    std::string SendFaultLog(const WatchPoint &watchPoint, const std::string& logPath, const std::string& type,
        const std::string& processName, const std::string& isScbPro) const;
    void MergeFreezeJsonFile(const WatchPoint &watchPoint, const std::vector<WatchPoint>& list) const;
    static void InitLogFfrt(const WatchPoint &watchPoint, std::ostringstream& ffrt);
    static std::string GetDisPlayPowerInfo();
    static std::string GetPowerStateString(OHOS::PowerMgr::PowerState state);
    static std::string IsScbProName(std::string& processName);

    std::unique_ptr<LogStoreEx> logStore_ = nullptr;
    std::shared_ptr<FreezeCommon> freezeCommon_ = nullptr;
};
}  // namespace HiviewDFX
}  // namespace OHOS
#endif // FREEZE_VENDOR_H
