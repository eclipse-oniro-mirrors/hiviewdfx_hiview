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

#ifndef DFR_OOMDUMP_EVENT_HANDLER_H
#define DFR_OOMDUMP_EVENT_HANDLER_H

#include "leak_event_handler_base.h"
namespace OHOS {
namespace HiviewDFX {

class OOMdumpEventHandler : public LeakEventHandlerBase {
public:
    std::shared_ptr<LeakEventInfoBase> HandleMessage(SysEvent sysEvent) override;
    ErrCode DumpMessage(std::shared_ptr<LeakEventInfoBase> eventInfo) override;
    ErrCode ReportMessage(std::shared_ptr<LeakEventInfoBase> eventInfo) override;
    ErrCode ReportSubEvent(std::shared_ptr<LeakEventInfoBase> eventInfo);
};

class OomLeakInfo : public LeakEventInfoBase {
public:
    explicit OomLeakInfo(const std::string &eventName);
    void SetPid(pid_t pid);
    pid_t GetPid();
    void SetTid(pid_t tid);
    pid_t GetTid();
    void SetUid(pid_t uid);
    void SetAppRunningId(std::string id);
    std::string GetAppRunningId();
    void SetProcessName(std::string processName);
    std::string GetProcessName();
    bool IsApp();
    void SetLeakedTime();
    std::string GetLeakedTime() const;
    std::string GetHapVersion();
    std::string GetBundleName();
    void SetHeapType(std::string heapType);
    std::string GetHeapType() const;
    void SetThreadType(std::string threadType);
    std::string GetThreadType() const;
    void SetSpaceType(std::string spaceType);
    std::string GetSpaceType() const;
    void SetLastAllocateObjectSize(int lastAllocateObjectSize);
    int GetLastAllocateObjectSize();
    void SetArktsType(std::string arktsType);
    std::string GetArktsType() const;

private:
    pid_t pid_ { 0 };
    pid_t tid_ { 0 };
    pid_t uid_ { 0 };
    int lastAllocateObjectSize_ { 0 };
    std::string appRunningId_ { "" };
    std::string processName_ { "" };
    std::string leakedTime_ { "" };
    std::string bundleVersion_ { "" };
    std::string bundleName_ { "" };
    std::string heapType_ { "" };
    std::string threadType_ { "" };
    std::string spaceType_ { "" };
    std::string arktsType_ { "" };
};
} // HiviewDFX
} // OHOS
#endif // DFR_OOMDUMP_EVENT_HANDLER_H
