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

void GetFormatedTime(uint64_t target, char* buf, size_t bufSize);
void GetNameByPid(int32_t pid, char* buf, size_t bufSize);
bool ToAbsolutePath(const char* path, char* output, size_t outSize);
bool realpath_async_safe(const char* path, char* resolved, size_t bufsize);
#endif