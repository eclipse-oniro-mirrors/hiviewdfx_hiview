/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DFR_THIRD_PARTY_FRAMEWORK_EVENT_HANDLER_H
#define DFR_THIRD_PARTY_FRAMEWORK_EVENT_HANDLER_H

#include "leak_event_handler_base.h"
#include <regex>
#include <unordered_map>
#include <mutex>
#include <vector>

namespace OHOS {
namespace HiviewDFX {

class ThirdPartyFrameworkEventHandler : public LeakEventHandlerBase {
public:
    ThirdPartyFrameworkEventHandler() = default;
    ~ThirdPartyFrameworkEventHandler() override = default;

    std::shared_ptr<LeakEventInfoBase> HandleMessage(SysEvent sysEvent) override;
    ErrCode DumpMessage(std::shared_ptr<LeakEventInfoBase> eventInfo) override;
    ErrCode ReportMessage(std::shared_ptr<LeakEventInfoBase> eventInfo) override;
};

class ThirdPartyFrameworkLeakInfo : public OomLeakInfoBase {
public:
    explicit ThirdPartyFrameworkLeakInfo(const std::string &eventName);
    ~ThirdPartyFrameworkLeakInfo() override = default;

    bool IsFrameworkVersionValid();
    std::string GetFirstFrame();

    bool IsEverReported(const std::string &processName);
    void RecordReport(const std::string &processName);
    time_t GetLastReportRecord(const std::string &processName);

    void SetFrameworkType(const std::string &type);
    std::string GetFrameworkType();

    void SetFrameworkVersion(const std::string &ver);
    std::string GetFrameworkVersion();

private:
    static std::unordered_map<std::string, time_t> lastLogStampMap_;
    static std::mutex lastLogStampMutex_;
    std::string frameworkType_ { "" };
    std::string frameworkVersion_ { "" };
};

} // HiviewDFX
} // OHOS

#endif // DFR_THIRD_PARTY_FRAMEWORK_EVENT_HANDLER_H
