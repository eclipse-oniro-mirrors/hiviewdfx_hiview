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

#include "third_party_framework_event_handler.h"

#include <fcntl.h>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>
#include "fault_info_base.h"
#include "hisysevent.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
using namespace std;

DEFINE_LOG_LABEL(0xD002DA0, "ThirdPartyFrameworkEventHandler");

std::unordered_map<std::string, time_t> ThirdPartyFrameworkLeakInfo::lastLogStampMap_;
std::mutex ThirdPartyFrameworkLeakInfo::lastLogStampMutex_;

ThirdPartyFrameworkLeakInfo::ThirdPartyFrameworkLeakInfo(const std::string &eventName)
    : OomLeakInfoBase(eventName)
{
}

bool ThirdPartyFrameworkLeakInfo::IsFrameworkVersionValid()
{
    return std::regex_match(frameworkVersion_, std::regex(R"(^\d+\.\d+\.\d+(\-.*)?$)"));
}

std::string ThirdPartyFrameworkLeakInfo::GetFirstFrame()
{
    if (frameworkType_ == "OH_KMP_KOTLIN") {
        return "OH_KMP_KOTLIN_LEAK";
    }
    if (frameworkType_ == "OH_REACT_NATIVE_HERMES") {
        return "OH_REACT_NATIVE_HERMES_LEAK";
    }
    if (frameworkType_ == "OH_FLUTTER_DART") {
        return "OH_FLUTTER_DART_LEAK";
    }
    return "UNKNOWN_THIRD_PARTY_FRAMEWORK_LEAK";
}

bool ThirdPartyFrameworkLeakInfo::IsEverReported(const std::string &processName)
{
    time_t lastLogStamp = GetLastReportRecord(processName);
    time_t now = time(nullptr);
    if (lastLogStamp <= 0) {
        return false;
    }
    if (now <= lastLogStamp) {
        return true;
    }
    return (now - lastLogStamp) <= static_cast<time_t>(SEC_PER_DAY);
}

void ThirdPartyFrameworkLeakInfo::RecordReport(const std::string &processName)
{
    std::lock_guard<std::mutex> lock(lastLogStampMutex_);
    lastLogStampMap_[processName] = time(nullptr);
}

time_t ThirdPartyFrameworkLeakInfo::GetLastReportRecord(const std::string &processName)
{
    std::lock_guard<std::mutex> lock(lastLogStampMutex_);
    auto it = lastLogStampMap_.find(processName);
    if (it == lastLogStampMap_.end()) {
        return 0;
    }
    return it->second;
}

void ThirdPartyFrameworkLeakInfo::SetFrameworkType(const std::string &type)
{
    frameworkType_ = type;
}

std::string ThirdPartyFrameworkLeakInfo::GetFrameworkType()
{
    return frameworkType_;
}

void ThirdPartyFrameworkLeakInfo::SetFrameworkVersion(const std::string &ver)
{
    frameworkVersion_ = ver;
}

std::string ThirdPartyFrameworkLeakInfo::GetFrameworkVersion()
{
    return frameworkVersion_;
}

std::shared_ptr<LeakEventInfoBase> ThirdPartyFrameworkEventHandler::HandleMessage(SysEvent sysEvent)
{
    int32_t pid = sysEvent.GetEventIntValue("pid_");
    std::string processName = sysEvent.GetEventValue("PROCESS_NAME");
    int64_t happenTime = sysEvent.GetEventIntValue("HAPPEN_TIME");
    std::string fwType = sysEvent.GetEventValue("FW_TYPE");
    std::string fwVer = sysEvent.GetEventValue("FW_VER");
    std::string appVer = sysEvent.GetEventValue("APP_VER");
    std::string runningId = sysEvent.GetEventValue("RUNNING_ID");
    shared_ptr<LeakEventInfoBase> eventInfo = make_shared<ThirdPartyFrameworkLeakInfo>("FW_MEM_ANOMALY");
    auto fwLeakInfo = static_pointer_cast<ThirdPartyFrameworkLeakInfo>(eventInfo);
    fwLeakInfo->SetFrameworkType(fwType);
    fwLeakInfo->SetFrameworkVersion(fwVer);
    if (!fwLeakInfo->IsFrameworkVersionValid()) {
        HIVIEW_LOGE("Invalid framework version: %{public}s", fwVer.c_str());
        return nullptr;
    }
    if (fwLeakInfo->IsEverReported(processName)) {
        HIVIEW_LOGE("Event: %{public}s has Reported at %{public}ld", fwLeakInfo->GetEventName().c_str(),
            fwLeakInfo->GetLastReportRecord(processName));
        return nullptr;
    }
    fwLeakInfo->SetLeakedTime();
    fwLeakInfo->SetPid(pid);
    fwLeakInfo->SetProcessName(processName);
    fwLeakInfo->SetAppVersion(appVer);
    fwLeakInfo->SetAppRunningId(runningId);
    return eventInfo;
}

ErrCode ThirdPartyFrameworkEventHandler::DumpMessage(std::shared_ptr<LeakEventInfoBase> eventInfo)
{
    return SUCCESSED;
}

ErrCode ThirdPartyFrameworkEventHandler::ReportMessage(std::shared_ptr<LeakEventInfoBase> eventInfo)
{
    auto fwLeakInfo = static_pointer_cast<ThirdPartyFrameworkLeakInfo>(eventInfo);
    if (fwLeakInfo == nullptr) {
        HIVIEW_LOGE("ReportMessage fwLeakInfo is null");
        return FAILURE;
    }
    std::string summaryInfo = "THIRD_PARTY_FW_LEAK FW_TYPE: " + fwLeakInfo->GetFrameworkType() +
                             ", FW_VER: " + fwLeakInfo->GetFrameworkVersion() +
                             ", APP_VER: " + fwLeakInfo->GetAppVersion() +
                             ", PROCESS: " + fwLeakInfo->GetProcessName() +
                             ", PID: " + std::to_string(fwLeakInfo->GetPid()) +
                             ", RUNNING_ID: " + fwLeakInfo->GetAppRunningId();
    HiSysEventWrite(
        HiSysEvent::Domain::RELIABILITY, "MEMORY_LEAK", HiSysEvent::EventType::FAULT, "MODULE",
        fwLeakInfo->GetProcessName(), "HAPPEN_TIME", fwLeakInfo->GetLeakedTime(), "FIRST_FRAME",
        fwLeakInfo->GetFirstFrame(), "SECOND_FRAME", "WARNING", "FINGERPRINT",
        DfrUtil::GetInstance().CalcFingerPrint(fwLeakInfo->GetProcessName() + fwLeakInfo->GetFrameworkType(), 0),
        "SUMMARY", summaryInfo);
    fwLeakInfo->RecordReport(fwLeakInfo->GetProcessName());
    return SUCCESSED;
}

} // HiviewDFX
} // OHOS
