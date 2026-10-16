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
const char ADDR_SANITIZER_EVENT[] = "ADDR_SANITIZER";
constexpr int DEFAULT_BUFFER_SIZE = 64;
constexpr int DECIMAL_BASE = 10;
constexpr unsigned MAX_HISYSEVENT_SIZE = 1000;
constexpr int MILLISECONDS_PER_SECOND = 1000;
constexpr int MANOSECONDS_PER_MILLISECOND = 1000 * 1000;
const mode_t DEFAULT_SANITIZER_LOG_MODE = 0644;
const unsigned MIN_SUMMARY_SPACE = 10;

// Fault type strings
const char END_GWP_ASAN_REPORT[] = "End GWP-ASan report";
const char END_TSAN_REPORT[] = "End Tsan report";
const char END_CFI_REPORT[] = "End CFI report";
const char END_UBSAN_REPORT[] = "End Ubsan report";
const char END_HWASAN_REPORT[] = "End Hwasan report";
const char END_ASAN_REPORT[] = "End Asan report";

const char FAULT_TYPE_GWP_ASAN[] = "GWP-ASAN";
const char FAULT_TYPE_TSAN[] = "TSAN";
const char FAULT_TYPE_UBSAN[] = "UBSAN";
const char FAULT_TYPE_HWASAN[] = "HWASAN";
const char FAULT_TYPE_ASAN[] = "ASAN";

const char FAULTLOGGER_PATH[] = "faultlogger";
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
        (static_cast<uint64_t>(ts.tv_nsec) / MANOSECONDS_PER_MILLISECOND);
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
    } else if (strstr(g_asanlog, END_CFI_REPORT) != nullptr || strstr(g_asanlog, END_UBSAN_REPORT) != nullptr) {
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
    g_currInfo.description[descLen] = '\0';
    g_currInfo.descriptionLen = descLen;
    const bool isAsanType = (strcmp(faultType, FAULT_TYPE_ASAN) == 0 || strcmp(faultType, FAULT_TYPE_HWASAN) == 0);
    if (logPath == nullptr || strlen(logPath) == 0 || !isAsanType) {
        errno_t err = strncpy_s(g_currInfo.logPath, sizeof(g_currInfo.logPath),
            FAULTLOGGER_PATH, sizeof(g_currInfo.logPath) - 1);
        if (err != EOK) {
            g_currInfo.logPath[0] = '\0';
        }
    } else {
        errno_t err = strncpy_s(g_currInfo.logPath, sizeof(g_currInfo.logPath),
            logPath, sizeof(g_currInfo.logPath) - 1);
        if (err != EOK) {
            strncpy_s(g_currInfo.logPath, sizeof(g_currInfo.logPath),
                FAULTLOGGER_PATH, sizeof(g_currInfo.logPath) - 1);
        }
    }
    g_currInfo.pid = getprocpid();
    g_currInfo.uid = static_cast<int32_t>(getuid());
    strncpy_s(g_currInfo.faultType, sizeof(g_currInfo.faultType), faultType, strlen(faultType));
    GetNameByPid(g_currInfo.pid, g_currInfo.moduleName, sizeof(g_currInfo.moduleName));
    uint64_t timeTmp = GetTimeMilliseconds();
    char timeBuf[DEFAULT_BUFFER_SIZE] = {0};
    GetFormatedTime(timeTmp, timeBuf, sizeof(timeBuf));
    g_currInfo.happenTime = static_cast<uint64_t>(strtoull(timeBuf, nullptr, DECIMAL_BASE));
    const char* appRunningId = &DFX_GetAppRunningUniqueId == nullptr ? "" : DFX_GetAppRunningUniqueId();
    strncpy_s(g_currInfo.appRunningId, sizeof(g_currInfo.appRunningId), appRunningId, strlen(appRunningId));
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
    if (addLen < 0) {
        HiSysEventEasyWrite(ADDR_SANITIZER_DOMAIN,
            ADDR_SANITIZER_EVENT, HiSysEventEasyType::EASY_EVENT_TYPE_FAULT, params);
        return;
    }
    len += addLen;
    remaining -= addLen;
    if (remaining > 1 && currInfo.descriptionLen > 0) {
        size_t copyLen = (static_cast<size_t>(remaining - 1) < currInfo.descriptionLen) ?
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
    if (strcmp(currInfo.logPath, FAULTLOGGER_PATH) != 0 && WriteToSandbox(currInfo)) {
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

bool IsValidSandboxPath(const char* realPath)
{
    const char* prefix = "/data/storage/el";
    size_t prefixLen = strlen(prefix);
    size_t pathLen = strlen(realPath);
    if (pathLen < prefixLen || strncmp(realPath, prefix, prefixLen) != 0) {
        return false;
    }
    size_t numStart = prefixLen;
    if (numStart >= pathLen) {
        return false;
    }

    size_t p = numStart;
    while (p < pathLen && isdigit(realPath[p])) {
        p++;
    }

    if (p < pathLen && realPath[p] != '/') {
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
    size_t fileLen = logPathLen - pos;
    if (dirLen >= sizeof(logDir) || fileLen >= sizeof(fileName)) {
        return false;
    }
    errno_t err = strncpy_s(logDir, sizeof(logDir), currInfo.logPath, dirLen);
    if (err != EOK) {
        return false;
    }
    errno_t errName = strncpy_s(fileName, sizeof(fileName), currInfo.logPath + pos, fileLen);
    if (errName != EOK) {
        return false;
    }
    char fullPath[512] = {0};
    int fullLen = snprintf_s(fullPath, sizeof(fullPath), sizeof(fullPath) - 1, "%s%s", logDir, fileName);
    if (fullLen <= 0 || fullLen >= static_cast<int>(sizeof(fullPath))) {
        return false;
    }
    char realPath[512] = {0};
    if (!RealpathAsyncSafe(fullPath, realPath, sizeof(realPath))) {
        return false;
    }
    if (!IsValidSandboxPath(realPath)) {
        return false;
    }
    int pathLen = snprintf_s(outPath, outPathSize, outPathSize - 1,
        "%s.%d.%llu", realPath,
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
