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
#ifndef FAULTLOG_CPPCRASH_H
#define FAULTLOG_CPPCRASH_H

#include <string>

#include "faultlog_info.h"
#include "faultlog_manager.h"
#include "faultlog_processor_base.h"
#include "json/json.h"

namespace OHOS {
namespace HiviewDFX {
class FaultLogCppCrash : public FaultLogProcessorBase {
private:
    void ReportEventToAppEvent(const FaultLogInfo& info) override;
    void AddSpecificInfo(FaultLogInfo& info) override;
    void DoFaultLogLimit(const std::string& logPath, int32_t faultType) const override;
    void ReportCppCrashToAppEvent(const FaultLogInfo& info) const;
    std::string GetStackInfo(const FaultLogInfo& info) const;
    void AddCppCrashInfo(FaultLogInfo& info);
    void CheckFaultLogAsync(const FaultLogInfo& info);
    static bool CheckFaultLog(const FaultLogInfo& info);
    std::string ReadStackFromPipe(const FaultLogInfo& info) const;
    Json::Value FillStackInfo(const FaultLogInfo& info, std::string& stackInfoOriginal) const;
    bool TruncateLogIfExceedsLimit(std::string& readContent) const;
    bool RemoveHiLogSection(std::string& readContent) const;
    void CheckHilogTime(FaultLogInfo& info);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif
