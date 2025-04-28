/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef FAULTLOG_PROCESSOR_BASE_H
#define FAULTLOG_PROCESSOR_BASE_H

#include "faultlog_hilog_helper.h"
#include "faultlog_manager.h"
#include "event_publish.h"
#include "faultlog_processor_interface.h"

namespace OHOS {
namespace HiviewDFX {
using namespace FaultlogHilogHelper;
class FaultLogProcessorBase : public FaultLogProcessorInterface {
public:
    ~FaultLogProcessorBase() override = default;
    static void GetProcMemInfo(FaultLogInfo& info);
    static std::list<std::string> GetDigtStrArr(const std::string &target);
    void AddFaultLog(FaultLogInfo& info, const std::shared_ptr<EventLoop>& workLoop,
        const std::shared_ptr<FaultLogManager>& faultLogManager) override;
protected:
    std::string ReadLogFile(const std::string& logPath) const;
    void WriteLogFile(const std::string& logPath, const std::string& content) const;
private:
    virtual void AddSpecificInfo(FaultLogInfo& info) = 0;
    virtual void DoFaultLogLimit(const std::string& logPath, int32_t faultType) const {}
    void ReportEventToAppEvent(const FaultLogInfo& info) override {}
    void ProcessFaultLog(FaultLogInfo& info);
    void AddCommonInfo(FaultLogInfo& info);
    void DoFaultLogLimit(const FaultLogInfo& info);
    void SaveFaultInfoToRawDb(FaultLogInfo& info);
    void SaveFaultLogToFile(FaultLogInfo& info);
    bool VerifyModule(FaultLogInfo& info);
    void AddBundleInfo(FaultLogInfo& info);
    void AddForegroundInfo(FaultLogInfo& info);
    void AddHilog(FaultLogInfo& info);
    void UpdateTerminalThreadStack(FaultLogInfo& info);
    void PrintFaultLogInfo(const FaultLogInfo& info);
protected:
    std::shared_ptr<EventLoop> workLoop_;
    std::shared_ptr<FaultLogManager> faultLogManager_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif
