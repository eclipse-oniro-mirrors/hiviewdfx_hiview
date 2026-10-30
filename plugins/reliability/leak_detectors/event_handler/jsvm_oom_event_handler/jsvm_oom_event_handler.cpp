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

#include "jsvm_oom_event_handler.h"

#include <string>
#include "hisysevent.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
using namespace std;

DEFINE_LOG_LABEL(0xD002DA0, "JsvmOomEventHandler");

JsvmOomLeakInfo::JsvmOomLeakInfo(const std::string &eventName) : OomLeakInfoBase(eventName)
{
}

void JsvmOomLeakInfo::SetDumpJsonHeap(const std::string &heap)
{
    dumpJsonHeap_ = heap;
}

std::string JsvmOomLeakInfo::GetDumpJsonHeap()
{
    return dumpJsonHeap_;
}

std::string JsvmOomLeakInfo::GetFirstFrame()
{
    return "JSVM_OOM";
}

std::shared_ptr<LeakEventInfoBase> JsvmOomEventHandler::HandleMessage(SysEvent sysEvent)
{
    int32_t pid = sysEvent.GetEventIntValue("pid_");
    int64_t happenTime = sysEvent.GetEventIntValue("time_");
    std::string processName = sysEvent.GetEventValue("PROCESS_NAME");
    std::string appRunningId = sysEvent.GetEventValue("APP_RUNNING_ID");
    std::string dumpJsonHeap = sysEvent.GetEventValue("DUMPJSONHEAP");
    std::string appVer = sysEvent.GetEventValue("APP_VER");

    shared_ptr<LeakEventInfoBase> eventInfo = make_shared<JsvmOomLeakInfo>("JSVM_OOM");
    auto jsvmOomInfo = static_pointer_cast<JsvmOomLeakInfo>(eventInfo);
    jsvmOomInfo->SetLeakedTime();
    jsvmOomInfo->SetPid(pid);
    jsvmOomInfo->SetTime(to_string(happenTime));
    jsvmOomInfo->SetProcessName(processName);
    jsvmOomInfo->SetAppRunningId(appRunningId);
    jsvmOomInfo->SetDumpJsonHeap(dumpJsonHeap);
    jsvmOomInfo->SetAppVersion(appVer);
    return eventInfo;
}

ErrCode JsvmOomEventHandler::DumpMessage(std::shared_ptr<LeakEventInfoBase> eventInfo)
{
    return SUCCESSED;
}

ErrCode JsvmOomEventHandler::ReportMessage(std::shared_ptr<LeakEventInfoBase> eventInfo)
{
    auto jsvmOomInfo = static_pointer_cast<JsvmOomLeakInfo>(eventInfo);
    std::string summaryInfo = "JSVM_OOM PROCESS:" + jsvmOomInfo->GetProcessName() +
                              " DUMPJSONHEAP:" + jsvmOomInfo->GetDumpJsonHeap();
    int32_t ret = HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, "MEMORY_LEAK", HiSysEvent::EventType::FAULT,
        "PID", jsvmOomInfo->GetPid(),
        "TIME", jsvmOomInfo->GetTime(),
        "APP_RUNNING_ID", jsvmOomInfo->GetAppRunningId(),
        "HAPPEN_TIME", jsvmOomInfo->GetLeakedTime(),
        "MODULE", jsvmOomInfo->GetProcessName(),
        "FIRST_FRAME", jsvmOomInfo->GetFirstFrame(),
        "SECOND_FRAME", "OVER_LIMIT",
        "FINGERPRINT", DfrUtil::GetInstance().CalcFingerPrint(jsvmOomInfo->GetProcessName() +
            jsvmOomInfo->GetFirstFrame() + jsvmOomInfo->GetAppVersion(), 0),
        "SUMMARY", summaryInfo);
    if (ret < 0) {
        HIVIEW_LOGW("HiSysEventWrite failed, ret=%d", ret);
    }
    return SUCCESSED;
}

} // HiviewDFX
} // OHOS
