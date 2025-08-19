/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#include "xperf_service_interfaces.h"
#include "xperf_service_log.h"
#include "video_jank_monitor.h"

namespace OHOS {
namespace HiviewDFX {

XperfServiceInterfaces &XperfServiceInterfaces::GetInstance()
{
    static XperfServiceInterfaces instance;
    return instance;
}

XperfServiceInterfaces::XperfServiceInterfaces()
{
}

XperfServiceInterfaces::~XperfServiceInterfaces() noexcept
{
}

void XperfServiceInterfaces::ReportSurfaceInfo(int32_t pid, std::string bundleName, int64_t uniqueId,
    std::string surfaceName)
{
    LOGI("XperfServiceInterfaces::ReportSurfaceInfo ------");
    VideoJankMonitor::GetInstance().OnSurfaceReceived(pid, bundleName, uniqueId, surfaceName);
}

}
}
