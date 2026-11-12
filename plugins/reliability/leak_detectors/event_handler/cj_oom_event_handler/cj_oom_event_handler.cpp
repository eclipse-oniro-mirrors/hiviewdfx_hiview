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

#include "cj_oom_event_handler.h"

#include <string>
#include "hisysevent.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
using namespace std;

DEFINE_LOG_LABEL(0xD002DA0, "CjOomEventHandler");

CJ_OOMLeakInfo::CJ_OOMLeakInfo(const std::string &eventName) : OomLeakInfoBase(eventName)
{
}

void CJ_OOMLeakInfo::SetAppRunningUniqueId(const std::string &id)
{
    SetAppRunningId(id);
}

std::string CJ_OOMLeakInfo::GetAppRunningUniqueId()
{
    return GetAppRunningId();
}

std::string CJ_OOMLeakInfo::GetFirstFrame()
{
    return "CJ_OOM";
}

std::shared_ptr<LeakEventInfoBase> CjOomEventHandler::HandleMessage(SysEvent sysEvent)
{
    pid_t pid = sysEvent.GetEventIntValue("pid_");
    pid_t tid = sysEvent.GetEventIntValue("tid_");
    std::string processName = sysEvent.GetEventValue("PROCESS_NAME");
    std::string type = sysEvent.GetEventValue("TYPE");
    std::string appRunningUniqueId = sysEvent.GetEventValue("APP_RUNNING_UNIQUE_ID");
    std::string appVer = sysEvent.GetEventValue("APP_VER");

    shared_ptr<LeakEventInfoBase> eventInfo = make_shared<CJ_OOMLeakInfo>("CJ_OOM");
    auto cjOomInfo = static_pointer_cast<CJ_OOMLeakInfo>(eventInfo);
    cjOomInfo->SetLeakedTime();
    cjOomInfo->SetPid(pid);
    cjOomInfo->SetTid(tid);
    cjOomInfo->SetProcessName(processName);
    cjOomInfo->SetType(type);
    cjOomInfo->SetAppRunningUniqueId(appRunningUniqueId);
    cjOomInfo->SetAppVersion(appVer);
    return eventInfo;
}

ErrCode CjOomEventHandler::DumpMessage(std::shared_ptr<LeakEventInfoBase> eventInfo)
{
    return SUCCESSED;
}

ErrCode CjOomEventHandler::ReportMessage(std::shared_ptr<LeakEventInfoBase> eventInfo)
{
    auto cjOomInfo = static_pointer_cast<CJ_OOMLeakInfo>(eventInfo);
    if (cjOomInfo == nullptr) {
        HIVIEW_LOGE("ReportMessage cjOomInfo is null");
        return FAILURE;
    }
    std::string summaryInfo = "CJ_OOM PROCESS:" + cjOomInfo->GetProcessName() +
                              " TYPE:" + cjOomInfo->GetType() +
                              " ID:" + cjOomInfo->GetAppRunningUniqueId();
    HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, "MEMORY_LEAK", HiSysEvent::EventType::FAULT,
        "PID", cjOomInfo->GetPid(),
        "TID", cjOomInfo->GetTid(),
        "HAPPEN_TIME", cjOomInfo->GetLeakedTime(),
        "MODULE", cjOomInfo->GetProcessName(),
        "FIRST_FRAME", cjOomInfo->GetFirstFrame(),
        "SECOND_FRAME", "OVER_LIMIT",
        "FINGERPRINT", DfrUtil::GetInstance().CalcFingerPrint(cjOomInfo->GetProcessName() +
            cjOomInfo->GetFirstFrame() + cjOomInfo->GetType() + cjOomInfo->GetAppVersion(), 0),
        "SUMMARY", summaryInfo);
    return SUCCESSED;
}

} // HiviewDFX
} // OHOS
