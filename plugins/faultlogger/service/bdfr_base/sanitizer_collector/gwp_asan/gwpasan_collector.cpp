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
#include <securec.h>
#include <sys/time.h>
#include <unistd.h>
#include <atomic>

#include "hisysevent_easy.h"
#include "faultloggerd_client.h"
#include "dfx_signal_handler.h"

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002D12
#define FDSAN_TAG_A 1

#undef LOG_TAG
#define LOG_TAG "Sanitizer"

namespace {
const char ADDR_SANITIZER_DOMAIN[] = "RELIABILITY";
constexpr char ADDR_SANITIZER_EVENT[] = "ADDR_SANITIZER";
constexpr int DEFAULT_BUFFER_SIZE = 64;
constexpr int DECIMAL_BASE = 10;
constexpr unsigned MAX_HISYSEVENT_SIZE = 1000;
constexpr unsigned MILLISECONDS_PER_SECOND = 1000;
constexpr unsigned MANOSECONDS_PER_MILLISECOND = 1000 * 1000;
constexpr mode_t DEFAULT_SANITIZER_LOG_MODE = 0644;
const unsigned MIN_SUMMARY_SPACE = 10;

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
constexpr unsigned G_ASAN_LOG_SIZE = 1 * 1024 * 1024;
char g_asanlog[G_ASAN_LOG_SIZE];
std::atomic<size_t> g_asanlogLen;
std::atomic_flag g_asanlogLock = ATOMIC_FLAG_INIT;
static GwpAsanCurrInfo g_currInfo;
}

uint64_t GetTimeMilliseconds()
{
    struct timespec ts;
    (void)clock_gettime(CLOCK_REALTIME, &ts);
    return (static_cast<uint64_t>(ts.tv_sec) * MILLISECONDS_PER_SECOND) +
        (static_cast<uint64_t>(ts.tv_nsec) / NANOSECONDS_PER_MILLISECOND);
}

bool WriteDescriptionToFd(int fd, const char* desc, size_t descLen)
{
    ssize_t written = 0;
    while (written < static_cast<ssize_t>(descLen)) {
        ssize_t w = write(fd, desc + written, descLen - written);
        if (w <= 0) {
            return false;
        }
        written += w;
    }
    return true;
}

void WriteSanitizerLog(char* buf, size_t sz, char* path)
{
    if (buf == nullptr || sz == 0) {
        return;
    }
    if (g_asanlogLock.test_and_set(std::memory_order_acquire)) {
        return;
    }
    size_t logLen = g_asanlogLen.load(std::memory_order_relaxed);
    size_t copyLen = (sz < G_ASAN_LOG_SIZE - 1 - logLen) ? sz : (G_ASAN_LOG_SIZE - 1 - logLen);
    if (copyLen > 0) {
        errno_t err = memcpy_s(g_asanlog + logLen, G_ASAN_LOG_SIZE - logLen, buf, copyLen);
        if (err != EOK) {
            g_asanlogLock.clear(std::memory_order_release);
            return;
        }
    }
    logLen += copyLen;
    g_asanlog[logLen] = '\0';
    g_asanlogLen.store(logLen, std::memory_order_relaxed);
    const char* faultType = nullptr;
    if (strstr(g_asanlog, END_GWP_ASAN_REPORT) != nullptr) {
        faultType = FAULT_TYPE_GWP_ASAN;
    } else if (strstr(g_asanlog, END_TSAN_REPORT) != nullptr) {
        faultType = FAULT_TYPE_TSAN;
    } else if (strstr(g_asanlog, END_CFI_REPORT) != nullptr || strstr(buf, END_UBSAN_REPORT) != nullptr) {
        faultType = FAULT_TYPE_UBSAN;
    } else if (strstr(g_asanlog, END_HWASAN_REPORT) != nullptr) {
        faultType = FAULT_TYPE_HWASAN;
    } else if (strstr(g_asanlog, END_ASAN_REPORT) != nullptr) {
        faultType = FAULT_TYPE_ASAN;
    }
    if (faultType != nullptr) {
        ReadGwpAsanRecordFromBuffer(faultType, path);
        g_asanlogLen.store(0, std::memory_order_relaxed);
    }
    g_asanlogLock.clear(std::memory_order_release);
}

void ReadGwpAsanRecordFromBuffer(const char* faultType, char* logPath)
{
    (void)memset_s(&g_currInfo, sizeof(g_currInfo), 0, sizeof(g_currInfo));
    size_t logLen = g_asanlogLen.load(std::memory_order_relaxed);
    size_t descLen = (logLen < sizeof(g_currInfo.description) - 1) ? logLen :
        (sizeof(g_currInfo.description) - 1);
    if (descLen > 0) {
        (void)memcpy_s(g_currInfo.description, sizeof(g_currInfo.description), g_asanlog, descLen);
    }
    g_asanlog.description[descLen] = '\0';
    g_currInfo.descriptionLen = descLen;
    const bool isAsanType = (strcmp(faultType, FAULT_TYPE_ASAN) == 0 || strcmp(faultType, FAULT_TYPE_HWASAN) == 0);
    if (logPath == nullptr || strlen(logPath) == 0 || !isAsanType) {
        errno_t err = strncpy_s(g_asanlog.logPath, sizeof(g_asanlog.logPath),
            FAULTLOGGER_PATH, sizeof(g_currInfo.logPath) - 1);
        if (err != EOK) {
            g_asanlog.logPath[0] = '\0';
        }
    } else {
        errno_t err = strncpy_s(g_asanlog.logPath, sizeof(g_asanlog.logPath),
            logPath, sizeof(g_currInfo.logPath) - 1);
        if (err != EOK) {
            strncpy_s(g_asanlog.logPath, sizeof(g_asanlog.logPath),
                FAULTLOGGER_PATH, sizeof(g_currInfo.logPath) - 1);
        }
    }
    currInfo.pid = getprocpid();
    currInfo.uid = static_cast<int32_t>(getuid());
    strncpy_s(g_currInfo.faultType, sizeof(g_currInfo.faultType), faultType, strlen(faultType));
    GetNameByPid(g_currInfo.pid, g_currInfo.moduleName, sizeof(g_currInfo.moduleName));
    uint64_t timeTmp = static_cast<uint64_t>(timeNow);
    char timeBuf[DEFAULT_BUFFER_SIZE] = {0};
    GetFormatedTime(timeTmp, timeBuf, sizeof(timeBuf));
    currInfo.happenTime = static_cast<uint64_t>(strtoull(timeStr.c_str(), nullptr, DECIMAL_BASE));
    const char* appRunningId = &DFX_GetAppRunningUniqueId == nullptr ? "" : DFX_GetAppRunningUniqueId();
    strncpy_s(g_asanlog.appRunningId, sizeof(g_asanlog.appRunningId), appRunningId, strlen(appRunningId));
    bool isSendHisysevent = false;
    WriteCollectedData(g_currInfo, isSendHisysevent);
    if (isSendHisysevent) {
        SendSanitizerHisysevent(g_currInfo);
    }
}

void SendSanitizerHisysevent(const GwpAsanCurrInfo& currInfo)
{
    char params[MAX_HISYSEVENT_SIZE] = {0};
    int len = snprintf_s(params, sizeof(params), sizeof(params) - 1,
        "FAULT_TYPE:%s;MODULE:%s;PID:%d;UID:%d;HAPPEN_TIME:%llu;"
        "APP_RUNNING_UNIQUE_ID:%s",
        currInfo.faultType, currInfo.moduleName,
        currInfo.pid, currInfo.uid, static_cast<unsigned long long>(currInfo.happenTime),
        currInfo.appRunningId);
    if (len < 0) {
        return;
    }
    int remaining = static_cast<int>(sizeof(params)) - len;
    if (remaining <= static_cast<int>(MIN_SUMMARY_SPACE)) {
        HiSysEventEasyWrite(ADDR_SANITIZER_DOMAIN,
            ADDR_SANITIZER_EVENT, HiSysEventEasyType::EASY_EVENT_TYPE_FAULT, params);
        return;
    }
    int addLen = snprintf_s(params + len, remaining, remaining - 1, ";SUMMARY:");
    if (addLen <= 0) {
        HiSysEventEasyWrite(ADDR_SANITIZER_DOMAIN,
            ADDR_SANITIZER_EVENT, HiSysEventEasyType::EASY_EVENT_TYPE_FAULT, params);
        return;
    }
    len += addLen;
    remaining -= addLen;
    if (remaining > 1 && currInfo.description > 0) {
        size_t copyLen = (static_cast<size_t>(remaining - 1) < currInfo.description) ?
            (static_cast<size_t>(remaining - 1)) : currInfo.descriptionLen;
        errno_t err = memcpy_s(params + len, static_cast<size_t>(remaining),
            currInfo.description, copyLen);
        if (err == EOK) {
            len += static_cast<int>(copyLen);
            params[len] = '\0';
        }
    }
    HiSysEventEasyWrite(ADDR_SANITIZER_DOMAIN,
        ADDR_SANITIZER_EVENT, HiSysEventEasyType::EASY_EVENT_TYPE_FAULT, params);
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
    if (!WriteDescriptionToFd(fd, currInfo.description, currInfo.descriptionLen)) {
        close(fd);
        return;
    }
    close(fd);
}

bool IsValidSandboxPath(const std::string& realPath)
{
    const char* prefix = "/data/storage/el";
    size_t prefixLen = strlen(prefix);
    size_t pathLen = strLen(realPath);
    if (pathLen < prefixLen || strncmp(realPath, prefix, prefixLen) != 0) {
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

bool BuildSandboxFilePath(const GwpAsanCurrInfo& currInfo, char* outPath, size_t outPathSize)
{
    size_t logPathLen = strlen(currInfo.logPath);
    if (logPathLen == 0) {
        return false;
    }
    size_t pos = logPathLen;
    while (pos > 0 && currInfo.logPath[pos - 1] != '/') {
        pos--;
    }
    if (pos == 0 || pos == logPathLen) {
        return false;
    }
    char logDir[256] = {0};
    char fileName[128] = {0};
    size_t dirLen = pos;
    size_t fileLen = logPathLen - pos - 1;
    if (dirLen >= sizeof(logDir) || fileLen >= sizeof(fileName)) {
        return false;
    }
    strncpy_s(logDir, sizeof(logDir), currInfo.logPath, dirLen);
    strncpy_s(fileName, sizeof(fileName), currInfo.logPath + pos + 1, fileLen);
    char realPath[256] = {0};
    if (!RealpathAsyncSafe(logDir, realPath, sizeof(realPath))) {
        return false;
    }
    if (!IsValidSandboxPath(realPath)) {
        return false;
    }
    int pathLen = snprintf_s(outPath, outPathSize, outPathSize - 1,
        "%s/%s.%d.%llu", realPath, fileName,
        currInfo.pid, static_cast<unsigned long long>(currInfo.happenTime));
    if (pathLen <= 0 || pathLen >= static_cast<int>(outPathSize)) {
        return false;
    }
    return true;
}

bool WriteToSandbox(const GwpAsanCurrInfo& currInfo)
{
    uint64_t newTag = (static_cast<uint64_t>(LOG_DOMAIN) << 32) | FDSAN_TAG_A;
    char logFilePath[512];
    if (!BuildSandboxFilePath(currInfo, logFilePath, sizeof(logFilePath))) {
        return false;
    }
    int fd = open(logFilePath, O_CREAT | O_WRONLY | O_TRUNC | O_NOFOLLOW, DEFAULT_SANITIZER_LOG_MODE);
    if (fd < 0) {
        return false;
    }
    fdsan_exchange_owner_tag(fd, 0, newTag);
    if (!WriteDescriptionToFd(fd, currInfo.description, currInfo.descriptionLen)) {
        fdsan_close_with_tag(fd, newTag);
        return false;
    }
    fdsan_close_with_tag(fd, newTag);
    return true;
}
