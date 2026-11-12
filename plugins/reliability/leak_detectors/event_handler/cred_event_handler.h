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

#ifndef DFR_CRED_EVENT_HANDLER_H
#define DFR_CRED_EVENT_HANDLER_H

#include "leak_event_handler_base.h"
#include <atomic>
namespace OHOS {
namespace HiviewDFX {

class CredEventHandler : public LeakEventHandlerBase {
public:
    std::shared_ptr<LeakEventInfoBase> HandleMessage(SysEvent sysEvent) override;
    ErrCode DumpMessage(std::shared_ptr<LeakEventInfoBase> eventInfo) override;
    ErrCode ReportMessage(std::shared_ptr<LeakEventInfoBase> eventInfo) override;
};

class CredLeakInfo : public LeakEventInfoBase {
public:
    explicit CredLeakInfo(const std::string &eventName);
    std::vector<int> ConvertStringToIntArray(std::string memOrderListStr);
    bool PrintMemList(std::ofstream &fout);

    void SetProcessCount(int processCount);
    int GetProcessCount();
    void SetThreadsCount(int threadsCount);
    int GetThreadsCount();
    void SetMemTotal(unsigned int memTotal);
    unsigned int GetMemTotal();
    void SetMemUsed(unsigned int memUsed);
    unsigned int GetMemUsed();
    void SetMemOrderList(const std::string &memOrderList);
    std::string GetMemOrderList();
    bool IsEverReported();
    void RecordReport();
    time_t GetLastReportRecord();

private:
    int processCount_{ 0 };
    int threadsCount_{ 0 };
    unsigned int memTotal_{ 0 };
    unsigned int memUsed_{ 0 };
    std::string memOrderList_;
    static std::atomic<time_t> lastLogStamp_;
};

} // HiviewDFX
} // OHOS
#endif // DFR_CRED_EVENT_HANDLER_H
