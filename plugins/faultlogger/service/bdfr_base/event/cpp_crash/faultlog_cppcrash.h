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

#include "faultlog_info_inner.h"
#include "faultlog_event_ipc.h"
#include "json/json.h"

namespace OHOS {
namespace HiviewDFX {
class FaultLogCppCrash final : public FaultLogEventIpc {
private:
    bool NeedSkip() const override;
    bool ReportEventToAppEvent() const override;
    void DoFaultLogLimit(const std::string& logPath) const override;
    void UpdateFaultLogInfo() override;
    static Json::Value FillStackInfo(const FaultLogInfo& info, std::string& stackInfoOriginal);
    static bool CheckFaultLog(const FaultLogInfo& info);
    static bool ReportProcessKillEvent(const FaultLogInfo& info);
    static bool TruncateLogIfExceedsLimit(std::string& readContent);
    static int64_t GetLastLineHilogTime(const std::string& lastLineHilog);
    static std::string GetStackInfo(const FaultLogInfo& info);
    static std::string ReadLogFile(const std::string& logPath);
    static std::string ReadStackFromPipe(const FaultLogInfo& info);
    static void AddCppCrashInfo(FaultLogInfo& info);
    static void CheckHilogTime(FaultLogInfo& info);
    static void ReportCppCrashToAppEvent(const FaultLogInfo& info);
    static void WriteLogFile(const std::string& logPath, const std::string& content);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif
