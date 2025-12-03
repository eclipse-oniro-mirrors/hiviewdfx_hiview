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

#include "cjson_util.h"
#include "hiview_logger.h"
#include "time_util.h"
#include "hisysevent.h"
#include "uc_telemetry_callback.h"

namespace OHOS::HiviewDFX {
DEFINE_LOG_TAG("HiView-UnifiedCollector");
namespace {
const int64_t DURATION_DEFAULT = 3600 * SECONDS_TO_MS; // ms
const int64_t MAX_DURATION = 7 * 24 * 3600 * SECONDS_TO_MS; // ms
const uint32_t BT_M_UNIT = 1024 * 1024;
const int64_t MAX_BUFFER_SIZE = 500 * 1024; // 500M
constexpr char TELEMETRY_DOMAIN[] = "TELEMETRY";
constexpr char TAGS[] = "tags";
constexpr char BUFFER_SIZE[] = "bufferSize";

constexpr char SWITCH_ON[] = "on";
constexpr char SWITCH_OFF[] = "off";
constexpr char POLICY_POWER[] = "power";
constexpr char POLICY_MANUAL[] = "manual";

// Default quota of flow control
const int64_t DEFAULT_XPERF_SIZE = 20 * 1024 * 1024;
const int64_t DEFAULT_XPOWER_SIZE = 20 * 1024 * 1024;
const int64_t DEFAULT_RELIABILITY_SIZE = 20 * 1024 * 1024;
const int64_t DEFAULT_TOTAL_SIZE = 50 * 1024 * 1024;
const int64_t MAX_TOTAL_SIZE = 1024; // 1G
const int64_t CHECK_CYCLE_SECONDS = 30;

constexpr char KEY_ID[] = "telemetryId";
constexpr char KEY_FILTER_NAME[] = "appFilterName";
constexpr char KEY_SA_NAMES[] = "saNames";
constexpr char KEY_SWITCH_STATUS[] = "telemetryStatus";
constexpr char KEY_TRACE_POLICY[] = "tracePolicy";
constexpr char KEY_TRACE_TAG[] = "traceArgs";
constexpr char KEY_TOTAL_QUOTA[] = "traceQuota";
constexpr char KEY_OPEN_TIME[] = "traceOpenTime";
constexpr char KEY_DURATION[] = "traceDuration";
constexpr char KEY_XPERF_QUOTA[] = "xperfTraceQuota";
constexpr char KEY_XPOWER_QUOTA[] = "xpowerTraceQuota";
constexpr char KEY_RELIABILITY_QUOTA[] = "reliabilityTraceQuota";
constexpr char TOTAL[] = "Total";

const std::unordered_set<std::string> TRACE_TAG_FILTER_LIST {
    "sched", "freq", "disk", "sync", "binder", "mmc", "membus", "load", "pagecache", "workq", "net", "dsched",
    "graphic", "multimodalinput", "dinput", "ark", "ace", "window", "zaudio", "daudio", "zmedia", "dcamera",
    "zcamera", "dhfwk", "app", "ability", "power", "samgr", "nweb"
};

const std::unordered_set<std::string> TRACE_SA_FILTER_LIST {
    "render_service", "foundation"
};

std::vector<std::string> ParseAndFilterTraceArgs(const std::unordered_set<std::string> &filterList,
    cJSON* root, const std::string &key)
{
    if (!cJSON_IsObject(root)) {
        HIVIEW_LOGE("trace jsonArgs parse error");
        return {};
    }
    std::vector<std::string> traceArgs;
    CJsonUtil::GetStringArray(root, key, traceArgs);
    auto new_end = std::remove_if(traceArgs.begin(), traceArgs.end(), [&filterList](const std::string& tag) {
        return filterList.find(tag) == filterList.end();
    });
    traceArgs.erase(new_end, traceArgs.end());
    return traceArgs;
}
}

void TelemetryListener::OnUnorderedEvent(const Event &msg)
{
    bool isCloseMsg = false;
    TelemetryParams params;
    std::string errorMsg = CheckValidParam(msg, params, isCloseMsg);
    HIVIEW_LOGI("isClose:%{public}d begin time:%{public}" PRId64 "", isCloseMsg, params.beginTime);
    if (!errorMsg.empty()) {
        return WriteErrorEvent(errorMsg, params);
    }
    if (isCloseMsg) {
        HandleStop();
        return;
    }
    params.appFilterName = msg.GetValue(KEY_FILTER_NAME);
    params.traceDuration = msg.GetInt64Value(KEY_DURATION) * SECONDS_TO_MS;
    if (params.traceDuration <= 0) {
        params.traceDuration = DURATION_DEFAULT;
    } else if (params.traceDuration > MAX_DURATION) {
        params.traceDuration = MAX_DURATION;
    }
    GetSaNames(msg, params);

    bool isTimeOut = false;
    if (!InitTelemetryDbData(msg, isTimeOut, params)) {
        return WriteErrorEvent("init telemetry time table fail", params);
    }
    if (isTimeOut) {
        HIVIEW_LOGI("trace already time out");
        return;
    }
    isCanceled_ = false;
    ffrt::submit([this, params] {
        while (params.beginTime > TimeUtil::GetSeconds()) {
            if (isCanceled_) {
                return;
            }
            ffrt::this_task::sleep_for(std::chrono::seconds(CHECK_CYCLE_SECONDS));
        }
        std::unique_lock<ffrt::mutex> lock(telemetryMutex_);
        if (!isCanceled_) {
            this->HandleStart(params);
        }
    });
}

std::string TelemetryListener::CheckValidParam(const Event &msg, TelemetryParams &params, bool &isCloseMsg)
{
    std::string errorMsg;
    if (!CheckTelemetryId(msg, params, errorMsg)) {
        return errorMsg;
    }
    if (!CheckTraceTags(msg, params, errorMsg)) {
        return errorMsg;
    }
    if (!CheckTracePolicy(msg, params, errorMsg)) {
        return errorMsg;
    }
    if (!CheckSwitchValid(msg, isCloseMsg, errorMsg)) {
        return errorMsg;
    }
    if (!CheckBeginTime(msg, params, errorMsg)) {
        return errorMsg;
    }
    return errorMsg;
}

bool TelemetryListener::InitTelemetryDbData(const Event &msg, bool &isTimeOut, const TelemetryParams &params)
{
    std::map<std::string, int64_t> flowControlQuotas {
        {CallerName::XPERF, DEFAULT_XPERF_SIZE },
        {CallerName::XPOWER, DEFAULT_XPOWER_SIZE},
        {CallerName::RELIABILITY, DEFAULT_RELIABILITY_SIZE},
        {TOTAL, DEFAULT_TOTAL_SIZE}
    };
    auto xperfTraceQuota = msg.GetInt64Value(KEY_XPERF_QUOTA);
    if (xperfTraceQuota > 0) {
        flowControlQuotas[CallerName::XPERF] = xperfTraceQuota * BT_M_UNIT;
    }
    auto xpowerTraceQuota = msg.GetInt64Value(KEY_XPOWER_QUOTA);
    if (xpowerTraceQuota > 0) {
        flowControlQuotas[CallerName::XPOWER] = xpowerTraceQuota * BT_M_UNIT;
    }
    auto reliabilityTraceQuota = msg.GetInt64Value(KEY_RELIABILITY_QUOTA);
    if (reliabilityTraceQuota > 0) {
        flowControlQuotas[CallerName::RELIABILITY] = reliabilityTraceQuota * BT_M_UNIT;
    }
    auto totalTraceQuota = msg.GetInt64Value(KEY_TOTAL_QUOTA);
    if (totalTraceQuota > 0 && totalTraceQuota <= MAX_TOTAL_SIZE) {
        flowControlQuotas[TOTAL] = totalTraceQuota * BT_M_UNIT;
    } else if (totalTraceQuota > MAX_TOTAL_SIZE) {
        flowControlQuotas[TOTAL] = MAX_TOTAL_SIZE * BT_M_UNIT;
    } else {
        HIVIEW_LOGI("default total quota size");
    }

    int64_t running_time = 0;
    auto ret = TraceFlowController(FlowControlName::TELEMETRY).InitTelemetryData(params.telemetryId, running_time,
        flowControlQuotas);
    if (ret == TelemetryRet::EXIT) {
        return false;
    }
    isTimeOut = running_time >= params.traceDuration;
    return true;
}

void TelemetryListener::HandleStart(const TelemetryParams &params)
{
    std::vector<std::string> traceTags = {"ace", "app", "ark", "binder", "freq", "graphic", "multimodalinput", "nweb",
        "sched", "window"};
    uint32_t bufferSize = 0;
    if (!params.traceTag.empty()) {
        traceTags = params.traceTag;
    }
    if (params.bufferSize > 0) {
        bufferSize = params.bufferSize;
    }
    ScenarioInfo telemetryScenario {
        .scenario = TraceScenario::TRACE_TELEMETRY,
        .args = {
            .tags = std::move(traceTags),
            .bufferSize = bufferSize,
        },
        .tracePolicy = params.tracePolicy
    };
    auto ret = TraceStateMachine::GetInstance().OpenTrace(telemetryScenario);
    if (ret.IsSuccess()) {
        std::shared_ptr<TelemetryCallback> callback;
        switch (params.tracePolicy) {
            case TelemetryPolicy::POWER:
                callback = std::make_shared<PowerCallback>(params);
                break;
            case TelemetryPolicy::MANUAL:
                callback = std::make_shared<ManualCallback>(params);
                break;
            default:
                callback = std::make_shared<UcTelemetryCallback>(params);
                break;
        }
        bool isSuccess = TraceStateMachine::GetInstance().RegisterTelemetryCallback(callback);
        HIVIEW_LOGI("register callback result:%{public}d, traceDuration:%{public}" PRId64 "", isSuccess,
            params.traceDuration);
    } else {
        WriteErrorEvent("trace state error", params);
    }
}

void TelemetryListener::HandleStop()
{
    std::unique_lock<ffrt::mutex> lock(telemetryMutex_);
    TraceStateMachine::GetInstance().CloseTrace(TraceScenario::TRACE_TELEMETRY);
    TraceFlowController controller(FlowControlName::TELEMETRY);
    controller.ClearTelemetryData();
    isCanceled_ = true;
}

void TelemetryListener::WriteErrorEvent(const std::string &error, const TelemetryParams &params)
{
    HIVIEW_LOGE("%{public}s", error.c_str());
    HiSysEventWrite(TELEMETRY_DOMAIN, "TASK_INFO", HiSysEvent::EventType::STATISTIC,
        "ID", params.telemetryId,
        "STAGE", "TRACE_BEGIN",
        "ERROR", error);
}

bool TelemetryListener::ProcessTraceTag(const std::string &traceTag, std::vector<std::string> &traceTags,
    uint32_t &bufferSize)
{
    cJSON* root = cJSON_Parse(traceTag.c_str());
    if (root == nullptr) {
        return false;
    }
    traceTags = ParseAndFilterTraceArgs(TRACE_TAG_FILTER_LIST, root, TAGS);
    if (traceTags.empty()) {
        cJSON_Delete(root);
        return false;
    }
    bufferSize = static_cast<uint32_t>(CJsonUtil::GetIntValue(root, BUFFER_SIZE));
    cJSON_Delete(root);
    if (bufferSize <= 0) {
        HIVIEW_LOGE("jsonArgs parse trace bufferSize error");
        return false;
    }
    if (bufferSize > MAX_BUFFER_SIZE) {
        bufferSize = MAX_BUFFER_SIZE;
    }
    return true;
}

bool TelemetryListener::CheckTelemetryId(const Event &msg, TelemetryParams &params, std::string &errorMsg)
{
    std::string telemetryId = msg.GetValue(KEY_ID);
    if (telemetryId.empty()) {
        errorMsg.append("telemetryId get empty");
        return false;
    }
    params.telemetryId = telemetryId;
    return true;
}

void TelemetryListener::GetSaNames(const Event &msg, TelemetryParams &params)
{
    std::string saJsonNames = msg.GetValue(KEY_SA_NAMES);
    if (!saJsonNames.empty()) {
        cJSON* root = cJSON_Parse(saJsonNames.c_str());
        if (root == nullptr) {
            return;
        }
        auto saNames = ParseAndFilterTraceArgs(TRACE_SA_FILTER_LIST, root, KEY_SA_NAMES);
        for (const auto &saName : saNames) {
            auto param = "startup.service.ctl." + saName + ".pid";
            params.saParams.emplace_back(param);
        }
        cJSON_Delete(root);
    }
}

bool TelemetryListener::CheckTraceTags(const Event &msg, TelemetryParams &params, std::string &errorMsg)
{
    auto traceTagStr = msg.GetValue(KEY_TRACE_TAG);
    std::vector<std::string> traceTags;
    uint32_t bufferSize = 0;
    if (!traceTagStr.empty() && !ProcessTraceTag(traceTagStr, traceTags, bufferSize)) {
        errorMsg.append("process trace tag fail");
        return false;
    }
    params.traceTag = traceTags;
    params.bufferSize = bufferSize;
    return true;
}

bool TelemetryListener::CheckTracePolicy(const Event &msg, TelemetryParams &params, std::string &errorMsg)
{
    auto tracePolicy = msg.GetValue(KEY_TRACE_POLICY);
    if (tracePolicy.empty()) {
        params.tracePolicy = TelemetryPolicy::DEFAULT;
        return true;
    }
    if (tracePolicy == POLICY_POWER) {
        params.tracePolicy = TelemetryPolicy::POWER;
    } else if (tracePolicy == POLICY_MANUAL) {
        params.tracePolicy = TelemetryPolicy::MANUAL;
    } else {
        errorMsg.append("trace policy get empty");
        return false;
    }
    return true;
}

bool TelemetryListener::CheckSwitchValid(const Event &msg, bool &isCloseMsg, std::string &errorMsg)
{
    auto switchStatus = msg.GetValue(KEY_SWITCH_STATUS);
    if (switchStatus.empty()) {
        errorMsg.append("switchStatus get empty");
        return false;
    }
    if (switchStatus == SWITCH_OFF) {
        isCloseMsg = true;
        return false;
    }
    if (switchStatus != SWITCH_ON) {
        errorMsg.append("switchStatus param get error");
        return false;
    }
    return true;
}

bool TelemetryListener::CheckBeginTime(const Event &msg, TelemetryParams &params, std::string &errorMsg)
{
    // Get begin time of telemetry trace unit seconds
    int64_t beginTime = msg.GetInt64Value(KEY_OPEN_TIME);
    if (beginTime < 0) {
        errorMsg.append("begin time get failed");
        return false;
    }
    params.beginTime = beginTime;
    return true;
}
}
