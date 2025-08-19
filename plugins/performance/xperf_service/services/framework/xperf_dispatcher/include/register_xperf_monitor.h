/*
 * Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_HIVIEWDFX_REGISTER_XPERF_MONITOR_H
#define OHOS_HIVIEWDFX_REGISTER_XPERF_MONITOR_H

#include <map>
#include "xperf_monitor.h"

namespace OHOS {
namespace HiviewDFX {
class RegisterMonitor {
public:
    std::map<int32_t, std::vector<XperfMonitor*>> RegisterXperfMonitor();
    void RegisterMonitorByLogID(int32_t logId, XperfMonitor* monitor);
    void InitPlayStateMonitor();
    void RegisterVideoMonitor();
    XperfMonitor* MakeVideoMonitor();
private:
    std::map<int32_t, std::vector<XperfMonitor*>> dispatchers;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif