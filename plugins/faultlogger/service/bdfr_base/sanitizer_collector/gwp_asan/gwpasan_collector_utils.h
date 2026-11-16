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
#ifndef GWPASAN_COLLECTOR_UTILS_H
#define GWPASAN_COLLECTOR_UTILS_H

#include <cstdint>
#include <cstddef>
#include <sys/stat.h>

constexpr size_t MAX_END_MARKER_LEN = 32;
constexpr char FAULT_TYPE_GWP_ASAN[] = "GWP-ASAN";
constexpr char FAULT_TYPE_TSAN[] = "TSAN";
constexpr char FAULT_TYPE_UBSAN[] = "UBSAN";
constexpr char FAULT_TYPE_HWASAN[] = "HWASAN";
constexpr char FAULT_TYPE_ASAN[] = "ASAN";

void GetFormatedTime(uint64_t target, char* buf, size_t bufSize);
void GetNameByPid(int32_t pid, char* buf, size_t bufSize);
bool ToAbsolutePath(const char* path, char* output, size_t outSize);
bool RealpathAsyncSafe(const char* path, char* resolved, size_t bufsize);
const char* DetectFaultType(const char* tail, size_t tailLen, const char* data, size_t len);
#endif