/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
#ifndef FAULTLOG_FORMATTER_H
#define FAULTLOG_FORMATTER_H
#include <string>
#include "faultlog_info.h"
namespace OHOS {
namespace HiviewDFX {
namespace FaultLogger {
void WriteDfxLogToFile(int32_t fd);
void WriteFaultLogToFile(int32_t fd, int32_t logType, const std::map<std::string, std::string>& sections);
FaultLogInfo ParseCppCrashFromFile(const std::string& path);
void WriteStackTraceFromLog(int32_t fd, const std::string& pidStr, const std::string& path);
bool WriteLogToFile(int32_t fd, const std::string& path, const std::map<std::string, std::string>& sections);
bool IsFaultLogLimit();

void JumpBuildInfo(int32_t fd, std::ifstream& logFile);
}  // namespace FaultLogger
}  // namespace HiviewDFX
}  // namespace OHOS
#endif
