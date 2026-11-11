/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef DFR_RS_FD_WARNING_EVENT_HANDLER_H
#define DFR_RS_FD_WARNING_EVENT_HANDLER_H

#include "leak_event_handler_base.h"
#include <atomic>
#include <map>
#include <vector>
#include "fd_leak_info.h"
#include "ffrt.h"

namespace OHOS {
namespace HiviewDFX {

struct RsFdWarningParams {
    pid_t pid { 0 };
    std::string processName;
    int32_t fdThreshold { 0 };
    int32_t fdNum { 0 };
    std::string appRunningId;
};

class RsFdWarningInfo : public LeakEventInfoBase {
public:
    RsFdWarningInfo(const std::string &eventName, const RsFdWarningParams &params);
    ~RsFdWarningInfo() override = default;

    std::string GetFilePath() override;
    std::string GetFileName() override;

    pid_t GetPid();
    std::string GetProcessName();
    int32_t GetFdThreshold();
    int32_t GetFdNum();
    std::string GetAppRunningId();

    bool IsEverReported();
    void RecordReport();

    std::shared_ptr<FdLeakInfo> GetFdLeakInfo() { return fdLeakInfo_; }

private:
    RsFdWarningParams params_;
    std::shared_ptr<FdLeakInfo> fdLeakInfo_;

    static ffrt::mutex reportedMtx_;
    static std::map<std::string, time_t> reportedMap_;
};

class RsFdWarningEventHandler : public LeakEventHandlerBase {
public:
    std::shared_ptr<LeakEventInfoBase> HandleMessage(SysEvent sysEvent) override;
    ErrCode DumpMessage(std::shared_ptr<LeakEventInfoBase> eventInfo) override;
    ErrCode ReportMessage(std::shared_ptr<LeakEventInfoBase> eventInfo) override;
};

} // HiviewDFX
} // OHOS
#endif // DFR_RS_FD_WARNING_EVENT_HANDLER_H
