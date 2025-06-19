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
#ifndef HIVIEWDFX_HIVIEW_EVENT_LOGGER_UTIL_H
#define HIVIEWDFX_HIVIEW_EVENT_LOGGER_UTIL_H

#include <string>

#include "log_store_ex.h"
#include "faultlogger_client.h"

namespace OHOS {
namespace HiviewDFX {
void StartBootScan();
time_t GetFileLastAccessTimeStamp(const std::string& fileName);
FaultLogInfoInner ParseFaultLogInfoFromFile(const std::string &path, const std::string &fileName);
FaultLogInfoInner ExtractInfoFromFileName(const std::string& fileName);
LogStoreEx::LogFileFilter CreateLogFileFilter(int32_t id, const std::string& filePrefix);
}  // namespace HiviewDFX
}  // namespace OHOS
#endif  // HIVIEWDFX_HIVIEW_FAULTLOGGER_UTIL_H
