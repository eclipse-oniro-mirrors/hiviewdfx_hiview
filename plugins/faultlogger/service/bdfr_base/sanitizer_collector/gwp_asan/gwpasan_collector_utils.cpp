/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
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
#include "gwpasan_collector_utils.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <securec.h>
#include <sys/time.h>
#include <unistd.h>

#include "dfx_signal_handler.h"
#include "faultloggerd_client.h"
#include "hisysevent_easy.h"

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002D12

#undef LOG_TAG
#define LOG_TAG "Sanitizer"

namespace {
constexpr unsigned BUF_SIZE = 128;
constexpr uint64_t TIME_RATIO = 1000;
constexpr size_t TIME_FORMAT_LEN = 17;
constexpr char ZERO_TIME_FORMAT[TIME_FORMAT_LEN + 1] = "00000000000000000";
constexpr size_t MAX_INPUT_PATH_LEN = 512;
constexpr size_t ABS_PATH_BUF_SIZE = 512;
constexpr size_t SYMLINK_CHECK_BUF_SIZE = 512;
constexpr size_t MIN_RESOLVED_BUF_SIZE = 2;
constexpr size_t CURRENT_DIR_BUF_SIZE = 1;
constexpr size_t PARENT_DIR_BUF_SIZE = 2;
constexpr int TIMEZONE_OFFSET_SECONDS = 8 * 3600;  // UTC+8
constexpr char END_GWP_ASAN_REPORT[] = "End GWP-ASan report";
constexpr char END_TSAN_REPORT[] = "End Tsan report";
constexpr char END_CFI_REPORT[] = "End CFI report";
constexpr char END_UBSAN_REPORT[] = "End Ubsan report";
constexpr char END_HWASAN_REPORT[] = "End Hwasan report";
constexpr char END_ASAN_REPORT[] = "End Asan report";
}

const char* FindEndMarker(const char* data, size_t len, const char* marker)
{
    size_t markerLen = strlen(marker);
    if (markerLen == 0 || len < markerLen) {
        return nullptr;
    }
    const char* cur = data;
    size_t remain = len;
    while (remain >= markerLen) {
        const char* pos = static_cast<const char*>(memchr(cur, marker[0], remain - markerLen + 1));
        if (pos == nullptr) {
            return nullptr;
        }
        if (memcmp(pos, marker, markerLen) == 0) {
            return pos;
        }
        remain -= static_cast<size_t>(pos - cur) + 1;
        cur = pos + 1;
    }
    return nullptr;
}

bool IsMarkerCrossBoundary(const char* tail, size_t tailLen, const char* data, size_t len, const char* marker)
{
    size_t markerLen = strlen(marker);
    for (size_t k = 1; k < markerLen && k <= tailLen; ++k) {
        size_t need = markerLen - k;
        if (need > len) {
            continue;
        }
        if (memcmp(tail + tailLen - k, marker, k) == 0 && memcmp(data, marker + k, need) == 0) {
            return true;
        }
    }
    return false;
}

const char* DetectFaultType(const char* tail, size_t tailLen, const char* data, size_t len)
{
    if (FindEndMarker(data, len, END_GWP_ASAN_REPORT) != nullptr ||
        IsMarkerCrossBoundary(tail, tailLen, data, len, END_GWP_ASAN_REPORT)) {
        return FAULT_TYPE_GWP_ASAN;
    }
    if (FindEndMarker(data, len, END_TSAN_REPORT) != nullptr ||
        IsMarkerCrossBoundary(tail, tailLen, data, len, END_TSAN_REPORT)) {
        return FAULT_TYPE_TSAN;
    }
    if (FindEndMarker(data, len, END_CFI_REPORT) != nullptr ||
        IsMarkerCrossBoundary(tail, tailLen, data, len, END_CFI_REPORT) ||
        FindEndMarker(data, len, END_UBSAN_REPORT) != nullptr ||
        IsMarkerCrossBoundary(tail, tailLen, data, len, END_UBSAN_REPORT)) {
        return FAULT_TYPE_UBSAN;
    }
    if (FindEndMarker(data, len, END_HWASAN_REPORT) != nullptr ||
        IsMarkerCrossBoundary(tail, tailLen, data, len, END_HWASAN_REPORT)) {
        return FAULT_TYPE_HWASAN;
    }
    if (FindEndMarker(data, len, END_ASAN_REPORT) != nullptr ||
        IsMarkerCrossBoundary(tail, tailLen, data, len, END_ASAN_REPORT)) {
        return FAULT_TYPE_ASAN;
    }
    return nullptr;
}

void GetFormatedTime(uint64_t target, char* buf, size_t bufSize)
{
    if (buf == nullptr || bufSize == 0) {
        return;
    }
    uint64_t millsec = target % TIME_RATIO;
    time_t out = static_cast<time_t>(target / TIME_RATIO) + TIMEZONE_OFFSET_SECONDS;
    struct tm tmStruct {0};
    struct tm* timeInfo = gmtime_r(&out, &tmStruct);
    if (timeInfo == nullptr) {
        strncpy_s(buf, bufSize, ZERO_TIME_FORMAT, sizeof(ZERO_TIME_FORMAT) - 1);
        return;
    }
    if (bufSize < TIME_FORMAT_LEN) {
        strncpy_s(buf, bufSize, ZERO_TIME_FORMAT, sizeof(ZERO_TIME_FORMAT) - 1);
        return;
    }
    (void)strftime(buf, bufSize, "%Y%m%d%H%M%S", timeInfo);
    size_t used = strlen(buf);
    int err = snprintf_s(buf + used, bufSize - used, bufSize - used - 1, "%03llu",
        static_cast<unsigned long long>(millsec));
    if (err < 0) {
        buf[used] = '\0';
        return;
    }
}

bool ToAbsolutePath(const char* path, char* output, size_t outSize)
{
    if (path == nullptr || output == nullptr || outSize == 0) {
        return false;
    }
    if (path[0] == '/') {
        size_t len = strlen(path);
        if (len >= outSize) {
            return false;
        }
        errno_t err = memcpy_s(output, outSize, path, len + 1);
        if (err != EOK) {
            return false;
        }
        return true;
    }
    if (getcwd(output, outSize) == nullptr) {
        return false;
    }
    size_t cwdLen = strlen(output);
    size_t pathLen = strlen(path);
    if (cwdLen + 1 + pathLen + 1 >= outSize) {
        return false;
    }
    output[cwdLen] = '/';
    errno_t err = memcpy_s(output + cwdLen + 1, outSize - (cwdLen + 1), path, pathLen + 1);
    if (err != EOK) {
        return false;
    }
    return true;
}

bool IsValidInputPath(const char* path)
{
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    size_t len = strlen(path);
    if (len >= MAX_INPUT_PATH_LEN) {
        return false;
    }
    size_t i = 0;
    while (i < len) {
        while (i < len && path[i] == '/') {
            i++;
        }
        if (i >= len) {
            break;
        }
        size_t segStart = i;
        while (i < len && path[i] != '/') {
            i++;
        }
        size_t segLen = i - segStart;
        if ((segLen == CURRENT_DIR_BUF_SIZE && path[segStart] == '.') ||
           (segLen == PARENT_DIR_BUF_SIZE && path[segStart] == '.' && path[segStart + 1] == '.')) {
            return false;
        }
    }
    return true;
}

bool ContainsSymlink(const char* path)
{
    char buf[SYMLINK_CHECK_BUF_SIZE];
    size_t len = strlen(path);
    if (len == 0 || len >= sizeof(buf)) {
        return true;
    }
    errno_t err = memcpy_s(buf, sizeof(buf), path, len + 1);
    if (err != EOK) {
        return false;
    }
    size_t i = 1;
    while (i < len) {
        while (i < len && buf[i] != '/') {
            i++;
        }
        char saved = buf[i];
        buf[i] = '\0';
        struct stat st;
        bool isLast = (saved == '\0');
        bool lstatFailed = (lstat(buf, &st) != 0);
        buf[i] = saved;
        if (lstatFailed) {
            // Last component may be a log file to be created; ENOENT is allowed.
            // All non-last components must exist and must not be a symlink.
            if (isLast && errno == ENOENT) {
                return false;
            }
            return true;
        }
        if (S_ISLNK(st.st_mode)) {
            return true;
        }
        if (isLast) {
            break;
        }
        i++;
    }
    return false;
}

bool RealpathAsyncSafe(const char* path, char* resolved, size_t bufSize)
{
    if (path == nullptr || resolved == nullptr || bufSize < MIN_RESOLVED_BUF_SIZE) {
        return false;
    }
    if (!IsValidInputPath(path)) {
        return false;
    }
    char absPath[ABS_PATH_BUF_SIZE];
    if (!ToAbsolutePath(path, absPath, sizeof(absPath))) {
        return false;
    }
    if (ContainsSymlink(absPath)) {
        return false;
    }
    size_t len = strlen(absPath);
    if (len >= bufSize) {
        return false;
    }
    errno_t err = memcpy_s(resolved, bufSize, absPath, len + 1);
    if (err != EOK) {
        return false;
    }
    return true;
}

void GetNameByPid(int32_t pid, char* buf, size_t bufSize)
{
    if (bufSize == 0) {
        return;
    }
    buf[0] = '\0';
    char path[BUF_SIZE] = {0};
    int err = snprintf_s(path, sizeof(path), sizeof(path) - 1, "/proc/%d/cmdline", pid);
    if (err <= 0) {
        return;
    }
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return;
    }
    size_t i = 0;
    char chunk[BUF_SIZE] = {0};
    ssize_t bytesRead;
    while (i < (bufSize - 1)) {
        size_t want = (bufSize - 1 - i) < sizeof(chunk) ? (bufSize - 1 - i) : sizeof(chunk);
        bytesRead = read(fd, chunk, want);
        if (bytesRead <= 0) {
            break;
        }
        size_t j = 0;
        while (j < static_cast<size_t>(bytesRead) && i < bufSize - 1) {
            if (!isgraph(chunk[j])) {
                close(fd);
                buf[i] = '\0';
                return;
            }
            buf[i] = chunk[j];
            i++;
            j++;
        }
    }
    buf[i] = '\0';
    close(fd);
}