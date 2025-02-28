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
#include "hisysevent.h"

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
const std::string KEY_REMAIN_TIME = "remainTimeToStop";
const std::string KEY_XPERF_QUOTA = "xperfTraceQuota";
const std::string KEY_XPOWER_QUOTA = "xpowerTraceQuota";
const std::string TOATL = "Total";
const std::string KEY_FLOW_RATE = "traceCompressRatio";
const std::string KEY_TRACE_TAG = "traceArgs";
const int32_t DURATION_DEFAULT = 3600; // Seconds
const uint32_t DEFAULT_RATIO = 7;
const uint32_t BT_M_UNIT = 1024 * 1024;
constexpr char TELEMETRY_DOMAIN[] = "TELEMETRY";

// Default quota of flow control
const int64_t DEFAULT_XPERF_SIZE = 140 * 1024 * 1024;
const int64_t DEFAULT_XPOWER_SIZE = 140 * 1024 * 1024;
const int64_t DEFAULT_TOTAL_SIZE = 350 * 1024 * 1024;
}

void TelemetryListener::OnUnorderedEvent(const Event &msg)
{
    bool isCloseMsg = false;
    int64_t beginTime;
    std::string telemetryId;
    std::string errorMsg = GetValidParam(msg, isCloseMsg, beginTime, telemetryId);
    std::string bundleName = msg.GetValue(KEY_BUNDLE_NAME);
    if (errorMsg.empty()) { // no error in getting necessary params
        if (isCloseMsg) {
            SendStopEvent(msg);
            return;
        }

        // Get telemetry trace duration time
        int64_t traceDuration = msg.GetInt64Value(KEY_DURATION);
        if (traceDuration <= 0) {
            traceDuration = DURATION_DEFAULT;
        }

        // calculate telemetry trace end time
        int64_t endTime = beginTime + traceDuration;

        // In reboot scene, beginTime endTime will update to first start record
        auto ret = InitTelemetryDb(msg, beginTime, endTime);
        if (ret == TelemetryFlow::EXIT) {
            errorMsg.append("InitTelemetryDb failed;");
        } else {
            StartMsg startMsg {beginTime, endTime, telemetryId, bundleName};
            bool result = SendStartEvent(startMsg, msg, errorMsg);
            HIVIEW_LOGI("beginTime:%{public}d, endTime:%{public}d, result:%{public}d",
                static_cast<int>(beginTime), static_cast<int>(endTime), result);
        }
    }
    if (!errorMsg.empty()) {
        HiSysEventWrite(TELEMETRY_DOMAIN, "TASK_INFO", HiSysEvent::EventType::STATISTIC,
            "ID", telemetryId,
            "STAGE", "TRACE_BEGIN",
            "ERROR", errorMsg.c_str(),
            "BUNDLE_NAME", bundleName);
    }
}

std::string TelemetryListener::GetValidParam(const Event &msg, bool &isCloseMsg, int64_t &beginTime,
    std::string &telemetryId)
{
    std::string errorMsg;
    auto switchStatus = msg.GetValue(KEY_SWITCH_STATUS);
    if (switchStatus.empty()) {
        HIVIEW_LOGE("switch status msg is null");
        errorMsg.append("switchStatus empty;");
        return errorMsg;
    }
    if (switchStatus == SWITCH_OFF) {
        isCloseMsg = true;
        return errorMsg;
    }
    if (switchStatus != SWITCH_ON) {
        HIVIEW_LOGE("switch status msg error %{public}s", switchStatus.c_str());
        errorMsg.append("switchStatus param error;");
        return errorMsg;
    }
    telemetryId = msg.GetValue(KEY_ID);
    if (telemetryId.empty()) {
        HIVIEW_LOGE("init msg error, start return");
        errorMsg.append("telemetryId empty;");
        return errorMsg;
    }

    // Get begin time of telemetry trace
    beginTime = msg.GetInt64Value(KEY_OPEN_TIME);
    if (beginTime < 0) {
        HIVIEW_LOGE("get open time param fail, return fail");
        errorMsg.append("begin time get failed;");
        return errorMsg;
    } else if (beginTime == 0) {
        beginTime = TimeUtil::GetSeconds();
    }
    return errorMsg;
}

TelemetryFlow TelemetryListener::InitTelemetryDb(const Event &msg, int64_t &beginTime, int64_t &endTime)
{
    std::map<std::string, int64_t> flowControlQuotas {
        {CallerName::XPERF, DEFAULT_XPERF_SIZE },
        {CallerName::XPOWER, DEFAULT_XPOWER_SIZE},
        {TOATL, DEFAULT_TOTAL_SIZE}
    };
    int32_t traceCompressRatio = msg.GetIntValue(KEY_FLOW_RATE);
    if (traceCompressRatio <= 0) {
        traceCompressRatio = DEFAULT_RATIO;
    }
    auto xperfTraceQuota = msg.GetInt64Value(KEY_XPERF_QUOTA);
    if (xperfTraceQuota > 0) {
        flowControlQuotas[CallerName::XPERF] = xperfTraceQuota * traceCompressRatio * BT_M_UNIT;
    }
    auto xpowerTraceQuota = msg.GetInt64Value(KEY_XPOWER_QUOTA);
    if (xpowerTraceQuota > 0) {
        flowControlQuotas[CallerName::XPOWER] = xpowerTraceQuota * traceCompressRatio * BT_M_UNIT;
    }
    auto totalTraceQuota = msg.GetInt64Value(KEY_TOTAL_QUOTA);
    if (totalTraceQuota > 0) {
        flowControlQuotas[TOATL] = totalTraceQuota * traceCompressRatio * BT_M_UNIT;
    }
    HIVIEW_LOGI("Ratio:%{public}d, xperfQuota:%{public}d, xpowerQuota:%{public}d, totalQuota:%{public}d",
        static_cast<int>(traceCompressRatio), static_cast<int>(xperfTraceQuota), static_cast<int>(xpowerTraceQuota),
            static_cast<int>(totalTraceQuota));
    return TraceFlowController(BusinessName::TELEMETRY).InitTelemetryData(flowControlQuotas, beginTime, endTime);
}

bool TelemetryListener::SendStartEvent(const StartMsg &startMsg, const Event &msg, std::string &errorMsg)
{
    auto event = std::make_shared<Event>(GetListenerName());
    auto ucPlugin = myPlugin_.lock();
    if (ucPlugin == nullptr) {
        HIVIEW_LOGE("ucPlugin is null");
        return false;
    }
    std::string traceTags = msg.GetValue(KEY_TRACE_TAG);
    event->eventName_ = TelemetryEvent::TELEMETRY_START;
    event->SetValue(KEY_TRACE_TAG, traceTags);
    event->SetValue(KEY_ID, startMsg.telemetryId);
    event->SetValue(KEY_BUNDLE_NAME, startMsg.bundleName);
    auto timeNow = TimeUtil::GetSeconds();
    if (timeNow >= startMsg.endTime) {
        HIVIEW_LOGE("telemetry already end, return");
        errorMsg.append("telemetry already time end;");
        return false;
    }
    if (timeNow >= startMsg.beginTime && timeNow < startMsg.endTime) {
        HIVIEW_LOGI("immediately start now");
        int64_t duration = startMsg.endTime - timeNow;
        event->SetValue(KEY_REMAIN_TIME, static_cast<int32_t>(duration));
        ucPlugin->GetWorkLoop()->AddEvent(ucPlugin, event);
        return true;
    }
    auto timeInterval = startMsg.beginTime - timeNow;
    event->SetValue(KEY_REMAIN_TIME, static_cast<int32_t>(startMsg.endTime - startMsg.beginTime));
    HIVIEW_LOGI("send delay message timeInterval:%{public}d", static_cast<int>(timeInterval));
    myPlugin_.lock()->DelayProcessEvent(event, timeInterval);
    return true;
}

void TelemetryListener::SendStopEvent(const Event &msg)
{
    auto event = std::make_shared<Event>(GetListenerName());
    event->eventName_ = TelemetryEvent::TELEMETRY_STOP;
    event->SetValue(KEY_ID, msg.GetValue(KEY_ID));
    HIVIEW_LOGI("send stop now");
    auto ucPlugin = myPlugin_.lock();
    if (ucPlugin == nullptr) {
        HIVIEW_LOGE("ucPlugin is null");
        return;
    }
    ucPlugin->GetWorkLoop()->AddEvent(ucPlugin, event);
}
}
