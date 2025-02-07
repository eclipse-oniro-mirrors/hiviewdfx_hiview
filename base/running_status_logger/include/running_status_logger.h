/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_HIVIEWDFX_RUNNING_STATUS_LOGGER_H
#define OHOS_HIVIEWDFX_RUNNING_STATUS_LOGGER_H

#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>

#include "singleton.h"
#include "log_file_writer.h"

namespace OHOS {
namespace HiviewDFX {
class RunningStatusLogger : public OHOS::DelayedRefSingleton<RunningStatusLogger> {
public:
    void LogRunningStatusInfo(const std::string& logInfo);
    void LogEventCountStatisticInfo(const std::string& logInfo);
    void LogEventRunningLogInfo(const std::string& logInfo);

private:
   std::shared_ptr<LogFileWriter> GetLogFileWriter(const LogStrategy& strategy);

private:
    std::mutex logMutex_;
    std::unordered_map<std::string, std::shared_ptr<LogFileWriter>> allWriters_;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // OHOS_HIVIEWDFX_RUNNING_STATUS_LOGGER_H
