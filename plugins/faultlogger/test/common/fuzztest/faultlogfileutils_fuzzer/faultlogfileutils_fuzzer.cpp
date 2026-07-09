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
#include <fcntl.h>
#include <list>
#include <map>
#include <string>
#include <unistd.h>

#include "faultlog_bootscan.h"
#include "faultlog_database.h"
#include "faultlog_formatter.h"
#include "faultlog_info_inner.h"
#include "faultlog_manager.h"
#include "faultlog_dump.h"
#include "faultlogfileutils_fuzzer.h"
#include "fuzz_data_source.h"
#include "json/json.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr size_t MAX_STR_LEN = 100;

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

void FuzzBootScan(FuzzDataSource& source)
{
    std::string file;
    (void)ReadString(source, file);
    (void)FaultLogBootScan::IsCrashType(file);
    int64_t now = 0;
    (void)source.GetValue(now);
    (void)FaultLogBootScan::IsInValidTime(file, static_cast<time_t>(now));
    (void)FaultLogBootScan::IsCrashTempBigFile(file);
    FaultLogInfo info;
    FillFaultLogInfo(source, info);
    (void)FaultLogBootScan::IsEmptyStack(file, info);
    (void)FaultLogBootScan::IsReported(info);
}

void FuzzDatabase(FuzzDataSource& source)
{
    std::string fileName;
    (void)ReadString(source, fileName);
    (void)FaultLogDatabase::GetAppFreezeExtInfoFromFileName(fileName);
    int32_t pid = 0;
    int32_t uid = 0;
    int32_t faultType = 0;
    (void)source.GetValue(pid);
    (void)source.GetValue(uid);
    (void)source.GetValue(faultType);
    (void)FaultLogDatabase::IsFaultExist(pid, uid, faultType % FaultLogType::MAX_TYPE);
    FaultLogInfo info;
    FillFaultLogInfo(source, info);
    (void)FaultLogDatabase::GetLifeTimeValue(info);
}

void FuzzFormatter(FuzzDataSource& source)
{
    (void)FaultLogger::IsFaultLogLimit();
    FaultLogInfo info;
    FillFaultLogInfo(source, info);
    int32_t fd = open("/dev/null", O_WRONLY);
    if (fd >= 0) {
        FaultLogger::WriteDfxLogToFile(fd);
        int32_t logType = 0;
        (void)source.GetValue(logType);
        std::map<std::string, std::string> sections;
        std::string key;
        std::string value;
        if (ReadString(source, key) && ReadString(source, value)) {
            sections[key] = value;
        }
        FaultLogger::WriteFaultLogToFile(fd, logType % FaultLogType::MAX_TYPE, sections);
        std::string path;
        (void)ReadString(source, path);
        (void)FaultLogger::WriteLogToFile(fd, path, sections);
        FaultLogger::WriteStackTraceFromLog(fd, std::to_string(info.pid), path);
        close(fd);
    }
    std::string path;
    (void)ReadString(source, path);
    FaultLogger::ParseCppCrashFromTextFile(path, info);
    Json::Value root;
    (void)FaultLogger::ParseJsonFromFile(path, root);
}

void FuzzManager(FuzzDataSource& source)
{
    FaultLogManager manager;
    manager.Init();
    std::string name;
    (void)ReadString(source, name);
    std::string content;
    (void)manager.GetFaultLogContent(name, content);
    int64_t timeVal = 0;
    int32_t id = 0;
    int32_t faultType = 0;
    (void)source.GetValue(timeVal);
    (void)source.GetValue(id);
    (void)source.GetValue(faultType);
    std::string module;
    (void)ReadString(source, module);
    (void)manager.CreateTempFaultLogFile(static_cast<time_t>(timeVal), id,
        faultType % FaultLogType::MAX_TYPE, module);
    FaultLogInfo info;
    FillFaultLogInfo(source, info);
    (void)manager.SaveFaultLogToFile(info);
    manager.RemoveOldFile(info);
    (void)FaultLogManager::CreateFaultLogStore();
    (void)FaultLogManager::CreateLogFileFilter(static_cast<time_t>(timeVal), id,
        faultType % FaultLogType::MAX_TYPE, module);
}

void FuzzFileUtils(const uint8_t* data, size_t size)
{
    FuzzDataSource source(data, size);
    FuzzBootScan(source);
    FuzzDatabase(source);
    FuzzFormatter(source);
    FuzzManager(source);
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzFileUtils(data, size);
    return 0;
}
