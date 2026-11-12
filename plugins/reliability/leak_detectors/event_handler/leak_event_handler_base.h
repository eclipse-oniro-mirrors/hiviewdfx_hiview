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

#ifndef DFR_LEAK_EVENT_HANDLER_BASE_H
#define DFR_LEAK_EVENT_HANDLER_BASE_H

#include "sys_event.h"
#include <string>
#include "fault_common.h"
#include "ffrt.h"
#include "dfr_util.h"

namespace OHOS {
namespace HiviewDFX {

class LeakEventInfoBase {
public:
    explicit LeakEventInfoBase(const std::string &eventName);
    virtual ~LeakEventInfoBase() {};
    virtual std::string GetFilePath();
    virtual std::string GetFileName();

    inline std::string GetEventName()
    {
        return eventName_;
    }

    inline std::string GetLeakedTime()
    {
        return leakedTime_;
    }

    inline void SetLeakedTime()
    {
        leakedTime_ = DfrUtil::GetInstance().GetRealTimeStampStr();
    }

protected:
    std::string leakedTime_;
    std::string eventName_;
};

// OOM事件通用信息基类 - 抽取所有子类的公共Get/Set方法
class OomLeakInfoBase : public LeakEventInfoBase {
public:
    explicit OomLeakInfoBase(const std::string &eventName);
    ~OomLeakInfoBase() override = default;

    void SetPid(int32_t pid);
    int32_t GetPid();

    void SetTid(int32_t tid);
    int32_t GetTid();

    void SetProcessName(const std::string &name);
    std::string GetProcessName();

    void SetBundleName(const std::string &name);
    std::string GetBundleName();

    void SetAppRunningId(const std::string &id);
    std::string GetAppRunningId();

    void SetAppVersion(const std::string &ver);
    std::string GetAppVersion();

    void SetTime(const std::string &time);
    std::string GetTime();

    void SetType(const std::string &type);
    std::string GetType();

protected:
    int32_t pid_ { 0 };
    int32_t tid_ { 0 };
    std::string processName_ { "" };
    std::string bundleName_ { "" };
    std::string appRunningId_ { "" };
    std::string appVersion_ { "" };
    std::string time_ { "" };
    std::string type_ { "" };
};

// 事件处理函数基类
class LeakEventHandlerBase {
public:
    LeakEventHandlerBase() {};
    virtual ~LeakEventHandlerBase() {};
    virtual ErrCode HandleEvent(SysEvent sysEvent);
    virtual std::shared_ptr<LeakEventInfoBase> HandleMessage(SysEvent sysEvent) = 0;
    virtual ErrCode DumpMessage(std::shared_ptr<LeakEventInfoBase> eventInfo) = 0;
    virtual ErrCode ReportMessage(std::shared_ptr<LeakEventInfoBase> eventInfo) = 0;
};

} // HiviewDFX
} // OHOS
#endif // DFR_LEAK_EVENT_HANDLER_BASE_H