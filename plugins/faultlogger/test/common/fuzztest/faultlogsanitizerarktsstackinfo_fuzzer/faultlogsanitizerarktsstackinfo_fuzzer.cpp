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
#include <fstream>
#include <string>

#include "faultlog_sanitizer.h"
#include "faultlogsanitizerarktsstackinfo_fuzzer.h"
#include "fuzz_data_source.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr size_t MAX_STR_LEN = 500;

bool ReadString(FuzzDataSource& source, std::string& str)
{
    return source.GetString(str, MAX_STR_LEN);
}

bool WriteFuzzContentToTempFile(const std::string& tmpPath, const std::string& content)
{
    std::ofstream ofs(tmpPath);
    if (!ofs.is_open()) {
        return false;
    }
    ofs << content;
    ofs.close();
    return true;
}

void CleanupTempFile(const std::string& tmpPath)
{
    std::remove(tmpPath.c_str());
    std::remove((tmpPath + ".tmp").c_str());
}

void FuzzParserArkTsStackInfo(FuzzDataSource& source)
{
    std::string moduleName;
    if (!ReadString(source, moduleName)) {
        return;
    }
    std::string fileContent;
    if (!ReadString(source, fileContent)) {
        return;
    }
    std::string tmpPath = "/tmp/fuzz_sanitizer_parser_arkts";
    if (!WriteFuzzContentToTempFile(tmpPath, fileContent)) {
        return;
    }
    FaultLogSanitizer sanitizer;
    (void)sanitizer.ParserArkTsStackInfo(moduleName, tmpPath);
    CleanupTempFile(tmpPath);
}

void FuzzForkProcessParseArkTsStackInfo(FuzzDataSource& source)
{
    std::string moduleName;
    if (!ReadString(source, moduleName)) {
        return;
    }
    std::string fileContent;
    if (!ReadString(source, fileContent)) {
        return;
    }
    std::string tmpPath = "/tmp/fuzz_sanitizer_fork_arkts";
    if (!WriteFuzzContentToTempFile(tmpPath, fileContent)) {
        return;
    }
    FaultLogSanitizer sanitizer;
    (void)sanitizer.ForkProcessParseArkTsStackInfo(moduleName, tmpPath);
    CleanupTempFile(tmpPath);
}
}

void FuzzArkTsStackInfo(const uint8_t* data, size_t size)
{
    FuzzDataSource source(data, size);
    FuzzParserArkTsStackInfo(source);
    FuzzForkProcessParseArkTsStackInfo(source);
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzArkTsStackInfo(data, size);
    return 0;
}
