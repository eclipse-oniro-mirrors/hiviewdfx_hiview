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

#ifndef DFR_PROCESS_KILL_EVENT_HANDLER_H
#define DFR_PROCESS_KILL_EVENT_HANDLER_H

#include "leak_event_handler_base.h"
#include <atomic>
namespace OHOS {
namespace HiviewDFX {

struct ProcesssKillEventParam {
    pid_t pid = 0;
    int uid = 0;
    int adj = 0;
    std::string eventName;
    std::string processName;
    std::string msg;
    std::string innerMsg;
    std::string reason;
    std::string appRunningId;
    int64_t eventParam1 = 0;
    int64_t eventParam2 = 0;
};

class ProcessKillEventHandler : public LeakEventHandlerBase {
public:
    ErrCode HandleEvent(SysEvent sysEvent) override;
    std::shared_ptr<LeakEventInfoBase> HandleMessage(SysEvent sysEvent) override;
    ErrCode DumpMessage(std::shared_ptr<LeakEventInfoBase> eventInfo) override;
    ErrCode ReportMessage(std::shared_ptr<LeakEventInfoBase> eventInfo) override;
    ErrCode AppEventReport(std::shared_ptr<LeakEventInfoBase> eventInfo);
    ErrCode SubEventReport(std::shared_ptr<LeakEventInfoBase> eventInfo);

private:
    void GetThreadInfo(pid_t pid, int64_t &threadCount, std::string &msg);
};

class ProcessKillLeakInfo : public LeakEventInfoBase {
public:
    explicit ProcessKillLeakInfo(const ProcesssKillEventParam &eventParam);

    void SetPid(pid_t pid);
    pid_t GetPid();
    void SetUid(pid_t uid);
    pid_t GetUid();
    void SetProcessName(std::string processName);
    std::string GetProcessName();
    void SetKillReason(std::string killReason);
    std::string GetKillReason();
    void SetDetailMsg(std::string killMsg);
    std::string GetDetailMsg();
    void SetCmdType(uint32_t cmdType);
    uint32_t GetCmdType();
    void SetInfoType(uint32_t infoType);
    uint32_t GetInfoType();
    void SetAppRunningId(std::string appRunningId);
    std::string GetAppRunningId();
    void DumpDetailInfo(std::ofstream &fout, const std::string &detailInfo, const std::string &logType);
    std::string GetApplicationVersion();

    std::string GetFileName() override;
    virtual bool IsDumpMessage();
    virtual void SysEventReport() = 0;
    virtual void AppEventReport() = 0;
    virtual void SubSysEventReport() = 0;
    virtual void DumpGeneralInfo(std::ofstream &fout) = 0;
    virtual void HandleSubeventInfo(const std::string &data) = 0;

protected:
    pid_t pid_ { 0 };
    pid_t uid_ { 0 };
    int adj_ { 0 };
    std::string processName_;
    std::string killReason_;
    std::string detailMsg_;
    std::string appRunningId_;
    std::string version_ { "unknown" };
    uint32_t cmdType_ { CMD_GET_MEM_DETAIL_INFO };
    uint32_t infoType_ { 0 };
};
} // HiviewDFX
} // OHOS
#endif // DFR_PROCESS_KILL_EVENT_HANDLER_H
