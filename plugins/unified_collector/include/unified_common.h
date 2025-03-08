/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
#ifndef HIVIEWDFX_HIVIEW_UNIFIED_COMMON_H
#define HIVIEWDFX_HIVIEW_UNIFIED_COMMON_H
#include <string>

namespace OHOS::HiviewDFX {
namespace Telemetry {
    const std::string KEY_TELEMETRY_TRACE_TAG = "traceArgs";
    const std::string KEY_REMAIN_TIME = "remainTimeToStop";
    const std::string KEY_ID = "telemetryId";
    const std::string KEY_BUNDLE_NAME = "bundleName";
    const std::string KEY_SWITCH_STATUS = "telemetryStatus";
    const std::string SWITCH_ON = "on";
    const std::string SWITCH_OFF = "off";
    const std::string KEY_TOTAL_QUOTA = "traceQuota";
    const std::string KEY_OPEN_TIME = "traceOpenTime";
    const std::string KEY_DURATION = "traceDuration";
    const std::string KEY_DELAY_TIME = "delayTime";
    const std::string KEY_XPERF_QUOTA = "xperfTraceQuota";
    const std::string KEY_XPOWER_QUOTA = "xpowerTraceQuota";
    const std::string TOATL = "Total";
    const std::string KEY_FLOW_RATE = "traceCompressRatio";
    const std::string KEY_TRACE_TAG = "traceArgs";
    constexpr char TELEMETRY_DOMAIN[] = "TELEMETRY";
}
}
#endif //HIVIEWDFX_HIVIEW_UNIFIED_COMMON_H
