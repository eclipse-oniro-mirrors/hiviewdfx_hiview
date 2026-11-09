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

#ifndef LIBORPHANMEM_STRING_HELPER_H
#define LIBORPHANMEM_STRING_HELPER_H

#include "map_allocator.h"

namespace OHOS {
inline constexpr size_t PID_LENGTH = 8;
inline constexpr size_t PROC_LENGTH = 128;
inline constexpr int DECIMAL_BASE = 10;

class StringHelper {
public:
    StringHelper(){};
    ~StringHelper(){};
    bool StartsWith(const MAP_ALLOCATOR::String &str, const char *prefix);
    bool EndsWith(const MAP_ALLOCATOR::String &str, const char *suffix);
    bool IsSanitizerMap(const MAP_ALLOCATOR::String &str);
    bool ConcatenatePath(char *prefix, pid_t pid, const char *suffix);
    char *PidToStr(char *buf, size_t len, pid_t pid);
};
}

#endif  // LIBORPHANMEM_STRING_HELPER_H
