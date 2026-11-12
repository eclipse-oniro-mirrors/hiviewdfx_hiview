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

#include "arkweb_v8_oom_event_handler.h"

#include <string>
#include "hisysevent.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
using namespace std;

DEFINE_LOG_LABEL(0xD002DA0, "ArkWebV8OomEventHandler");

ArkWebV8OomLeakInfo::ArkWebV8OomLeakInfo(const std::string &eventName) : OomLeakInfoBase(eventName)
{
}

std::string ArkWebV8OomLeakInfo::GetFirstFrame()
{
    return "ARKWEB_V8_OOM";
}

std::shared_ptr<LeakEventInfoBase> ArkWebV8OomEventHandler::HandleMessage(SysEvent sysEvent)
{
    std::string type = sysEvent.GetEventValue("TYPE");
    if (type != "V8_JS_MEM_LEAK") {
        HIVIEW_LOGW("type is not V8_JS_MEM_LEAK, type: %{public}s", type.c_str());
        return nullptr;
    }
    int32_t pid = sysEvent.GetEventIntValue("pid_");
    int64_t happenTime = sysEvent.GetEventIntValue("time_");
    std::string bundleName = sysEvent.GetEventValue("BUNDLE_NAME");

    shared_ptr<LeakEventInfoBase> eventInfo = make_shared<ArkWebV8OomLeakInfo>("PAGE_MEM_LEAK");
    auto arkwebV8OomInfo = static_pointer_cast<ArkWebV8OomLeakInfo>(eventInfo);
    arkwebV8OomInfo->SetLeakedTime();
    arkwebV8OomInfo->SetPid(pid);
    arkwebV8OomInfo->SetTime(to_string(happenTime));
    arkwebV8OomInfo->SetBundleName(bundleName);
    return eventInfo;
}

ErrCode ArkWebV8OomEventHandler::DumpMessage(std::shared_ptr<LeakEventInfoBase> eventInfo)
{
    return SUCCESSED;
}

ErrCode ArkWebV8OomEventHandler::ReportMessage(std::shared_ptr<LeakEventInfoBase> eventInfo)
{
    auto arkwebV8OomInfo = static_pointer_cast<ArkWebV8OomLeakInfo>(eventInfo);
    HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, "MEMORY_LEAK", HiSysEvent::EventType::FAULT,
        "PID", arkwebV8OomInfo->GetPid(),
        "HAPPEN_TIME", arkwebV8OomInfo->GetLeakedTime(),
        "MODULE", arkwebV8OomInfo->GetBundleName(),
        "FIRST_FRAME", arkwebV8OomInfo->GetFirstFrame(),
        "SECOND_FRAME", "OVER_LIMIT",
        "FINGERPRINT", DfrUtil::GetInstance().CalcFingerPrint(arkwebV8OomInfo->GetBundleName() +
            arkwebV8OomInfo->GetFirstFrame(), 0));
    return SUCCESSED;
}

} // HiviewDFX
} // OHOS
