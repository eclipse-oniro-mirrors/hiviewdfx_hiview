/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
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
#include "sanitizer_telemetry.h"

#include <parameters.h>
#include "hilog/log.h"
#include "hisysevent.h"

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002D12

#undef LOG_TAG
#define LOG_TAG "SanitizerTelemetry"

namespace {
    constexpr const char* const TELEMETRY_KEY_ID = "telemetryId";
    constexpr const char* const TELEMETRY_KEY_FAULT = "fault";
    constexpr const char* const TELEMETRY_KEY_STATUS = "telemetryStatus";
    constexpr const char* const TELEMETRY_KEY_BUNDLE_NAME = "bundleName";
    constexpr const char* const TELEMETRY_ON = "on";
    constexpr const char* const TELEMETRY_OFF = "off";
    constexpr const char* const TELEMETRY_KEY_GWP_ENABLE = "gwpEnable";
    constexpr const char* const TELEMETRY_KEY_GWP_SAMPLE_RATE = "gwpSampleRate";
    constexpr const char* const TELEMETRY_KEY_GWP_MAX_SLOTS = "gwpMaxSlots";
    constexpr const char* const TELEMETRY_KEY_MIN_SAMPLE_SIZE = "minSampleSize";
    constexpr const char* const TELEMETRY_KEY_STACK_PARAM = "stackParam";
    constexpr const char* const PARAM_GWP_ASAN_ENABLE = "gwp_asan.enable.app.";
    constexpr const char* const PARAM_GWP_ASAN_SAMPLE = "gwp_asan.sample.app.";
    constexpr const char* const PARAM_GWP_ASAN_MIN_SIZE = "gwp_asan.app.min_size.";
    constexpr const char* const PARAM_GWP_ASAN_LIBRARY = "gwp_asan.app.library.";
    constexpr uint32_t TELEMETRY_SANITIZER_TYPE = 0x100;
    constexpr int DECIMAL_BASE = 10;
}

namespace OHOS {
namespace HiviewDFX {

void SanitizerTelemetry::OnUnorderedEvent(const Event& msg)
{
    if (msg.messageType_ != Event::TELEMETRY_EVENT) {
        HILOG_INFO(LOG_CORE, "only accept TELEMETRY_EVENT, current type: %{public}d", msg.messageType_);
        return;
    }
    std::string telemetryStatus = msg.GetValue(TELEMETRY_KEY_STATUS);
    if (telemetryStatus == TELEMETRY_OFF) {
        std::string bundleName = msg.GetValue(TELEMETRY_KEY_BUNDLE_NAME);
        ClearTelemetryParam(bundleName);
    } else if (telemetryStatus == TELEMETRY_ON) {
        HandleTelemetryStart(msg);
    } else {
        HILOG_WARN(LOG_CORE, "wrong telemetryStatus: %{public}s", telemetryStatus.c_str());
    }
}

void SanitizerTelemetry::HandleTelemetryStart(const Event& msg)
{
    GwpTelemetryEvent gwpTelemetryEvent;
    auto valuePairs = msg.GetKeyValuePairs();
    if (valuePairs.count(TELEMETRY_KEY_FAULT) == 0) {
        HILOG_ERROR(LOG_CORE, "no fault");
        return;
    }
    long long tmp;
    if (!SafeStoll(valuePairs[TELEMETRY_KEY_FAULT], tmp)) {
        HILOG_ERROR(LOG_CORE, "failed to SafeStoll faultStr: %{public}s", valuePairs[TELEMETRY_KEY_FAULT].c_str());
        return;
    }
    gwpTelemetryEvent.fault = static_cast<uint64_t>(tmp);
    if (!(gwpTelemetryEvent.fault & TELEMETRY_SANITIZER_TYPE)) {
        HILOG_WARN(LOG_CORE, "not sanitizer telemetry type");
        return;
    }
    if (valuePairs.count(TELEMETRY_KEY_ID) == 0 || valuePairs[TELEMETRY_KEY_ID].empty()) {
        HILOG_ERROR(LOG_CORE, "invalid telemetryId");
        return;
    }
    if (valuePairs.count(TELEMETRY_KEY_BUNDLE_NAME) == 0) {
        HILOG_ERROR(LOG_CORE, "no bundleName");
        return;
    }

    gwpTelemetryEvent.telemetryId = valuePairs[TELEMETRY_KEY_ID];
    gwpTelemetryEvent.bundleName = valuePairs[TELEMETRY_KEY_BUNDLE_NAME];
    if (valuePairs.count(TELEMETRY_KEY_GWP_ENABLE) != 0) {
        gwpTelemetryEvent.gwpEnable = valuePairs[TELEMETRY_KEY_GWP_ENABLE];
    }
    if (valuePairs.count(TELEMETRY_KEY_GWP_SAMPLE_RATE) != 0) {
        gwpTelemetryEvent.gwpSampleRate = valuePairs[TELEMETRY_KEY_GWP_SAMPLE_RATE];
    }
    if (valuePairs.count(TELEMETRY_KEY_GWP_MAX_SLOTS) != 0) {
        gwpTelemetryEvent.gwpMaxSlots = valuePairs[TELEMETRY_KEY_GWP_MAX_SLOTS];
    }
    if (valuePairs.count(TELEMETRY_KEY_MIN_SAMPLE_SIZE) != 0) {
        gwpTelemetryEvent.minSampleSize = valuePairs[TELEMETRY_KEY_MIN_SAMPLE_SIZE];
    }
    if (valuePairs.count(TELEMETRY_KEY_STACK_PARAM) != 0) {
        gwpTelemetryEvent.stackParam = valuePairs[TELEMETRY_KEY_STACK_PARAM];
    }
    SetTelemetryParam(gwpTelemetryEvent);
}

void SanitizerTelemetry::SetTelemetryParam(const GwpTelemetryEvent& gwpTelemetryEvent)
{
    std::string sample = "";
    if (!gwpTelemetryEvent.gwpSampleRate.empty()) {
        sample = gwpTelemetryEvent.gwpSampleRate;
    }
    if (!gwpTelemetryEvent.gwpMaxSlots.empty()) {
        sample += ":" + gwpTelemetryEvent.gwpMaxSlots;
    }
    std::string bundleName = gwpTelemetryEvent.bundleName;
    SetIfTelemetryExist(PARAM_GWP_ASAN_ENABLE + bundleName, gwpTelemetryEvent.gwpEnable);
    SetIfTelemetryExist(PARAM_GWP_ASAN_SAMPLE + bundleName, sample);
    SetIfTelemetryExist(PARAM_GWP_ASAN_MIN_SIZE + bundleName, gwpTelemetryEvent.minSampleSize);
    SetIfTelemetryExist(PARAM_GWP_ASAN_LIBRARY + bundleName, gwpTelemetryEvent.stackParam);
}

void SanitizerTelemetry::ClearTelemetryParam(const std::string& bundleName)
{
    ClearIfParameterSet(PARAM_GWP_ASAN_ENABLE + bundleName);
    ClearIfParameterSet(PARAM_GWP_ASAN_SAMPLE + bundleName);
    ClearIfParameterSet(PARAM_GWP_ASAN_MIN_SIZE + bundleName);
    ClearIfParameterSet(PARAM_GWP_ASAN_LIBRARY + bundleName);
}

void SanitizerTelemetry::SetIfTelemetryExist(const std::string& key, const std::string& value)
{
    if (!value.empty()) {
        HILOG_INFO(LOG_CORE, "set parameter: %{public}s = %{public}s", key.c_str(), value.c_str());
        OHOS::system::SetParameter(key, value);
    }
}

void SanitizerTelemetry::ClearIfParameterSet(const std::string& key)
{
    std::string value = OHOS::system::GetParameter(key, "");
    if (!value.empty()) {
        HILOG_INFO(LOG_CORE, "clear parameter: %{public}s", key.c_str());
        OHOS::system::SetParameter(key, "");
    }
}

bool SanitizerTelemetry::SafeStoll(const std::string& str, long long& value)
{
    value = 0;
    size_t start = 0;
    bool isNegative = false;

    if (str.empty()) {
        HILOG_ERROR(LOG_CORE, "str is empty.");
        return false;
    }

    if (str[0] == '-') {
        isNegative = true;
        start = 1;
    } else if (str[0] == '+') {
        start = 1;
    }

    size_t index = start;
    for (const char c :str.substr(start)) {
        if (!isdigit(c)) {
            HILOG_ERROR(LOG_CORE, "digit check failed. str: %{public}s, index: %{public}zu", str.c_str(), index);
            return false;
        }
        if (value > (LLONG_MAX - (c - '0')) / DECIMAL_BASE) {
            HILOG_ERROR(LOG_CORE, "out of range, str: %{public}s", str.c_str());
            return false;
        }
        value = value * DECIMAL_BASE + (c - '0');
        index++;
    }

    if (isNegative) {
        value = -value;
    }

    HILOG_DEBUG(LOG_CORE, "success, str: %{public}s, result: %{public}lld", str.c_str(), value);
    return true;
}
}  // namespace HiviewDFX
}  // namespace OHOS