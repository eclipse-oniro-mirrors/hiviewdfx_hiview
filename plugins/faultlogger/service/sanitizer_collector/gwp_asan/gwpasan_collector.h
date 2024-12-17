/*
 * Copyright (c) 2023 Huawei Device Co., Ltd. All rights reserved.
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
#ifndef GWPASAN_COLLECTOR_H
#define GWPASAN_COLLECTOR_H

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include <regex>

#ifdef __cplusplus
extern "C" {
#endif
void WriteGwpAsanLog(char* buf, size_t sz);
#ifdef __cplusplus
}
#endif

struct GwpAsanCurrInfo {
    /** the time of happening fault */
    uint64_t happenTime;
    /** the id of current user when fault happened  */
    int32_t uid;
    /** the id of process which fault happened*/
    int32_t pid;
    /** type of fault */
    std::string procName;
    /** name of module which fault occurred */
    std::string logPath;
    /** logPath of module which fault occurred */
    std::string appVersion;
    /** the reason why fault occurred */
    std::string errType;
    /** the summary of fault information */
    std::string description;
    /** information about faultlog using <key,value> */
    std::map<std::string, std::string> sectionMaps;
};

void ReadGwpAsanRecord(const std::string& gwpAsanBuffer, const std::string& errType);
std::string GetNameByPid(int32_t pid);
std::string CalcCollectedLogName(const GwpAsanCurrInfo &currInfo);
std::string GetErrorTypeFromHwAsanLog(const std::string& hwAsanBuffer);
std::string GetErrorTypeFromAsanLog(const std::string& asanBuffer);
std::string GetErrorTypeFromLog(const std::string& logBuffer, const std::regex& recordRe,
    int errTypeField, const std::string& defaultType);

#endif // GWPASAN_COLLECTOR_H