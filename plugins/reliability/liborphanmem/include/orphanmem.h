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

#ifndef LIBORPHANMEM_ORPHANMEM_H
#define LIBORPHANMEM_ORPHANMEM_H

#include <string>

enum ErrorCode {
    EC_SUCCESS = 0,
    EC_FAILED = -1,
    EC_LD_PRELOAD = -2,
};

namespace OHOS {
enum MemLeakFormat {
    TYPE_TXT = 0,
    TYPE_VECTOR,
    TYPE_SNAPSHOT,
};
bool GetMemLeakString(int format, std::string &leakStr, size_t limit = SIZE_MAX);
int64_t GetMemLeak(int format, std::string &leakStr, size_t limit = SIZE_MAX);
void BlockSignal(int signal);
void UnblockSignal(int signal);
}  // namespace OHOS

extern "C" {
bool OHOS_GetMemLeakString(int format, char **output, size_t limit = SIZE_MAX, int *length = nullptr);
const char* OHOS_GetErrorMessage(int64_t errCode);
int64_t OHOS_GetMemLeak(int format, char **output, size_t limit = SIZE_MAX, int *length = nullptr);
}
#endif  // LIBORPHANMEM_ORPHANMEM_H
