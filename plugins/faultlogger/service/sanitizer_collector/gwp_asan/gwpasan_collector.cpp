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
#include "gwpasan_collector.h"

#include <cstdint>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <mutex>
#include <securec.h>
#include <sys/time.h>
#include <time_util.h>
#include <unistd.h>
#include <vector>
#include <regex>

#include "bundle_mgr_client.h"
#include "event_publish.h"
#include "faultlog_util.h"
#include "file_util.h"
#include "hisysevent.h"
#include "json/json.h"
#include "hilog/log.h"
#include "hiview_logger.h"
#include "parameter_ex.h"

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002D12

#undef LOG_TAG
#define LOG_TAG "Sanitizer"

namespace {
constexpr unsigned ASAN_LOG_SIZE = 350 * 1024;
constexpr unsigned BUF_SIZE = 128;
constexpr unsigned ERRTYPE_FIELD = 4;
static std::stringstream g_asanlog;
}

void WriteGwpAsanLog(char* buf, size_t sz)
{
    if (buf == nullptr || sz == 0) {
        return;
    }
    static std::mutex sMutex;
    std::lock_guard<std::mutex> lock(sMutex);
    // append to buffer
    for (size_t i = 0; i < sz; i++) {
        g_asanlog << buf[i];
    }
    char *gwpOutput = strstr(buf, "End GWP-ASan report");
    char *tsanOutput = strstr(buf, "End Tsan report");
    char *cfiOutput = strstr(buf, "End CFI report");
    char *ubsanOutput = strstr(buf, "End Ubsan report");
    char *hwasanOutput = strstr(buf, "End Hwasan report");
    char *asanOutput = strstr(buf, "End Asan report");
    if (gwpOutput) {
        std::string gwpasanlog = g_asanlog.str();
        std::string errType = "GWP-ASAN";
        ReadGwpAsanRecord(gwpasanlog, errType);
        // clear buffer
        g_asanlog.str("");
    } else if (tsanOutput) {
        std::string tsanlog = g_asanlog.str();
        std::string errType = "TSAN";
        ReadGwpAsanRecord(tsanlog, errType);
        // clear buffer
        g_asanlog.str("");
    } else if (cfiOutput || ubsanOutput) {
        std::string ubsanlog = g_asanlog.str();
        std::string errType = "UBSAN";
        ReadGwpAsanRecord(ubsanlog, errType);
        // clear buffer
        g_asanlog.str("");
    } else if (hwasanOutput) {
        std::string hwasanlog = g_asanlog.str();
        std::string errType = "HWASAN";
        ReadGwpAsanRecord(hwasanlog, errType);
        // clear buffer
        g_asanlog.str("");
    } else if (asanOutput) {
        std::string asanlog = g_asanlog.str();
        std::string errType = "ASAN_" + GetErrorTypeFromAsanLog(asanlog);
        ReadGwpAsanRecord(asanlog, errType);
        // clear buffer
        g_asanlog.str("");
    }
}

std::string GetErrorTypeFromAsanLog(const std::string& gwpAsanBuffer)
{
    std::string errType = "";
    std::smatch captured;
    constexpr const char* const asanRecordRegex =
        "==([0-9a-zA-Z_.]+)==(\\d+)==ERROR: (AddressSanitizer|LeakSanitizer): (\\S+)";
    static const std::regex recordRe(asanRecordRegex);
    if (std::regex_search(gwpAsanBuffer, captured, recordRe)) {
        errType = captured[ERRTYPE_FIELD].str();
        HILOG_INFO(LOG_CORE, "ASAN errType is %{public}s.", errType.c_str());
    } else {
        HILOG_INFO(LOG_CORE, "ASAN Regex not match, set default type.");
        errType = "ASAN";
    }
    return errType;
}

void ReadGwpAsanRecord(const std::string& gwpAsanBuffer, const std::string& errType)
{
    GwpAsanCurrInfo currInfo;
    currInfo.description = ((gwpAsanBuffer.size() > ASAN_LOG_SIZE) ? (gwpAsanBuffer.substr(0, ASAN_LOG_SIZE) +
                                                                     "\nEnd Asan report") : gwpAsanBuffer);
    currInfo.pid = getpid();
    currInfo.uid = getuid();
    currInfo.errType = errType;
    currInfo.procName = GetNameByPid(currInfo.pid);
    currInfo.appVersion = "";
    time_t timeNow = time(nullptr);
    uint64_t timeTmp = timeNow;
    std::string timeStr = OHOS::HiviewDFX::GetFormatedTime(timeTmp);
    currInfo.happenTime = std::stoll(timeStr);
    std::string fullName = CalcCollectedLogName(currInfo);
    currInfo.logPath = fullName;
    HILOG_INFO(LOG_CORE, "ReportSanitizerAppEvent: uid:%{public}d, logPath:%{public}s.",
        currInfo.uid, currInfo.logPath.c_str());

    // Do upload when data ready
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::RELIABILITY, "ADDR_SANITIZER",
                    OHOS::HiviewDFX::HiSysEvent::EventType::FAULT,
                    "MODULE", currInfo.procName,
                    "VERSION", currInfo.appVersion,
                    "REASON", currInfo.errType,
                    "PID", currInfo.pid,
                    "UID", currInfo.uid,
                    "SUMMARY", currInfo.description,
                    "HAPPEN_TIME", currInfo.happenTime,
                    "LOG_PATH", currInfo.logPath);
}

std::string CalcCollectedLogName(const GwpAsanCurrInfo &currInfo)
{
    std::string filePath = "/data/log/faultlog/faultlogger/";
    std::string prefix = "";
    if (currInfo.errType.compare("GWP-ASAN") == 0) {
        prefix = "gwpasan";
    } else if (currInfo.errType.compare("TSAN") == 0) {
        prefix = "tsan";
    } else if (currInfo.errType.compare("UBSAN") == 0) {
        prefix = "ubsan";
    } else if (currInfo.errType.compare("HWASAN") == 0) {
        prefix = "hwasan";
    } else if (currInfo.errType.compare("ASAN") == 0) {
        prefix = "asan";
    } else {
        prefix = "sanitizer";
    }
    std::string name = currInfo.procName;
    if (name.find("/") != std::string::npos) {
        name = currInfo.procName.substr(currInfo.procName.find_last_of("/") + 1);
    }

    std::string fileName = "";
    fileName.append(prefix);
    fileName.append("-");
    fileName.append(name);
    fileName.append("-");
    fileName.append(std::to_string(currInfo.uid));
    fileName.append("-");
    fileName.append(std::to_string(currInfo.happenTime));

    std::string fullName = filePath + fileName;
    return fullName;
}

std::string GetNameByPid(int32_t pid)
{
    char path[BUF_SIZE] = { 0 };
    int err = snprintf_s(path, sizeof(path), sizeof(path) - 1, "/proc/%d/cmdline", pid);
    if (err <= 0) {
        return "";
    }
    char cmdline[BUF_SIZE] = { 0 };
    size_t i = 0;
    FILE *fp = fopen(path, "r");
    if (fp == nullptr) {
        return "";
    }
    while (i < (BUF_SIZE - 1)) {
        char c = static_cast<char>(fgetc(fp));
        // 0. don't need args of cmdline
        // 1. ignore unvisible character
        if (!isgraph(c)) {
            break;
        }
        cmdline[i] = c;
        i++;
    }
    (void)fclose(fp);
    return cmdline;
}
