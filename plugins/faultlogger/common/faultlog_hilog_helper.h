/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef FAULTLOG_HILOG_HELPER_H
#define FAULTLOG_HILOG_HELPER_H

#include <string>

#include "json/json.h"

namespace OHOS {
namespace HiviewDFX {
namespace FaultlogHilogHelper {
std::string GetHilogByPid(int32_t pid);
int DoGetHilogProcess(int32_t pid, int writeFd);
std::string ReadHilogTimeout(int fd);
Json::Value ParseHilogToJson(const std::string &hilogStr);
}
} // namespace HiviewDFX
} // namespace OHOS
#endif
