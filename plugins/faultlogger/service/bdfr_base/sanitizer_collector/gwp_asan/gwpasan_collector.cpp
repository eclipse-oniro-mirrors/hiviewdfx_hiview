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

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <mutex>
#include <securec.h>
#include <sys/time.h>
#include <unistd.h>
#include <parameters.h>

#include "hisysevent.h"
#include "hisysevent_easy.h"
#include "faultloggerd_client.h"
#include "file_util.h"
#include "common_defines.h"
#include "dfx_signal_handler.h"

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002D12

#undef LOG_TAG
#define LOG_TAG "Sanitizer"

namespace {
constexpr char ADDR_SANITIZER_EVENT[] = "ADDR_SANITIZER";
constexpr int DEFAULT_BUFFER_SIZE = 64;
constexpr unsigned BUF_SIZE = 128;
constexpr unsigned MAX_HISYSEVENT_SIZE = 1000;
constexpr unsigned MAX_EXTRACT_FRAME_NUM = 3;
constexpr unsigned FIRST_FRAME_IDX = 0;
constexpr unsigned SECOND_FRAME_IDX = 1;
constexpr unsigned LAST_FRAME_IDX = 2;
constexpr mode_t DEFAULT_SANITIZER_LOG_MODE = 0644;
constexpr uint64_t TIME_RATIO = 1000;
constexpr int DECIMAL_BASE = 10;
constexpr unsigned TELEMETRY_ID_FIELD_MAX_LEN = 50;  // Max length for ";TELEMETRY_ID:xxxxx"
constexpr unsigned MIN_SUMMARY_SPACE = 10;  // Minimum space required for SUMMARY field
constexpr unsigned HEX_PREFIX_LEN = 2;  // Length of "0x" or "0X"
constexpr unsigned HEX_OFFSET_PREFIX_LEN = 3;  // Length of "+0x"
constexpr unsigned COLON_SPACE_LEN = 2;  // Length of ": "

// Fault type strings
constexpr char END_GWP_ASAN_REPORT[] = "End GWP-ASan report";
constexpr char END_TSAN_REPORT[] = "End Tsan report";
constexpr char END_CFI_REPORT[] = "End CFI report";
constexpr char END_UBSAN_REPORT[] = "End Ubsan report";
constexpr char END_HWASAN_REPORT[] = "End Hwasan report";
constexpr char END_ASAN_REPORT[] = "End Asan report";

constexpr char FAULT_TYPE_GWP_ASAN[] = "GWP-ASAN";
constexpr char FAULT_TYPE_TSAN[] = "TSAN";
constexpr char FAULT_TYPE_UBSAN[] = "UBSAN";
constexpr char FAULT_TYPE_HWASAN[] = "HWASAN";
constexpr char FAULT_TYPE_ASAN[] = "ASAN";

constexpr char FAULTLOGGER_PATH[] = "faultlogger";

// Stack ignore list - using const char* array instead of unordered_set
constexpr const char* IGNORE_STACKS[] = {
    "libclang_rt.hwasan.so",
    "libclang_rt.asan.so",
    "libclang_rt.tsan.so",
    "libclang_rt.ubsan_standalone.so",
    "ld-musl-aarch64.so",
    "ld-musl-aarch64-asan.so",
    "libc++.so",
    "libc++_shared.so"
};
constexpr size_t IGNORE_STACKS_COUNT = sizeof(IGNORE_STACKS) / sizeof(IGNORE_STACKS[0]);

std::string g_asanlog;
std::mutex g_sMutex;
}

static std::string GetFormatedTime(uint64_t target)
{
    time_t now = time(nullptr);
    if (target > static_cast<uint64_t>(now)) {
        target = target / TIME_RATIO;
    }

    time_t out = static_cast<time_t>(target);
    struct tm tmStruct {0};
    struct tm* timeInfo = localtime_r(&out, &tmStruct);
    if (timeInfo == nullptr) {
        return "00000000000000";
    }

    char buf[DEFAULT_BUFFER_SIZE] = {0};
    (void)strftime(buf, DEFAULT_BUFFER_SIZE - 1, "%Y%m%d%H%M%S", timeInfo);
    return buf;  // Implicit conversion from char array to string
}

void WriteSanitizerLog(char* buf, size_t sz, char* path)
{
    if (buf == nullptr || sz == 0) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_sMutex);
    // append to buffer
    g_asanlog.append(buf, sz);

    const char* faultType = nullptr;
    if (strstr(buf, END_GWP_ASAN_REPORT) != nullptr) {
        faultType = FAULT_TYPE_GWP_ASAN;
    } else if (strstr(buf, END_TSAN_REPORT) != nullptr) {
        faultType = FAULT_TYPE_TSAN;
    } else if (strstr(buf, END_CFI_REPORT) != nullptr || strstr(buf, END_UBSAN_REPORT) != nullptr) {
        faultType = FAULT_TYPE_UBSAN;
    } else if (strstr(buf, END_HWASAN_REPORT) != nullptr) {
        faultType = FAULT_TYPE_HWASAN;
    } else if (strstr(buf, END_ASAN_REPORT) != nullptr) {
        faultType = FAULT_TYPE_ASAN;
    }

    if (faultType != nullptr) {
        ReadGwpAsanRecord(g_asanlog, faultType, path);
        g_asanlog.clear();
    }
}

void ReadGwpAsanRecord(const std::string& gwpAsanBuffer, const std::string& faultType, char* logPath)
{
    GwpAsanCurrInfo currInfo;
    currInfo.description = gwpAsanBuffer;

    const bool isAsanType = (faultType == FAULT_TYPE_ASAN || faultType == FAULT_TYPE_HWASAN);
    if (logPath == nullptr || strlen(logPath) == 0 || !isAsanType) {
        currInfo.logPath = FAULTLOGGER_PATH;
    } else {
        currInfo.logPath = logPath;
    }
    currInfo.pid = getprocpid();
    currInfo.uid = static_cast<int32_t>(getuid());
    currInfo.faultType = faultType;
    currInfo.errType = GetErrorTypeFromBuffer(gwpAsanBuffer, faultType);
    currInfo.moduleName = GetNameByPid(currInfo.pid);
    time_t timeNow = time(nullptr);
    uint64_t timeTmp = static_cast<uint64_t>(timeNow);
    std::string timeStr = GetFormatedTime(timeTmp);
    currInfo.happenTime = static_cast<uint64_t>(strtoull(timeStr.c_str(), nullptr, DECIMAL_BASE));
    currInfo.topStacks = GetTopStackWithoutCommonLib(currInfo.description);
    currInfo.telemetryId = OHOS::system::GetParameter("persist.hiviewdfx.priv.diagnosis.time.taskId", "");
    currInfo.appRunningId = &DFX_GetAppRunningUniqueId == nullptr ? "" : DFX_GetAppRunningUniqueId();

    bool isSendHisysevent = false;
    WriteCollectedData(currInfo, isSendHisysevent);
    if (isSendHisysevent) {
        SendSanitizerHisysevent(currInfo);
    }
}

void SendSanitizerHisysevent(const GwpAsanCurrInfo& currInfo)
{
    char params[MAX_HISYSEVENT_SIZE] = {0};
    int len = snprintf_s(params, sizeof(params), sizeof(params) - 1,
        "FAULT_TYPE:%s;MODULE:%s;REASON:%s;PID:%d;UID:%d;HAPPEN_TIME:%llu;"
        "FIRST_FRAME:%s;SECOND_FRAME:%s;LAST_FRAME:%s;APP_RUNNING_UNIQUE_ID:%s",
        currInfo.faultType.c_str(), currInfo.moduleName.c_str(), currInfo.errType.c_str(),
        currInfo.pid, currInfo.uid, static_cast<unsigned long long>(currInfo.happenTime),
        currInfo.topStacks[FIRST_FRAME_IDX].c_str(),
        currInfo.topStacks[SECOND_FRAME_IDX].c_str(), currInfo.topStacks[LAST_FRAME_IDX].c_str(),
        currInfo.appRunningId.c_str());
    if (len < 0) {
        HILOG_ERROR(LOG_CORE, "Failed to format HiSysEvent params");
        return;
    }

    // Add TELEMETRY_ID if available and there's enough space
    if (!currInfo.telemetryId.empty() && static_cast<size_t>(len) < sizeof(params) - TELEMETRY_ID_FIELD_MAX_LEN) {
        int remaining = static_cast<int>(sizeof(params)) - len - 1;
        int addLen = snprintf_s(params + len, remaining, remaining - 1,
            ";TELEMETRY_ID:%s", currInfo.telemetryId.c_str());
        if (addLen > 0) {
            len += addLen;
        }
    }

    // Check if there's enough space for SUMMARY field
    int remaining = static_cast<int>(sizeof(params)) - len - 1;
    if (remaining <= static_cast<int>(MIN_SUMMARY_SPACE)) {
        HiSysEventEasyWrite(OHOS::HiviewDFX::HiSysEvent::Domain::RELIABILITY,
            ADDR_SANITIZER_EVENT, HiSysEventEasyType::EASY_EVENT_TYPE_FAULT, params);
        return;
    }

    // Add SUMMARY field prefix
    int addLen = snprintf_s(params + len, remaining, remaining - 1, ";SUMMARY:");
    if (addLen <= 0) {
        HiSysEventEasyWrite(OHOS::HiviewDFX::HiSysEvent::Domain::RELIABILITY,
            ADDR_SANITIZER_EVENT, HiSysEventEasyType::EASY_EVENT_TYPE_FAULT, params);
        return;
    }

    // Copy description content
    len += addLen;
    remaining -= addLen;
    size_t copyLen = std::min(static_cast<size_t>(remaining), currInfo.description.size());
    if (copyLen > 0) {
        errno_t err = memcpy_s(params + len, remaining, currInfo.description.c_str(), copyLen);
        if (err == EOK) {
            len += static_cast<int>(copyLen);
            params[len] = '\0';
        }
    }

    HiSysEventEasyWrite(OHOS::HiviewDFX::HiSysEvent::Domain::RELIABILITY,
        ADDR_SANITIZER_EVENT, HiSysEventEasyType::EASY_EVENT_TYPE_FAULT, params);
}

std::string GetErrorTypeFromBuffer(const std::string& buffer, const std::string& faultType)
{
    // ASAN error type - replace regex with string search
    const char* summaryPos = strstr(buffer.c_str(), "SUMMARY: ");
    if (summaryPos != nullptr) {
        const char* asanPos = strstr(summaryPos, "AddressSanitizer: ");
        const char* leakPos = strstr(summaryPos, "LeakSanitizer: ");
        const char* errorPos = nullptr;

        if (asanPos != nullptr) {
            errorPos = asanPos + strlen("AddressSanitizer: ");
        } else if (leakPos != nullptr) {
            errorPos = leakPos + strlen("LeakSanitizer: ");
        }

        if (errorPos != nullptr) {
            // Extract error type until whitespace
            const char* end = errorPos;
            while (*end && !isspace(*end)) {
                end++;
            }
            if (end > errorPos) {
                return std::string(errorPos, end - errorPos);
            }
        }
    }

    // HWASAN error type - replace regex with string search
    const char* causePos = strstr(buffer.c_str(), "Potential Cause: ");
    if (causePos == nullptr) {
        causePos = strstr(buffer.c_str(), "Cause: ");
    }
    if (causePos != nullptr) {
        // Find the space after the colon (": ")
        const char* colonPos = strchr(causePos, ':');
        if (colonPos != nullptr && colonPos[1] == ' ') {
            const char* start = colonPos + COLON_SPACE_LEN; // Skip ": "
            // Extract error type until newline
            const char* end = strchr(start, '\n');
            if (end == nullptr) {
                end = start + strlen(start);
            }
            // Trim trailing whitespace
            while (end > start && isspace(*(end - 1))) {
                end--;
            }
            if (end > start) {
                return std::string(start, end - start);
            }
        }
    }

    return faultType;
}

void WriteCollectedData(const GwpAsanCurrInfo& currInfo, bool& isSendHisysevent)
{
    if (currInfo.logPath != FAULTLOGGER_PATH && WriteToSandbox(currInfo)) {
        return;
    }

    isSendHisysevent = true;
    WriteToFaultLogger(currInfo);
}

void WriteToFaultLogger(const GwpAsanCurrInfo& currInfo)
{
    struct FaultLoggerdRequest request;
    (void)memset_s(&request, sizeof(request), 0, sizeof(request));
    request.type = FaultLoggerType::ADDR_SANITIZER;
    request.pid = currInfo.pid;
    request.time = currInfo.happenTime;
    int fd = RequestFileDescriptorEx(&request);
    if (fd < 0) {
        return;
    }

    OHOS::HiviewDFX::FileUtil::SaveStringToFd(fd, currInfo.description);
    close(fd);
}

bool IsValidSandboxPath(const std::string& realPath)
{
    const char* prefix = "/data/storage/el";
    size_t prefixLen = strlen(prefix);
    if (realPath.compare(0, prefixLen, prefix) != 0) {
        return false;
    }

    size_t numStart = prefixLen;
    if (numStart >= realPath.length()) {
        return false;
    }

    size_t p = numStart;
    while (p < realPath.length() && isdigit(realPath[p])) {
        p++;
    }

    if (p < realPath.length() && realPath[p] != '/') {
        return false;
    }

    return (p != numStart);
}

bool WriteToSandbox(const GwpAsanCurrInfo& currInfo)
{
    auto pos = currInfo.logPath.find_last_of('/');
    if (pos == std::string::npos || pos == currInfo.logPath.length() - 1) {
        return false;
    }

    std::string logDir = currInfo.logPath.substr(0, pos);
    std::string fileName = currInfo.logPath.substr(pos + 1);
    std::string realPath;
    if (!OHOS::HiviewDFX::FileUtil::PathToRealPath(logDir, realPath)) {
        return false;
    }

    if (!IsValidSandboxPath(realPath)) {
        return false;
    }

    char logFilePath[512];
    int pathLen = snprintf_s(logFilePath, sizeof(logFilePath), sizeof(logFilePath) - 1,
        "%s/%s.%d.%llu", realPath.c_str(), fileName.c_str(),
        currInfo.pid, static_cast<unsigned long long>(currInfo.happenTime));
    if (pathLen <= 0 || pathLen >= static_cast<int>(sizeof(logFilePath))) {
        return false;
    }

    int fd = open(logFilePath, O_CREAT | O_WRONLY | O_TRUNC | O_NOFOLLOW, DEFAULT_SANITIZER_LOG_MODE);
    if (fd < 0) {
        return false;
    }

    char content[4096];
    std::string productName = OHOS::system::GetParameter("const.product.name", "Unknown");
    std::string displayVersion = OHOS::system::GetParameter("const.display.version", "Unknown");

    int len = snprintf_s(content, sizeof(content), sizeof(content) - 1,
        "Generated by HiviewDFX @OpenHarmony\n"
        "===============================================================\n"
        "Device info:%s\n"
        "Build info:%s\n"
        "Timestamp:%llu\n"
        "Module name:%s\n"
        "Pid:%d\n"
        "Uid:%d\n"
        "Reason:%s\n",
        productName.c_str(), displayVersion.c_str(),
        static_cast<unsigned long long>(currInfo.happenTime), currInfo.moduleName.c_str(),
        currInfo.pid, currInfo.uid, currInfo.errType.c_str());
    if (len > 0) {
        OHOS::HiviewDFX::FileUtil::SaveStringToFd(fd, std::string(content, len));
    }
    OHOS::HiviewDFX::FileUtil::SaveStringToFd(fd, currInfo.description);
    close(fd);
    return true;
}

bool IsIgnoreStack(const std::string& stack)
{
    for (size_t i = 0; i < IGNORE_STACKS_COUNT; i++) {
        if (stack.find(IGNORE_STACKS[i]) != std::string::npos) {
            return true;
        }
    }
    return false;
}

static size_t SkipDigits(const std::string& str, size_t pos)
{
    while (pos < str.length() && str[pos] >= '0' && str[pos] <= '9') {
        pos++;
    }
    return pos;
}

static size_t SkipWhitespace(const std::string& str, size_t pos)
{
    while (pos < str.length() && isspace(str[pos])) {
        pos++;
    }
    return pos;
}

static size_t SkipHexAddress(const std::string& str, size_t pos)
{
    while (pos < str.length() && isxdigit(str[pos])) {
        pos++;
    }
    return pos;
}

static size_t SkipWhitespaceAndParen(const std::string& str, size_t pos)
{
    while (pos < str.length() && (isspace(str[pos]) || str[pos] == '(')) {
        pos++;
    }
    return pos;
}

static bool IsValidStackFrame(const std::string& str, size_t pos)
{
    if (pos + 1 >= str.length()) {
        return false;
    }
    size_t numStart = pos + 1;
    if (str[numStart] < '0' || str[numStart] > '9') {
        return false;
    }

    size_t afterNum = SkipDigits(str, numStart);
    afterNum = SkipWhitespace(str, afterNum);
    if (afterNum + HEX_PREFIX_LEN > str.length()) {
        return false;
    }
    if (str.compare(afterNum, HEX_PREFIX_LEN, "0x") != 0 && str.compare(afterNum, HEX_PREFIX_LEN, "0X") != 0) {
        return false;
    }

    return true;
}

static size_t FindModuleEnd(const std::string& str, size_t pos)
{
    size_t plusPos = str.find("+0x", pos);
    if (plusPos == std::string::npos) {
        return std::string::npos;
    }

    size_t afterPlus = plusPos + HEX_OFFSET_PREFIX_LEN;
    if (afterPlus >= str.length() || !isxdigit(str[afterPlus])) {
        return std::string::npos;
    }

    return plusPos;
}

static std::string ExtractModuleFromFrame(const std::string& str, size_t pos)
{
    size_t numStart = pos + 1;
    size_t afterNum = SkipDigits(str, numStart);
    afterNum = SkipWhitespace(str, afterNum);
    size_t afterAddr = afterNum + HEX_PREFIX_LEN;
    afterAddr = SkipHexAddress(str, afterAddr);
    size_t moduleStart = SkipWhitespaceAndParen(str, afterAddr);

    size_t plusPos = FindModuleEnd(str, moduleStart);
    if (plusPos == std::string::npos || plusPos <= moduleStart) {
        return "";
    }

    // Find the end of offset (hex digits after +0x)
    size_t offsetEnd = plusPos + HEX_OFFSET_PREFIX_LEN;
    while (offsetEnd < str.length() && isxdigit(str[offsetEnd])) {
        offsetEnd++;
    }

    // Return module+offset format
    return str.substr(moduleStart, offsetEnd - moduleStart);
}

// Helper function: limit search scope to relevant area
static std::string LimitSearchScope(const std::string& description, size_t firstStackPos)
{
    if (firstStackPos == std::string::npos) {
        return description;
    }

    const char* boundaries[] = {
        "\n\n", "\r\n\r\n", "allocated by:", "Allocated By",
        "Previous write", "Previous read", "Location is", "Mutex",
        "freed by:", "Freed by"
    };
    size_t minPos = std::string::npos;
    for (const char* b : boundaries) {
        size_t boundaryPos = description.find(b, firstStackPos);
        if (boundaryPos != std::string::npos && boundaryPos < minPos) {
            minPos = boundaryPos;
        }
    }

    if (minPos == std::string::npos) {
        return description;
    }
    return description.substr(0, minPos);
}

// Helper function: search for valid stack frames within specified scope
static void SearchStackFrames(const std::string& record, std::vector<std::string>& topStacks,
    std::vector<std::string>& fallbackStacks)
{
    size_t pos = 0;
    while (pos < record.length() && topStacks.size() < MAX_EXTRACT_FRAME_NUM) {
        size_t hashPos = record.find('#', pos);
        if (hashPos == std::string::npos) {
            break;
        }

        if (!IsValidStackFrame(record, hashPos)) {
            pos = hashPos + 1;
            continue;
        }

        std::string module = ExtractModuleFromFrame(record, hashPos);
        if (!module.empty()) {
            if (fallbackStacks.size() < MAX_EXTRACT_FRAME_NUM) {
                fallbackStacks.push_back(module);
            }
            if (!IsIgnoreStack(module) && topStacks.size() < MAX_EXTRACT_FRAME_NUM) {
                topStacks.push_back(module);
            }
        }

        size_t plusPos = record.find("+0x", hashPos);
        if (plusPos != std::string::npos) {
            pos = plusPos + HEX_OFFSET_PREFIX_LEN;
            while (pos < record.length() && isxdigit(record[pos])) {
                pos++;
            }
        } else {
            pos = hashPos + 1;
        }
    }
}

std::vector<std::string> GetTopStackWithoutCommonLib(const std::string& description)
{
    std::vector<std::string> topStacks;
    std::vector<std::string> fallbackStacks;
    size_t pos = 0;

    // Find the first valid stack frame position
    size_t firstStackPos = std::string::npos;
    while (pos < description.length()) {
        size_t hashPos = description.find('#', pos);
        if (hashPos == std::string::npos) {
            break;
        }
        if (IsValidStackFrame(description, hashPos)) {
            firstStackPos = hashPos;
            break;
        }
        pos = hashPos + 1;
    }

    // Limit search scope and search for stack frames
    std::string record = LimitSearchScope(description, firstStackPos);
    SearchStackFrames(record, topStacks, fallbackStacks);

    // Handle fallback and padding
    if (topStacks.empty() && !fallbackStacks.empty()) {
        topStacks = fallbackStacks;
    }
    while (topStacks.size() < MAX_EXTRACT_FRAME_NUM) {
        topStacks.push_back("/");
    }
    return topStacks;
}

std::string GetNameByPid(int32_t pid)
{
    char path[BUF_SIZE] = { 0 };
    int err = snprintf_s(path, sizeof(path), sizeof(path) - 1, "/proc/%d/cmdline", pid);
    if (err <= 0) {
        return "";
    }
    char cmdline[BUF_SIZE] = { 0 };
    FILE *fp = fopen(path, "r");
    if (fp == nullptr) {
        return "";
    }
    size_t i = 0;
    while (i < (BUF_SIZE - 1)) {
        char c = static_cast<char>(fgetc(fp));
        if (!isgraph(c)) {
            break;
        }
        cmdline[i] = c;
        i++;
    }
    (void)fclose(fp);
    return cmdline;
}
