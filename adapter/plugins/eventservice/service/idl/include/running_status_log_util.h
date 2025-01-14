/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_HIVIEWDFX_RUNNING_STATUS_LOG_UTIL
#define OHOS_HIVIEWDFX_RUNNING_STATUS_LOG_UTIL

#include <ctime>

#include "sys_event_query_rule.h"
#include "sys_event_rule.h"

namespace OHOS {
namespace HiviewDFX {
class RunningStatusLogUtil {
public:
    static void LogTooManyWatchRules(const std::vector<SysEventRule>& rules);
    static void LogTooManyWatchers(const int limit);
    static void LogTooManyQueryRules(const std::vector<SysEventQueryRule>& rules);
    static void LogTooManyEvents(const int limit);
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // OHOS_HIVIEWDFX_RUNNING_STATUS_LOG_UTIL