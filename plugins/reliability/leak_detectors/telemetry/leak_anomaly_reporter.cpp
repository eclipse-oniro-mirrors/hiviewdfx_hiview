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

// Resolves the abnormal process reporter from libleakdetectors_ext.z.so at
// runtime. The .so is a closed-source vendor part that depends on
// efficiency_manager_ext, so it must not be linked directly by this open-source
// library. When the .so or the symbol is absent, reporting degrades to a no-op.

#include "leak_anomaly_reporter.h"

#include <dlfcn.h>
#include <mutex>

#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "LeakAnomalyReporter");

namespace {
constexpr const char* const LEAKDETECTORS_EXT_SO = "libleakdetectors_ext.z.so";
constexpr const char* const REPORT_LEAK_ABNORMAL_PROCESS_SYMBOL = "ReportLeakAbnormalProcess";
constexpr int32_t REPORT_SUCCESS = 0;

using ReportLeakAbnormalProcessFunc = int32_t (*)(const char *);

ReportLeakAbnormalProcessFunc LoadReporter()
{
    static std::once_flag once;
    static ReportLeakAbnormalProcessFunc reporter = nullptr;
    std::call_once(once, []() {
        void *handle = dlopen(LEAKDETECTORS_EXT_SO, RTLD_LAZY);
        if (handle == nullptr) {
            HIVIEW_LOGI("dlopen %{public}s failed: %{public}s, anomaly report disabled",
                LEAKDETECTORS_EXT_SO, dlerror());
            return;
        }
        reporter = reinterpret_cast<ReportLeakAbnormalProcessFunc>(
            dlsym(handle, REPORT_LEAK_ABNORMAL_PROCESS_SYMBOL));
        if (reporter == nullptr) {
            HIVIEW_LOGE("dlsym %{public}s failed: %{public}s", REPORT_LEAK_ABNORMAL_PROCESS_SYMBOL, dlerror());
            dlclose(handle);
        }
    });
    return reporter;
}
} // namespace

int32_t ReportLeakAnomaly(const std::string &anomalyData)
{
    ReportLeakAbnormalProcessFunc reporter = LoadReporter();
    if (reporter == nullptr) {
        return REPORT_SUCCESS;
    }
    return reporter(anomalyData.c_str());
}

} // namespace HiviewDFX
} // namespace OHOS
