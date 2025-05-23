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
#include <vector>
#include <unordered_set>

namespace OHOS::HiviewDFX {
namespace Telemetry {
const std::string KEY_ID = "telemetryId";
const std::string KEY_FILTER_NAME = "appFilterName";
const std::string KEY_SA_NAMES = "saNames";
const std::string KEY_SWITCH_STATUS = "telemetryStatus";
const std::string KEY_TRACE_POLICY = "tracePolicy";
const std::string KEY_FLOW_RATE = "traceCompressRatio";
const std::string KEY_TRACE_TAG = "traceArgs";
const std::string KEY_TOTAL_QUOTA = "traceQuota";
const std::string KEY_OPEN_TIME = "traceOpenTime";
const std::string KEY_DURATION = "traceDuration";
const std::string KEY_XPERF_QUOTA = "xperfTraceQuota";
const std::string KEY_XPOWER_QUOTA = "xpowerTraceQuota";
const std::string KEY_RELIABILITY_QUOTA = "reliabilityTraceQuota";
const std::string TOTAL = "Total";
constexpr char TELEMETRY_DOMAIN[] = "TELEMETRY";

const std::unordered_set<std::string> TRACE_TAG_FILTER_LIST {
    "sched", "freq", "disk", "sync", "binder", "mmc", "membus", "load", "pagecache", "workq", "net", "dsched",
    "graphic", "multimodalinput", "dinput", "ark", "ace", "window", "zaudio", "daudio", "zmedia", "dcamera",
    "zcamera", "dhfwk", "app", "ability", "power", "samgr", "nweb"
};

const std::unordered_set<std::string> TRACE_SA_FILTER_LIST {
    "render_service", "foundation"
};
}

class PowerListener {
public:
    virtual ~PowerListener() = default;
    virtual void OnScreenOn() = 0;
    virtual void OnScreenOff() = 0;
};
}
#endif //HIVIEWDFX_HIVIEW_UNIFIED_COMMON_H
