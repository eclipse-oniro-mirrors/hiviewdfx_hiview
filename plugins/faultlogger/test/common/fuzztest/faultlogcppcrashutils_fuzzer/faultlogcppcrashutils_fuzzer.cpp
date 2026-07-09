/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

#include "faultlog_cppcrash.h"
#include "faultlog_info_inner.h"
#include "faultlogcppcrashutils_fuzzer.h"
#include "fuzz_data_source.h"
#include "json/json.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr size_t MAX_STR_LEN = 200;

bool ReadString(FuzzDataSource& source, std::string& str)
{
    return source.GetString(str, MAX_STR_LEN);
}

void FillFaultLogInfo(FuzzDataSource& source, FaultLogInfo& info)
{
    (void)source.GetValue(info.time);
    (void)source.GetValue(info.pid);
    (void)source.GetValue(info.id);
    (void)source.GetValue(info.faultLogType);
    std::string module;
    if (ReadString(source, module)) {
        info.module = module;
    }
    std::string reason;
    if (ReadString(source, reason)) {
        info.reason = reason;
    }
}
}

void FuzzStaticMethods(FuzzDataSource& source)
{
    std::string hilog;
    (void)ReadString(source, hilog);
    (void)FaultLogCppCrash::GetLastLineHilogTime(hilog);
    std::string content;
    (void)ReadString(source, content);
    (void)FaultLogCppCrash::TruncateLogIfExceedsLimit(content);
    FaultLogInfo info;
    FillFaultLogInfo(source, info);
    (void)FaultLogCppCrash::CheckFaultLog(info);
    bool isJsonFile = false;
    (void)source.GetValue(isJsonFile);
    (void)FaultLogCppCrash::GetCppCrashTempLogName(info, isJsonFile);
    std::string minidumpPath;
    Json::Value hiappeventJson;
    FaultLogCppCrash::FillStackInfo(info, minidumpPath, hiappeventJson);
    (void)FaultLogCppCrash::ReportProcessKillEvent(info);
    (void)FaultLogCppCrash::GetStackInfo(info, hiappeventJson);
    FaultLogCppCrash::CheckHilogTime(info);
    uint32_t timeOutUs = 0;
    (void)source.GetValue(timeOutUs);
    (void)FaultLogCppCrash::GetMinidumpPath(info, timeOutUs);
    (void)FaultLogCppCrash::DealMiniDumpEvent(info);
}

void FuzzFileMethods(FuzzDataSource& source)
{
    std::string logPath;
    (void)ReadString(source, logPath);
    std::string content;
    (void)ReadString(source, content);
    (void)FaultLogCppCrash::ReadLogFile(logPath);
    FaultLogCppCrash::WriteLogFile(logPath, content);
    (void)FaultLogCppCrash::TruncateAppCrashLog(logPath, content);
    FaultLogInfo info;
    FillFaultLogInfo(source, info);
    (void)FaultLogCppCrash::TryOpenJsonFileFd(info);
    FILE* fp = fopen("/dev/null", "r");
    if (fp != nullptr) {
        (void)FaultLogCppCrash::FindTargetOffset(fp, content);
        fclose(fp);
    }
}

void FuzzInstanceMethods(FuzzDataSource& source)
{
    FaultLogCppCrash crash;
    FaultLogInfo info;
    FillFaultLogInfo(source, info);
    crash.AddCppCrashInfo(info);
    (void)crash.ParseCppCrashJson(info);
    crash.ReportCppCrashToAppEvent(info);
    (void)crash.NeedSkip();
    (void)crash.ReportEventToAppEvent();
    std::string logPath;
    (void)ReadString(source, logPath);
    crash.DoFaultLogLimit(logPath);
    crash.UpdateFaultLogInfo();
}

void FuzzCppCrashUtils(const uint8_t* data, size_t size)
{
    FuzzDataSource source(data, size);
    FuzzStaticMethods(source);
    FuzzFileMethods(source);
    FuzzInstanceMethods(source);
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzCppCrashUtils(data, size);
    return 0;
}
