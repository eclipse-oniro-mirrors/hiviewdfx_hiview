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
#include "unified_common.h"

namespace OHOS::HiviewDFX {
DEFINE_LOG_TAG("HiView-UnifiedCollector");
namespace {

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
    std::string errorMsg = GetValidParam(msg, isCloseMsg);
    if (!errorMsg.empty()) {
        return WriteErrorEvent(errorMsg);
    }
    if (isCloseMsg) {
        SendStopEvent();
        return;
    }
    if (!InitAndCorrectTimes(msg)) {
        return WriteErrorEvent("init telemetry time table fail");
    }
    auto timeNow = TimeUtil::GetSeconds();
    if (timeNow >= endTime_) {
        return WriteErrorEvent("telemetry trace already time out");
    } else if (timeNow > beginTime_ && timeNow < endTime_) {
        SendStartEvent(msg,  endTime_ - timeNow);
    } else {
        SendStartEvent(msg, endTime_ - beginTime_, beginTime_ - timeNow);
    }
}

std::string TelemetryListener::GetValidParam(const Event &msg, bool &isCloseMsg)
{
    std::string errorMsg;
    auto switchStatus = msg.GetValue(Telemetry::KEY_SWITCH_STATUS);
    if (switchStatus.empty()) {
        errorMsg.append("switchStatus get empty");
        return errorMsg;
    }
    if (switchStatus == Telemetry::SWITCH_OFF) {
        isCloseMsg = true;
        return errorMsg;
    }
    if (switchStatus != Telemetry::SWITCH_ON) {
        errorMsg.append("switchStatus param get error");
        return errorMsg;
    }
    std::string telemetryId = msg.GetValue(Telemetry::KEY_ID);
    if (telemetryId.empty()) {
        errorMsg.append("telemetryId get empty");
        return errorMsg;
    }

    // Get begin time of telemetry trace
    int64_t beginTime = msg.GetInt64Value(Telemetry::KEY_OPEN_TIME);
    if (beginTime < 0) {
        errorMsg.append("begin time get failed");
        return errorMsg;
    } else if (beginTime == 0) {
        beginTime = TimeUtil::GetSeconds();
    }
    beginTime_ = beginTime;
    telemetryId_ = telemetryId;
    bundleName_ = msg.GetValue(Telemetry::KEY_BUNDLE_NAME);
    return errorMsg;
}

bool TelemetryListener::InitAndCorrectTimes(const Event &msg)
{
    std::map<std::string, int64_t> flowControlQuotas {
        {CallerName::XPERF, DEFAULT_XPERF_SIZE },
        {CallerName::XPOWER, DEFAULT_XPOWER_SIZE},
        {Telemetry::TOTAL, DEFAULT_TOTAL_SIZE}
    };
    int32_t traceCompressRatio = msg.GetIntValue(Telemetry::KEY_FLOW_RATE);
    if (traceCompressRatio <= 0) {
        traceCompressRatio = DEFAULT_RATIO;
    }
    auto xperfTraceQuota = msg.GetInt64Value(Telemetry::KEY_XPERF_QUOTA);
    if (xperfTraceQuota > 0) {
        flowControlQuotas[CallerName::XPERF] = xperfTraceQuota * traceCompressRatio * BT_M_UNIT;
    }
    auto xpowerTraceQuota = msg.GetInt64Value(Telemetry::KEY_XPOWER_QUOTA);
    if (xpowerTraceQuota > 0) {
        flowControlQuotas[CallerName::XPOWER] = xpowerTraceQuota * traceCompressRatio * BT_M_UNIT;
    }
    auto totalTraceQuota = msg.GetInt64Value(Telemetry::KEY_TOTAL_QUOTA);
    if (totalTraceQuota > 0) {
        flowControlQuotas[Telemetry::TOTAL] = totalTraceQuota * traceCompressRatio * BT_M_UNIT;
    }

    // In reboot scene, beginTime endTime will update to first start record
    auto ret = TraceFlowController(BusinessName::TELEMETRY).InitTelemetryData(telemetryId_, beginTime_,
        flowControlQuotas);
    if (ret == TelemetryRet::EXIT) {
        return false;
    }
    int64_t traceDuration = msg.GetInt64Value(Telemetry::KEY_DURATION);
    if (traceDuration <= 0) {
        traceDuration = DURATION_DEFAULT;
    }
    endTime_ = beginTime_ + traceDuration;
    return true;
}

bool TelemetryListener::SendStartEvent(const Event &msg, int64_t traceDuration, int64_t delayTime)
{
    auto event = std::make_shared<Event>(GetListenerName());
    auto ucPlugin = myPlugin_.lock();
    if (ucPlugin == nullptr) {
        HIVIEW_LOGE("ucPlugin is null");
        return false;
    }
    std::string traceArgs = msg.GetValue(Telemetry::KEY_TRACE_TAG);
    if (!traceArgs.empty() && !ParseAndFilterTraceArgs(Telemetry::TRACE_TAG_FILTER_LIST, traceArgs)) {
        HIVIEW_LOGE("traceTag error");
        return false;
    }
    event->eventName_ = TelemetryEvent::TELEMETRY_START;
    event->SetValue(Telemetry::KEY_TRACE_TAG, traceArgs);
    event->SetValue(Telemetry::KEY_ID, telemetryId_);
    event->SetValue(Telemetry::KEY_BUNDLE_NAME, bundleName_);
    event->SetValue(Telemetry::KEY_REMAIN_TIME, static_cast<int32_t>(traceDuration));
    if (delayTime > 0) {
        event->SetValue(Telemetry::KEY_DELAY_TIME, static_cast<int32_t>(delayTime));
    }
    ucPlugin->GetWorkLoop()->AddEvent(ucPlugin, event);
    return true;
}

void TelemetryListener::SendStopEvent()
{
    auto event = std::make_shared<Event>(GetListenerName());
    event->eventName_ = TelemetryEvent::TELEMETRY_STOP;
    event->SetValue(Telemetry::KEY_ID, telemetryId_);
    HIVIEW_LOGI("send stop now");
    auto ucPlugin = myPlugin_.lock();
    if (ucPlugin == nullptr) {
        HIVIEW_LOGE("ucPlugin is null");
        return;
    }
    ucPlugin->GetWorkLoop()->AddEvent(ucPlugin, event);
}

void TelemetryListener::WriteErrorEvent(const std::string &error)
{
    HIVIEW_LOGE("%{public}s", error.c_str());
    HiSysEventWrite(TELEMETRY_DOMAIN, "TASK_INFO", HiSysEvent::EventType::STATISTIC,
        "ID", telemetryId_,
        "STAGE", "TRACE_BEGIN",
        "ERROR", error,
        "BUNDLE_NAME", bundleName_);
}

}
