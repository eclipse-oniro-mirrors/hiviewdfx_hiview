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
#include "perf_reporter.h"
#include "surface_monitor.h"
 
namespace OHOS {
namespace HiviewDFX {
 
SurfaceMonitor& SurfaceMonitor::GetInstance()
{
    static SurfaceMonitor instance;
    return instance;
}
 
void SurfaceMonitor::ReportSurface(const int64_t& uniqueId, const std::string& surfaceName,
    const std::string& componentName, const std::string& bundleName, const int32_t& pid)
{
    PerfReporter::GetInstance().ReportSurface(uniqueId, surfaceName, componentName, bundleName, pid);
}
 
}
}