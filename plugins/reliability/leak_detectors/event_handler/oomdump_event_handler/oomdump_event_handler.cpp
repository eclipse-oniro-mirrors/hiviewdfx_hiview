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

#include "oomdump_event_handler.h"
#include "hisysevent.h"
#include "bundle_mgr_client.h"
#include "fault_common.h"

namespace OHOS {
namespace HiviewDFX {

using namespace std;
DEFINE_LOG_LABEL(0xD002DA0, "OOMdumpEventHandler");

OomLeakInfo::OomLeakInfo(const string &eventName) : LeakEventInfoBase(eventName)
{
}

void OomLeakInfo::SetPid(pid_t pid)
{
    pid_ = pid;
}

pid_t OomLeakInfo::GetPid()
{
    return pid_;
}

void OomLeakInfo::SetTid(pid_t tid)
{
    tid_ = tid;
}

pid_t OomLeakInfo::GetTid()
{
    return tid_;
}

void OomLeakInfo::SetUid(pid_t uid)
{
    uid_ = uid;
}

void OomLeakInfo::SetAppRunningId(std::string id)
{
    appRunningId_ = id;
}

string OomLeakInfo::GetAppRunningId()
{
    return appRunningId_;
}

void OomLeakInfo::SetProcessName(std::string processName)
{
    processName_ = processName;
}

string OomLeakInfo::GetProcessName()
{
    return processName_;
}

void OomLeakInfo::SetHeapType(std::string heapType)
{
    heapType_ = heapType;
}

std::string OomLeakInfo::GetHeapType() const
{
    return heapType_;
}

void OomLeakInfo::SetThreadType(std::string threadType)
{
    threadType_ = threadType;
}

std::string OomLeakInfo::GetThreadType() const
{
    return threadType_;
}

void OomLeakInfo::SetSpaceType(std::string spaceType)
{
    spaceType_ = spaceType;
}

std::string OomLeakInfo::GetSpaceType() const
{
    return spaceType_;
}

void OomLeakInfo::SetLastAllocateObjectSize(int lastAllocateObjectSize)
{
    lastAllocateObjectSize_ = lastAllocateObjectSize;
}

int OomLeakInfo::GetLastAllocateObjectSize()
{
    return lastAllocateObjectSize_;
}

void OomLeakInfo::SetArktsType(std::string arktsType)
{
    arktsType_ = arktsType;
}

std::string OomLeakInfo::GetArktsType() const
{
    return arktsType_;
}

bool OomLeakInfo::IsApp()
{
    return uid_ >= MIN_APP_USERID;
}

string OomLeakInfo::GetHapVersion()
{
    if (!bundleVersion_.empty()) {
        return bundleVersion_;
    }
    if (!IsApp()) {
        return "";
    }
    const string &bundleName = GetBundleName();
    if (bundleName.empty()) {
        return "";
    }
    AppExecFwk::BundleInfo info;
    AppExecFwk::BundleMgrClient client;
    if (!client.GetBundleInfo(bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT,
        info, AppExecFwk::Constants::ALL_USERID)) {
        return "";
    }
    bundleVersion_ = info.versionName;
    return bundleVersion_;
}

string OomLeakInfo::GetBundleName()
{
    if (bundleName_.empty()) {
        bundleName_ = DfrUtil::GetInstance().GetApplicationNameByUid(uid_);
    }
    return bundleName_;
}

string OomLeakInfo::GetLeakedTime() const
{
    return leakedTime_;
}

void OomLeakInfo::SetLeakedTime()
{
    leakedTime_ = DfrUtil::GetInstance().GetRealTimeStampStr();
}

std::shared_ptr<LeakEventInfoBase> OOMdumpEventHandler::HandleMessage(SysEvent sysEvent)
{
    std::string arktsType = sysEvent.GetEventValue("ARKTS_TYPE");
    if (arktsType != "dynamic" && arktsType != "static") {
        HIVIEW_LOGI("Invalid ArktsType: %{public}s", arktsType.c_str());
        return nullptr;
    }
    shared_ptr<LeakEventInfoBase> eventInfo = make_shared<OomLeakInfo>("OOM");
    if (eventInfo == nullptr) {
        return nullptr;
    }
    auto oomInfo = static_pointer_cast<OomLeakInfo>(eventInfo);
    oomInfo->SetPid(sysEvent.GetEventIntValue("PID"));
    oomInfo->SetTid(sysEvent.GetEventIntValue("TID"));
    oomInfo->SetUid(sysEvent.GetEventIntValue("uid_"));
    oomInfo->SetAppRunningId(sysEvent.GetEventValue("APP_RUNNING_UNIQUE_ID"));
    oomInfo->SetProcessName(sysEvent.GetEventValue("PROCESS_NAME"));
    oomInfo->SetLeakedTime();
    oomInfo->SetHeapType(sysEvent.GetEventValue("HEAP_TYPE"));
    oomInfo->SetThreadType(sysEvent.GetEventValue("THREAD_TYPE"));
    oomInfo->SetSpaceType(sysEvent.GetEventValue("SPACE_TYPE"));
    oomInfo->SetLastAllocateObjectSize(sysEvent.GetEventIntValue("LAST_ALLOCATE_OBJECT_SIZE"));
    oomInfo->SetArktsType(arktsType);
    return eventInfo;
}

ErrCode OOMdumpEventHandler::DumpMessage(std::shared_ptr<LeakEventInfoBase> eventInfo)
{
    return SUCCESSED;
}

ErrCode OOMdumpEventHandler::ReportMessage(std::shared_ptr<LeakEventInfoBase> eventInfo)
{
    ReportSubEvent(eventInfo);
    auto oomInfo = static_pointer_cast<OomLeakInfo>(eventInfo);
    const std::string& arktsType = oomInfo->GetArktsType();
    HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, "MEMORY_LEAK", HiSysEvent::EventType::FAULT,
        "PID", oomInfo->GetPid(),
        "TID", oomInfo->GetTid(),
        "HAPPEN_TIME", oomInfo->GetLeakedTime(),
        "MEMTOTAL", "0",
        "MODULE", oomInfo->GetProcessName().empty() ? "/" : oomInfo->GetProcessName(),
        "VERSION", oomInfo->GetHapVersion().empty() ? "/" : oomInfo->GetHapVersion(),
        "FIRST_FRAME", (arktsType == "dynamic") ? "JS_LEAK" : "JS_STAT_LEAK",
        "SECOND_FRAME", "OVER_LIMIT",
        // hask calculate by hap name and version
        "FINGERPRINT", DfrUtil::GetInstance().CalcFingerPrint(oomInfo->GetProcessName() +
            "JS_LEAK" + oomInfo->GetHapVersion() + "OVER_LIMIT", 0),
        "APP_RUNNING_ID", oomInfo->GetAppRunningId(),
        "ARKTS_TYPE", oomInfo->GetArktsType()
    );
    return SUCCESSED;
}

ErrCode OOMdumpEventHandler::ReportSubEvent(std::shared_ptr<LeakEventInfoBase> eventInfo)
{
    auto oomInfo = static_pointer_cast<OomLeakInfo>(eventInfo);
    if (oomInfo == nullptr) {
        HIVIEW_LOGE("ReportSubEvent oomInfo is null");
        return FAILURE;
    }
    HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, "MEMORY_LEAK_JS", HiSysEvent::EventType::FAULT,
        "APP_RUNNING_ID", oomInfo->GetAppRunningId(),
        "MODULE", oomInfo->GetProcessName(),
        "DEVICE_RAM_SIZE", DfrUtil::GetInstance().GetRamSize(),
        "HEAP_TYPE", oomInfo->GetHeapType(),
        "THREAD_TYPE", oomInfo->GetThreadType(),
        "SPACE_TYPE", oomInfo->GetSpaceType(),
        "LAST_ALLOCATE_OBJECT_SIZE", oomInfo->GetLastAllocateObjectSize());
    return SUCCESSED;
};
} // HiviewDFX
} // OHOS