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
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "constants.h"
#include "faultlog_sanitizer.h"
#include "faultlog_info_inner.h"
#include "faultlogsanitizerutils_fuzzer.h"
#include "fuzz_data_source.h"
#include "sys_event.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr size_t MAX_STR_LEN = 200;
constexpr int32_t MAX_MAP_COUNT = 5;

bool ReadString(FuzzDataSource& source, std::string& str)
{
    return source.GetString(str, MAX_STR_LEN);
}

std::vector<MapInfo> ReadMaps(FuzzDataSource& source)
{
    std::vector<MapInfo> maps;
    int32_t count = 0;
    if (!source.GetValue(count)) {
        return maps;
    }
    count = count % (MAX_MAP_COUNT + 1);
    for (int32_t i = 0; i < count; ++i) {
        MapInfo mi{0, 0, ""};
        if (!source.GetValue(mi.start) || !source.GetValue(mi.end)) {
            break;
        }
        std::string fileName;
        if (!ReadString(source, fileName)) {
            break;
        }
        mi.fileName = fileName;
        maps.push_back(mi);
    }
    return maps;
}
}

void FuzzProcessArkTsLine(FuzzDataSource& source)
{
    std::string line;
    if (!ReadString(source, line)) {
        return;
    }
    std::string packageName;
    if (!ReadString(source, packageName)) {
        return;
    }
    auto maps = ReadMaps(source);
    FaultLogSanitizer sanitizer;
    (void)sanitizer.ProcessArkTsLine(line, packageName, maps);
}

void FuzzLoadMaps(FuzzDataSource& source)
{
    std::string fileContent;
    if (!ReadString(source, fileContent)) {
        return;
    }
    std::string tmpPath = "/tmp/fuzz_sanitizer_maps";
    std::ofstream ofs(tmpPath);
    if (!ofs.is_open()) {
        return;
    }
    ofs << fileContent;
    ofs.close();
    std::ifstream ifs(tmpPath);
    if (!ifs.is_open()) {
        return;
    }
    FaultLogSanitizer sanitizer;
    (void)sanitizer.LoadMaps(ifs);
    ifs.close();
    std::remove(tmpPath.c_str());
}

void FuzzReportAndSysEvent(FuzzDataSource& source)
{
    int32_t pid = 0;
    int32_t uid = 0;
    (void)source.GetValue(pid);
    (void)source.GetValue(uid);
    std::string reason;
    (void)ReadString(source, reason);
    std::string moduleName;
    (void)ReadString(source, moduleName);
    SysEventCreator creator("DOMAIN", "EVENT", SysEventCreator::FAULT);
    creator.SetKeyValue("name_", "ADDRSAN");
    creator.SetKeyValue("pid_", pid);
    creator.SetKeyValue("uid_", uid);
    creator.SetKeyValue(FaultKey::REASON, reason);
    creator.SetKeyValue(FaultKey::MODULE_NAME, moduleName);
    auto sysEvent = std::make_shared<SysEvent>("desc", nullptr, creator);
    FaultLogSanitizer::ReportSanitizerToAppEvent(sysEvent);
    FaultLogSanitizer sanitizer;
    sanitizer.info_.reason = reason;
    sanitizer.info_.logPath = "/data/test/fuzz_log";
    (void)sanitizer.ReportToAppEvent(sysEvent);
    sanitizer.UpdateSysEvent(*sysEvent);
}

void FuzzSanitizerUtils(const uint8_t* data, size_t size)
{
    FuzzDataSource source(data, size);
    FuzzProcessArkTsLine(source);
    FuzzLoadMaps(source);
    FuzzReportAndSysEvent(source);
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzSanitizerUtils(data, size);
    return 0;
}
