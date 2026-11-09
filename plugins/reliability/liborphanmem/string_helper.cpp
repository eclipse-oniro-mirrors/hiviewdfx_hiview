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

#include "string_helper.h"

#include <cinttypes>
#include <cstring>

#include "log.h"

namespace OHOS {
bool StringHelper::StartsWith(const MAP_ALLOCATOR::String &str, const char *prefix)
{
    if (strlen(prefix) > str.size()) {
        return false;
    }
    return str.compare(0, strlen(prefix), prefix) == 0;
}

bool StringHelper::EndsWith(const MAP_ALLOCATOR::String &s, const char *suffix)
{
    size_t suffixLen = strlen(suffix);
    size_t sLen = s.length();
    if (suffixLen > sLen) {
        return false;
    }

    return s.compare(sLen - suffixLen, suffixLen, suffix) == 0;
}

bool StringHelper::IsSanitizerMap(const MAP_ALLOCATOR::String &s)
{
    return s == "[anon:low shadow]" || s == "[anon:high shadow]" || StartsWith(s, "[anon:hwasan");
}

char *StringHelper::PidToStr(char *buf, size_t len, pid_t pid)
{
    if (pid <= 0) {
        HILOGE("invalid pid: %{public}d", pid);
        return nullptr;
    }

    char *ptr = buf + len - 1;
    *ptr = '\0';
    while (pid > 0) {
        ptr--;
        if (ptr < buf) {
            HILOGE("buf too small, size: %{public}zu", len);
            return nullptr;
        }
        *ptr = '0' + (pid % DECIMAL_BASE);
        pid /= DECIMAL_BASE;
    }

    return ptr;
}

bool StringHelper::ConcatenatePath(char *prefix, pid_t pid, const char *suffix)
{
    char pidBuf[PID_LENGTH] = {0};
    char *pidStr = PidToStr(pidBuf, sizeof(pidBuf), pid);
    if (pidStr == nullptr) {
        HILOGE("pid2str failed, pid: %{public}d", pid);
        return false;
    }
    size_t ret = strlcat(prefix, pidStr, PROC_LENGTH);
    if (ret >= PROC_LENGTH) {
        HILOGE("strlcat pid failed, pid: %{public}d", pid);
        return false;
    }
    ret = strlcat(prefix, suffix, PROC_LENGTH);
    if (ret >= PROC_LENGTH) {
        HILOGE("strlcat suffix failed, suffix: %{public}s", suffix);
        return false;
    }
    return true;
}
}
