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

#include <memory>
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
    static void FillStackInfo(const FaultLogInfo& info, std::string& minidumpPath, Json::Value& hiappeventJson);
    static bool CheckFaultLog(const FaultLogInfo& info);
    static bool ReportProcessKillEvent(const FaultLogInfo& info);
    static bool TruncateLogIfExceedsLimit(std::string& readContent);
    static int64_t GetLastLineHilogTime(const std::string& lastLineHilog);
    static std::string GetStackInfo(const FaultLogInfo& info, Json::Value& hiappeventJson);
    static std::string ReadLogFile(const std::string& logPath);
    void AddCppCrashInfo(FaultLogInfo& info);
    static std::string ReadStackFromPipe(const FaultLogInfo& info);
    bool ParseCppCrashJson(FaultLogInfo& info);
    static bool TryOpenJsonFileFd(FaultLogInfo& info);
    static void CheckHilogTime(FaultLogInfo& info);
    void ReportCppCrashToAppEvent(const FaultLogInfo& info) const;
    static void WriteLogFile(const std::string& logPath, const std::string& content);
    static int TruncateAppCrashLog(const std::string& logPath, const std::string& target);
    static long FindTargetOffset(FILE* fp, const std::string& target);
    static std::string GetMinidumpPath(const FaultLogInfo& info, uint32_t timeOutUs);
    static std::string DealMiniDumpEvent(const FaultLogInfo& info);
    static std::string GetCppCrashTempLogName(const FaultLogInfo& info, bool isJsonFile);

    std::shared_ptr<Json::Value> hiappeventJson_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif
