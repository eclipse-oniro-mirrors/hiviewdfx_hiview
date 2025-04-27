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
void WriteSanitizerLog(char* buf, size_t sz, char* path);
#ifdef __cplusplus
}
#endif

struct GwpAsanCurrInfo {
    /** type of sanitizer */
    std::string faultType;
    /** the time of happening fault */
    uint64_t happenTime;
    /** the id of current user when fault happened  */
    int32_t uid;
    /** the id of process which fault happened*/
    int32_t pid;
    /** type of fault */
    std::string moduleName;
    /** name of module which fault occurred */
    std::string logPath;
    /** logPath of module which fault occurred */
    std::string appVersion;
    /** the reason why fault occurred */
    std::string errType;
    /** the detail of fault information */
    std::string description;
    /** the summary of fault information */
    std::string summary;
    /** information about faultlog using <key,value> */
    std::map<std::string, std::string> sectionMaps;
    /** hash value used for clustering */
    std::string hash;
    /** top stack */
    std::string topStack;
    /** telemetryId for gwpasan */
    std::string telemetryId;
};

void ReadGwpAsanRecord(const std::string& gwpAsanBuffer, const std::string& faultType, char* logPath);
std::string GetNameByPid(int32_t pid);
std::string GetErrorTypeFromBuffer(const std::string& buffer, const std::string& faultType);
std::string GetTopStackWithoutCommonLib(const std::string& description);
void WriteCollectedData(const GwpAsanCurrInfo& currInfo);
int32_t GetSanitizerFd(const GwpAsanCurrInfo& currInfo);
bool WriteNewFile(const int32_t fd, const GwpAsanCurrInfo& currInfo);
void SendSanitizerHisysevent(const GwpAsanCurrInfo& currInfo);
#endif // GWPASAN_COLLECTOR_H