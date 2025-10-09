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
#include "test_content.h"

#include <charconv>

#include "cjson_util.h"
#include "event.h"
#include "hiview_global.h"
#include "hiview_logger.h"
#include "trace_caller.h"
#include "trace_collector.h"

namespace OHOS::HiviewDFX {
DEFINE_LOG_TAG("TraceTestPlugin");
using namespace UCollectUtil;
using namespace UCollect;

namespace {
const std::string COMMON_EVENT_TELEMETRY_START = "telemetryStart";
const std::string COMMON_EVENT_TELEMETRY_END = "telemetryEnd";
const std::string CONFIG_PATH = "/data/log/hiview/unified_collection/trace/test_data.json";
}

void TestXperfDump(const EventFwk::CommonEventData &data)
{
    std::string action = data.GetWant().GetAction();
    auto result = TraceCollector::Create()->DumpTrace(XPERF);
    if (result.data.empty()) {
        HIVIEW_LOGW("result event:%{public}s, code:%{public}d empty data file",
            action.c_str(), result.retCode);
        return;
    }
    HIVIEW_LOGI("result event:%{public}s, code:%{public}d, fileCount:%{public}zu, headFile:%{public}s",
        action.c_str(), result.retCode, result.data.size(), result.data[0].c_str());
}

void TestReliabilityDump(const EventFwk::CommonEventData &data)
{
    std::string action = data.GetWant().GetAction();
    auto result = TraceCollector::Create()->DumpTrace(RELIABILITY);
    if (result.data.empty()) {
        HIVIEW_LOGW("result event:%{public}s, code:%{public}d empty data file",
            action.c_str(), result.retCode);
        return;
    }
    HIVIEW_LOGI("result event:%{public}s, code:%{public}d, fileCount:%{public}zu, headFile:%{public}s",
        action.c_str(), result.retCode, result.data.size(), result.data[0].c_str());
}

void TestXpowerDump(const EventFwk::CommonEventData &data)
{
    std::string action = data.GetWant().GetAction();
    auto result = TraceCollector::Create()->DumpTrace(XPOWER);
    if (result.data.empty()) {
        HIVIEW_LOGW("result event:%{public}s, code:%{public}d empty data file",
            action.c_str(), result.retCode);
        return;
    }
    HIVIEW_LOGI("result event:%{public}s, code:%{public}d, fileCount:%{public}zu, headFile:%{public}s",
        action.c_str(), result.retCode, result.data.size(), result.data[0].c_str());
}

void TestXperfExDump(const EventFwk::CommonEventData &data)
{
    std::string action = data.GetWant().GetAction();
    auto result = TraceCollector::Create()->DumpTrace(XPERF_EX);
    if (result.data.empty()) {
        HIVIEW_LOGW("result event:%{public}s, code:%{public}d empty data file",
            action.c_str(), result.retCode);
        return;
    }
    HIVIEW_LOGI("result event:%{public}s, code:%{public}d, fileCount:%{public}zu, headFile:%{public}s",
        action.c_str(), result.retCode, result.data.size(), result.data[0].c_str());
}

void TestScreenDump(const EventFwk::CommonEventData &data)
{
    std::string action = data.GetWant().GetAction();
    auto result = TraceCollector::Create()->DumpTrace(SCREEN);
    if (result.data.empty()) {
        HIVIEW_LOGW("result event:%{public}s, code:%{public}d empty data file",
            action.c_str(), result.retCode);
        return;
    }
    HIVIEW_LOGI("result event:%{public}s, code:%{public}d, fileCount:%{public}zu, headFile:%{public}s",
        action.c_str(), result.retCode, result.data.size(), result.data[0].c_str());
}

std::shared_ptr<Event> MakeTelemetryEvent()
{
    cJSON* root = CJsonUtil::ParseJsonRoot(CONFIG_PATH);
    if (root == nullptr) {
        return nullptr;
    }
    if (!cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return nullptr;
    }
    auto event = std::make_shared<Event>("TelemetrySubscriber");
    if (auto telemetryId = CJsonUtil::GetStringValue(root, "telemetryId"); !telemetryId.empty()) {
        event->SetValue("telemetryId", telemetryId);
    }
    if (auto traceArgs = CJsonUtil::GetStringValue(root, "traceArgs"); !traceArgs.empty()) {
        event->SetValue("traceArgs", traceArgs);
    }
    if (auto tracePolicy = CJsonUtil::GetStringValue(root, "tracePolicy"); !tracePolicy.empty()) {
        event->SetValue("tracePolicy", tracePolicy);
    }
    if (auto traceOpenTime = CJsonUtil::GetStringValue(root, "traceOpenTime"); !traceOpenTime.empty()) {
        event->SetValue("traceOpenTime", traceOpenTime);
    }
    if (auto appFilterName = CJsonUtil::GetStringValue(root, "appFilterName"); !appFilterName.empty()) {
        event->SetValue("appFilterName", appFilterName);
    }
    if (auto saNames = CJsonUtil::GetStringValue(root, "saNames"); !saNames.empty()) {
        event->SetValue("saNames", saNames);
    }
    if (auto traceDuration = CJsonUtil::GetStringValue(root, "traceDuration"); !traceDuration.empty()) {
        event->SetValue("traceDuration", traceDuration);
    }
    if (auto traceCompressRatio = CJsonUtil::GetStringValue(root, "traceCompressRatio"); !traceCompressRatio.empty()) {
        event->SetValue("traceCompressRatio", traceCompressRatio);
    }
    if (auto xperfTraceQuota = CJsonUtil::GetStringValue(root, "xperfTraceQuota"); !xperfTraceQuota.empty()) {
        event->SetValue("xperfTraceQuota", xperfTraceQuota);
    }
    if (auto xpowerTraceQuota = CJsonUtil::GetStringValue(root, "xpowerTraceQuota"); !xpowerTraceQuota.empty()) {
        event->SetValue("xpowerTraceQuota", xpowerTraceQuota);
    }
    if (auto reliabilityTraceQuota = CJsonUtil::GetStringValue(root, "reliabilityTraceQuota");
        !reliabilityTraceQuota.empty()) {
        event->SetValue("reliabilityTraceQuota", reliabilityTraceQuota);
    }
    if (auto traceQuota = CJsonUtil::GetStringValue(root, "traceQuota"); !traceQuota.empty()) {
        event->SetValue("traceQuota", traceQuota);
    }
    return event;
}

void TestTelemetryStart(const EventFwk::CommonEventData &data)
{
    std::string action = data.GetWant().GetAction();
    auto event = MakeTelemetryEvent();
    if (event == nullptr) {
        HIVIEW_LOGI("event == nullptr check config file parse");
        return;
    }
    event->messageType_ = Event::TELEMETRY_EVENT;
    event->eventName_ = COMMON_EVENT_TELEMETRY_START;
    event->SetValue("telemetryStatus", "on");
    HiviewGlobal::GetInstance()->PostUnorderedEvent(event);
}

void TestTelemetryEnd(const EventFwk::CommonEventData &data)
{
    std::string action = data.GetWant().GetAction();
    auto event = MakeTelemetryEvent();
    if (event == nullptr) {
        HIVIEW_LOGI("event == nullptr check config file parse");
        return;
    }
    event->messageType_ = Event::TELEMETRY_EVENT;
    event->eventName_ = COMMON_EVENT_TELEMETRY_END;
    event->SetValue("telemetryStatus", "off");
    HiviewGlobal::GetInstance()->PostUnorderedEvent(event);
}

void TelemetryDumpXpower(const EventFwk::CommonEventData &data)
{
    std::string action = data.GetWant().GetAction();
    auto result = TraceCollector::Create()->DumpTraceWithFilter(TeleModule::XPOWER, 0, 0);
    HIVIEW_LOGI("result event:%{public}s, code:%{public}d", action.c_str(), result.retCode);
}

void TelemetryDumpXPerf(const EventFwk::CommonEventData &data)
{
    std::string action = data.GetWant().GetAction();
    auto result = TraceCollector::Create()->DumpTraceWithFilter(TeleModule::XPERF, 0, 0);
    HIVIEW_LOGE("result event:%{public}s, code:%{public}d", action.c_str(), result.retCode);
}

void TestTraceOnXpower(const EventFwk::CommonEventData &data)
{
    CollectResult<int32_t> result;
    std::string timeStr = data.GetData();
    if (timeStr.empty()) {
        result = TraceCollector::Create()->FilterTraceOn(TeleModule::XPOWER);
        HIVIEW_LOGE("traceOn, code:%{public}d", result.retCode);
    } else {
        int64_t time = std::stoi(timeStr);
        result = TraceCollector::Create()->FilterTraceOn(TeleModule::XPOWER, time);
        HIVIEW_LOGE("postOn delay:%{public} " PRId64 ", code:%{public}d", time, result.retCode);
    }
}

void TestTraceOffXpower(const EventFwk::CommonEventData &data)
{
    std::string action = data.GetWant().GetAction();
    auto result = TraceCollector::Create()->FilterTraceOff(TeleModule::XPOWER);
    HIVIEW_LOGE("result event:%{public}s, code:%{public}d", action.c_str(), result.retCode);
}

void TestTraceOnXPerf(const EventFwk::CommonEventData &data)
{
    CollectResult<int32_t> result;
    std::string timeStr = data.GetData();
    if (timeStr.empty()) {
        result = TraceCollector::Create()->FilterTraceOn(TeleModule::XPERF);
        HIVIEW_LOGE("traceOn, code:%{public}d", result.retCode);
    } else {
        int64_t time = 0;
        auto ret = std::from_chars(timeStr.c_str(), timeStr.c_str() + timeStr.size(), time);
        if (ret.ec != std::errc()) {
            HIVIEW_LOGE("get time error");
            return;
        }
        result = TraceCollector::Create()->FilterTraceOn(TeleModule::XPERF, time);
        HIVIEW_LOGE("postOn delay:%{public} " PRId64 ", code:%{public}d", time, result.retCode);
    }
}

void TestTraceOffXPerf(const EventFwk::CommonEventData &data)
{
    std::string action = data.GetWant().GetAction();
    auto result = TraceCollector::Create()->FilterTraceOff(TeleModule::XPERF);
    HIVIEW_LOGE("result event:%{public}s, code:%{public}d", action.c_str(), result.retCode);
}
}
