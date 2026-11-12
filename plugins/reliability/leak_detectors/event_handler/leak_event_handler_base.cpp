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

#include "leak_event_handler_base.h"

#include <string>
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
using namespace std;
DEFINE_LOG_LABEL(0xD002DA0, "LeakEventHandlerBase");

namespace {
constexpr int BASE_INDEX = 4;
constexpr int BASE_PERCENTAGE = 100;
constexpr int BASE_NUMBER = 2;
}

LeakEventInfoBase::LeakEventInfoBase(const string &eventName) : eventName_(eventName)
{
}

OomLeakInfoBase::OomLeakInfoBase(const string &eventName) : LeakEventInfoBase(eventName)
{
}

// Process Info
void OomLeakInfoBase::SetPid(int32_t pid)
{
    pid_ = pid;
}

int32_t OomLeakInfoBase::GetPid()
{
    return pid_;
}

void OomLeakInfoBase::SetTid(int32_t tid)
{
    tid_ = tid;
}

int32_t OomLeakInfoBase::GetTid()
{
    return tid_;
}

void OomLeakInfoBase::SetProcessName(const string &name)
{
    processName_ = name;
}

string OomLeakInfoBase::GetProcessName()
{
    return processName_;
}

void OomLeakInfoBase::SetBundleName(const string &name)
{
    bundleName_ = name;
}

string OomLeakInfoBase::GetBundleName()
{
    return bundleName_;
}

void OomLeakInfoBase::SetAppRunningId(const string &id)
{
    appRunningId_ = id;
}

string OomLeakInfoBase::GetAppRunningId()
{
    return appRunningId_;
}

void OomLeakInfoBase::SetAppVersion(const string &ver)
{
    appVersion_ = ver;
}

string OomLeakInfoBase::GetAppVersion()
{
    return appVersion_;
}

void OomLeakInfoBase::SetTime(const string &time)
{
    time_ = time;
}

string OomLeakInfoBase::GetTime()
{
    return time_;
}

void OomLeakInfoBase::SetType(const string &type)
{
    type_ = type;
}

string OomLeakInfoBase::GetType()
{
    return type_;
}

string LeakEventInfoBase::GetFilePath()
{
    return string(MEMORY_LEAK_PATH);
}

string LeakEventInfoBase::GetFileName()
{
    return "/memleak-kernel-" + eventName_ + "-0-" + leakedTime_ + ".txt";
}

ErrCode LeakEventHandlerBase::HandleEvent(SysEvent sysEvent)
{
    auto eventInfo = HandleMessage(sysEvent);
    if (eventInfo == nullptr) {
        HIVIEW_LOGE("HandleMessage failed. eventName: %{public}s", sysEvent.eventName_.c_str());
        return FAILURE;
    }

    if (DumpMessage(eventInfo) == FAILURE) {
        HIVIEW_LOGE("Dump Event Message failed. eventName: %{public}s", eventInfo->GetEventName().c_str());
        return FAILURE;
    }

    if (ReportMessage(eventInfo) == FAILURE) {
        HIVIEW_LOGE("Report Event Message failed. eventName: %{public}s", eventInfo->GetEventName().c_str());
        return FAILURE;
    }
    return SUCCESSED;
}
} // HiviewDFX
} // OHOS
