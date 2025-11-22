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

#ifndef OHOS_HIVIEWDFX_LISTENER_STATUS_UTIL
#define OHOS_HIVIEWDFX_LISTENER_STATUS_UTIL

#include <string>
#include <vector>

#include "listener_status_monitor.h"
#include "sys_event_rule.h"

namespace OHOS {
namespace HiviewDFX {
class ListenerStatusUtil {
public:
    static ListenerCallerInfo GetListenerCallerInfo(const std::vector<SysEventRule>& rules = {});
    static ListenerCallerInfo GetListenerCallerInfo(int32_t uid, const std::string& name,
        const std::vector<SysEventRule>& rules = {});
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // OHOS_HIVIEWDFX_LISTENER_STATUS_UTIL
