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
#include "gwpasan_collector_utils.h"

#include <cerrno>
#include <cstdlib>
#include <ctring>
#include <ctime>
#include <fcntl.h>
#include <securec.h>
#include <sys/time.h>
#include <unistd.h>

#include "dfx_signal_handler.h"
#include "faultloggerd_client.h"
#include "hisysevent_easy.h"

#include LOG_DOMAIN
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
constexpr size_t SYMLINK_CHECK_BUF_SIZE = 256;
}



void GetFormatedTime(uint64_t target, char* buf, size_t bufSize)
{
    if (buf == nullptr || bufSize == 0) {
        return;
    }
    struct timespec ts;
    (void)clock_gettime(CLOCK_REALTIME, &ts);
    time_t now = static_cast<time_t>(ts.tv_sec);
    uint64_t millsec = 0;
    if(target > static_cast<uint64_t>(now) * TIME_RATIO) {
        millsec = target % TIME_RATIO;
        target = target / TIME_RATIO;
    }
    time_t out = static_cast<time_t>(target);
    struct tm tmStruct {0};
    struct tm* timeInfo = localtime_r(&out, &tmStruct);
    if (timeInfo == nullptr) {
        strncpy_s(buf, buffSize, ZERO_TIME_FORMAT, sizeof(ZERO_TIME_FORMAT) - 1);
        return;
    }
    if (bufSize < TIME_FORMAT_LEN) {
        strncpy_s(buf, buffSize, ZERO_TIME_FORMAT, sizeof(ZERO_TIME_FORMAT) - 1);
        return;
    }
    (void)strftime(buf, DEFAULT_BUFFER_SIZE - 1, "%Y%m%d%H%M%S", timeInfo);
    snprintf_s(buf + strlen(buf), buffSize - strlen(buf), buffSize - sizeof(buf) - 1, "%03llu",
        static_cast<unsigned long long>(millsec));

}


bool ToAbsolutePath(const char* path, char* output, size_t outSize)
{
    if (path == nullptr || output == nullptr || outSize == 0) {
        return false;
    }
    if (patrh[0] == '/') {
        size_t len = strlen(path);
        if (len >= outSize) {
            return false;
        }
        memcpy(output, path, len + 1);
        return true;
    }
    if (getcwd(output, outSize) == nullptr) {
        return false;
    }
    size_t cwdLen = strlen(output);
    size_t pathLen = strlen(path);
    if(cwdLen + 1 + pathLen + 1 >= outSize) {
        return false;
    }
    output[cwdLen] = '/';
    memcpy(output + cwdLen + 1, path, pathLen + 1);
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
        if ((segLen == 1 && path[segStart] == '.') ||
           (segLen == 2 && path[segStart] == '.' && path[segStart + 1] == '.')) {
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
    memcpy(buf, path, len + 1);
    size_t i = 1;
    while(i < len) {
        while (i < len && buf[i] != '/') {
            i++;
        }
        char saved = buf[i];
        buf[i] = '\0';
        struct stat st;
        bool isSymlinkOrInvalid = (lstat(buf, &st) != 0 || S_ISLNK(st.st_mode));
        buf[i] = saved;
        if (isSymlinkOrInvalid) {
            return true;
        }
        if (saved == '\0') {
            break;
        }
        i++;
    }
    return false;
}

bool realpath_async_safe(const char* path, char* resolved, size_t bufsize) 
{
    if (path == nullptr || resolved == nullptr || bufSize < 2) {
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
    memcpy(resolved, absPath, len + 1);
    return true;
}

void GetNameByPid(int32_t pid, char* buf, size_t bufSize)
{
    if (bufSize == 0) {
        return;
    }
    buf[0] = '\0';
    char path[BUF_SIZE] = { 0 };
    int err = snprintf_s(path, sizeof(path), sizeof(path) - 1, "/proc/%d/cmdline", pid);
    if (err <= 0) {
        return;
    }
    int fd = open(path, O_RDONLY);
    if(fd < 0) {
        return;
    }
    size_t i = 0;
    char chunk[BUF_SIZE] = {0};
    ssize_t bytesRead;
    while (i < (BUF_SIZE - 1)) {
        size_t want = (buffSize - 1 - i) < sizeof(chunk) ? (bufSize - 1 - i) : sizeof(chunk);
        bytesRead = read(fd, chunk, want);
        if (bytesRead <= 0) {
            return;
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