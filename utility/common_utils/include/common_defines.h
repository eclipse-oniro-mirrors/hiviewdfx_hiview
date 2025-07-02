/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H

#include <sys/stat.h>
#include <string>

#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
constexpr const char *UNKNOWN_STR = "unknown";
constexpr const char *PARAMETER_ENDSTACK = "END_STACK";
constexpr const char *TERMINAL_THREAD_STACK = "TERMINAL_THREAD_STACK";
constexpr const char *SMART_PARSER_PATH = "/system/etc/hiview/";
// buffer size
static const int HAS_LEN = 128;
static const int MAX_BUFFER_SIZE = 8 * 1024 * 1024; // 8:8M
static const int HAPPEN_TIME_DEFAULT = 0;

enum FingerPrintMode {
    FP_FILE = 0,
    FP_BUFFER,
    FP_ATTR
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // COMMON_DEFS_H
