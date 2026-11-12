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

#include "gwpasan_collector_utils.h"

#ifdef __cplusplus
extern "C" {
#endif
void WriteSanitizerLog(char* buf, size_t sz, char* path);
#ifdef __cplusplus
}
#endif

constexpr unsigned GWPASAN_MAX_FAULTTYPE_LEN = 32;
constexpr unsigned GWPASAN_MAX_MODULE_LEN = 128;
constexpr unsigned GWPASAN_MAX_LOGPATH_LEN = 256;
constexpr unsigned GWPASAN_MAX_ERRTYPE_LEN = 64;
constexpr unsigned GWPASAN_MAX_TELEMETRY_LEN = 64;
constexpr unsigned GWPASAN_MAX_APPRUNNINGIID_LEN = 64;
constexpr unsigned GWPASAN_MAX_DESCRIPT_LEN = 1 * 1024 * 1024;
struct GwpAsanCurrInfo {
    /** type of sanitizer */
    char faultType[GWPASAN_MAX_FAULTTYPE_LEN];
    /** the time of happening fault */
    uint64_t happenTime;
    /** the id of current user when fault happened  */
    int32_t uid;
    /** the id of process which fault happened*/
    int32_t pid;
    /** type of fault */
    char moduleName[GWPASAN_MAX_MODULE_LEN];
    /** name of module which fault occurred */
    char logPath[GWPASAN_MAX_LOGPATH_LEN];
    /** logPath of module which fault occurred */
    char appVersion[GWPASAN_MAX_MODULE_LEN];
    /** the reason why fault occurred */
    char errType[GWPASAN_MAX_ERRTYPE_LEN];
    /** the detail of fault information */
    char description[GWPASAN_MAX_DESCRIPT_LEN];
    size_t descriptionLen;
    /** telemetryId for gwpasan */
    char telemetryId[GWPASAN_MAX_TELEMETRY_LEN];
    /** information of appRunningId */
    char appRunningId[GWPASAN_MAX_APPRUNNINGIID_LEN];
};

bool WriteDescriptionToFd(int fd, const char* desc, size_t descLen);
bool BuildSandboxFilePath(const GwpAsanCurrInfo& currInfo, char* outPath, size_t outPathSize);
void ReadGwpAsanRecordFromBuffer(const char* faultType, char* logPath);
void WriteCollectedData(const GwpAsanCurrInfo& currInfo, bool& isSendHisysevent);
void WriteToFaultLogger(const GwpAsanCurrInfo& currInfo);
bool WriteToSandbox(const GwpAsanCurrInfo& currInfo);
void SendSanitizerHisysevent(const GwpAsanCurrInfo& currInfo);
bool IsValidSandboxPath(const char* realPath);
#endif // GWPASAN_COLLECTOR_H