/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "uc_telemetry_listener.h"

#include "hiview_logger.h"
#include "time_util.h"
#include "trace_utils.h"

namespace OHOS::HiviewDFX {
DEFINE_LOG_TAG("HiView-UnifiedCollector");
namespace {
const std::string KEY_ID = "telemetryId";
const std::string KEY_SWITCH_STATUS = "telemetryStatus";
const std::string SWITCH_ON = "on";
const std::string SWITCH_OFF = "off";
const std::string KEY_BUNDLE_NAME = "bundleName";
const std::string KEY_TOTAL_QUOTA = "traceQuota";
const std::string KEY_OPEN_TIME = "traceOpenTime";
const std::string KEY_DURATION = "traceDuration";
const std::string KEY_XPERF_QUOTA = "xperfTraceQuota";
const std::string KEY_XPOWER_QUOTA = "xpowerTraceQuota";
const std::string TOATL = "Total";
const std::string KEY_FLOW_RATE = "traceCompressRatio";
const std::string KEY_TRACE_TAG = "traceArgs";
const uint32_t DEFAULT_RATIO = 7;
const uint32_t BT_UNIT = 1024 * 1024;

// Default quota of flow control
const int64_t DEFAULT_XPERF_SIZE = 140 * 1024 * 1024;
const int64_t DEFAULT_XPOWER_SIZE = 140 * 1024 * 1024;
const int64_t DEFAULT_TOTAL_SIZE = 350 * 1024 * 1024;
}

void TelemetryListener::OnUnorderedEvent(const Event &msg)
{
    auto switchStatus = msg.GetValue(KEY_SWITCH_STATUS);
    if (switchStatus.empty()) {
        HIVIEW_LOGE("switch status msg is null");
        return;
    }
    if (switchStatus == SWITCH_OFF) {
        SendStopEvent();
        return;
    }
    if (switchStatus != SWITCH_ON) {
        HIVIEW_LOGE("switch status msg error");
        return;
    }
    if (auto openTime = msg.GetInt64Value(KEY_OPEN_TIME); openTime < 0) {
        HIVIEW_LOGE("get open time param fail, return fail");
        return;
    } else {
        openTime_ = static_cast<uint64_t>(openTime);
    }
    if (int32_t traceDuration = msg.GetIntValue(KEY_DURATION); traceDuration > 0) {
        traceDuration_ = traceDuration;
    }
    traceTags_ = msg.GetValue(KEY_TRACE_TAG);
    auto ret = InitFlowControlQuotas(msg);
    if (ret == TelemetryFlow::EXIT) {
        return;
    }
    SendStartEvent();
    HIVIEW_LOGD("openTime:%{public}d, traceDuration:%{public}d, traceTags:%{public}s, switchStatus:%{public}s",
        static_cast<int>(openTime_), static_cast<int>(traceDuration_), traceTags_.c_str(), switchStatus.c_str());
}

TelemetryFlow TelemetryListener::InitFlowControlQuotas(const Event &msg)
{
    std::map<std::string, int64_t> flowControlQuotas {
        {CallerName::XPERF, DEFAULT_XPERF_SIZE },
        {CallerName::XPOWER, DEFAULT_XPOWER_SIZE},
        {TOATL, DEFAULT_TOTAL_SIZE}
    };
    int traceCompressRatio = msg.GetIntValue(KEY_FLOW_RATE);
    if (traceCompressRatio <= 0) {
        traceCompressRatio = DEFAULT_RATIO;
    }
    auto xperfTraceQuota = msg.GetInt64Value(KEY_XPERF_QUOTA);
    if (xperfTraceQuota > 0) {
        flowControlQuotas[CallerName::XPERF] = xperfTraceQuota * traceCompressRatio * BT_UNIT;
    }
    auto xpowerTraceQuota = msg.GetInt64Value(KEY_XPOWER_QUOTA);
    if (xpowerTraceQuota > 0) {
        flowControlQuotas[CallerName::XPOWER] = xpowerTraceQuota * traceCompressRatio * BT_UNIT;
    }
    auto totalTraceQuota = msg.GetInt64Value(KEY_TOTAL_QUOTA);
    if (totalTraceQuota > 0) {
        flowControlQuotas[TOATL] = totalTraceQuota * traceCompressRatio * BT_UNIT;
    }
    HIVIEW_LOGD("Ratio:%{public}d, xperfQuota:%{public}d, xpowerQuota:%{public}d, totalQuota:%{public}d",
        static_cast<int>(traceCompressRatio), static_cast<int>(xperfTraceQuota), static_cast<int>(xpowerTraceQuota),
            static_cast<int>(totalTraceQuota));
    return TraceFlowController(BusinessName::TELEMETRY).InitTelemetryData(flowControlQuotas);
}

void TelemetryListener::SendStartEvent()
{
    auto event = std::make_shared<Event>(GetListenerName());
    auto ucPlugin = myPlugin_.lock();
    if (ucPlugin == nullptr) {
        HIVIEW_LOGE("ucPlugin is null");
        return;
    }
    event->eventName_ = TelemetryEvent::TELEMETRY_START;
    event->SetValue(KEY_TRACE_TAG, traceTags_);
    event->SetValue(KEY_DURATION, traceDuration_);
    auto timeNow = TimeUtil::GetSeconds();
    if (openTime_ == 0 || timeNow >= openTime_) {
        HIVIEW_LOGI("immediately start now");
        ucPlugin->GetWorkLoop()->AddEvent(ucPlugin, event);
        return;
    }
    auto timeInterval = openTime_ - timeNow;
    HIVIEW_LOGI("send delay message timeInterval:%d", static_cast<int>(timeInterval));
    myPlugin_.lock()->DelayProcessEvent(event, timeInterval);
}

void TelemetryListener::SendStopEvent()
{
    auto event = std::make_shared<Event>(GetListenerName());
    event->eventName_ = TelemetryEvent::TELEMETRY_STOP;
    HIVIEW_LOGI("send stop now");
    auto ucPlugin = myPlugin_.lock();
    if (ucPlugin == nullptr) {
        HIVIEW_LOGE("ucPlugin is null");
        return;
    }
    ucPlugin->GetWorkLoop()->AddEvent(ucPlugin, event);
}
}
