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
#include "trace_telemetry_state.h"

#include <hitrace_option/hitrace_option.h>

#include "hiview_logger.h"

namespace OHOS::HiviewDFX {
namespace {
    DEFINE_LOG_TAG("TraceStateMachine");
    const std::vector<std::string> TELEMETRY_TAG_GROUPS_DEFAULT = {"telemetry"};
}

bool TelemetryState::RegisterTelemetryCallback(std::shared_ptr<TelemetryCallback> stateCallback)
{
    if (stateCallback == nullptr) {
        return false;
    }
    stateCallback_ = stateCallback;
    stateCallback_->OnTelemetryStart();
    if (policy_ == TelemetryPolicy::DEFAULT) {
        stateCallback_->OnTelemetryTraceOn();
    }
    return true;
}

TraceRet TelemetryState::OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups)
{
    if (auto closeRet = Hitrace::CloseTrace(); closeRet != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE(":%{public}s, CloseTrace result:%{public}d", GetTag().c_str(), closeRet);
        return TraceRet(closeRet);
    }
    return TraceBaseState::OpenTrace(scenario, tagGroups);
}

TraceRet TelemetryState::OpenTrace(TraceScenario scenario, const std::string &args)
{
    if (auto closeRet = Hitrace::CloseTrace(); closeRet != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE(":%{public}s, CloseTrace result:%{public}d", GetTag().c_str(), closeRet);
        return TraceRet(closeRet);
    }
    return TraceBaseState::OpenTrace(scenario, args);
}

TraceRet TelemetryState::DumpTraceWithFilter(const std::vector<int32_t> &pidList, int maxDuration, uint64_t happenTime,
                                             TraceRetInfo &info)
{
    if (policy_ != TelemetryPolicy::DEFAULT) {
        if (innerStateMachine_ != nullptr && !innerStateMachine_->IsTraceOn()) {
            HIVIEW_LOGW(":%{public}s, IsTraceOn false", GetTag().c_str());
            return TraceRet(TraceStateCode::FAIL);
        }
    }
    info = Hitrace::DumpTrace(maxDuration, happenTime);
    HIVIEW_LOGI(":%{public}s, result:%{public}d", GetTag().c_str(), info.errorCode);
    return TraceRet(info.errorCode);
}

TraceRet TelemetryState::CloseTrace(TraceScenario scenario)
{
    if (scenario != TraceScenario::TRACE_TELEMETRY) {
        HIVIEW_LOGW(":%{public}s, scenario:%{public}d is deny", GetTag().c_str(), static_cast<int>(scenario));
        return TraceRet(TraceStateCode::FAIL);
    }
    return TraceBaseState::CloseTrace(scenario);
}

TraceRet TelemetryState::PowerTelemetryOn()
{
    if (policy_ != TelemetryPolicy::POWER || innerStateMachine_ == nullptr) {
        HIVIEW_LOGW(":%{public}s, policy:%{public}d is error", GetTag().c_str(), static_cast<int>(policy_));
        return TraceRet(TraceStateCode::POLICY_ERROR);
    }
    if (innerStateMachine_->IsTraceOn()) {
        HIVIEW_LOGW(":%{public}s, already power on", GetTag().c_str());
        return {};
    }
    return innerStateMachine_->TraceOn();
}

TraceRet TelemetryState::PowerTelemetryOff()
{
    if (policy_ != TelemetryPolicy::POWER || innerStateMachine_ == nullptr) {
        HIVIEW_LOGW(":%{public}s, policy:%{public}d is error", GetTag().c_str(), static_cast<int>(policy_));
        return TraceRet(TraceStateCode::POLICY_ERROR);
    }
    if (!innerStateMachine_->IsTraceOn()) {
        HIVIEW_LOGW(":%{public}s, already power off", GetTag().c_str());
        return {};
    }
    return innerStateMachine_->TraceOff();
}

TraceRet TelemetryState::TraceTelemetryOn()
{
    if (policy_ != TelemetryPolicy::MANUAL || innerStateMachine_ == nullptr) {
        HIVIEW_LOGW(":%{public}s, policy:%{public}d is error", GetTag().c_str(), static_cast<int>(policy_));
        return TraceRet(TraceStateCode::POLICY_ERROR);
    }
    return innerStateMachine_->TraceOn();
}

TraceRet TelemetryState::TraceTelemetryOff()
{
    if (policy_ != TelemetryPolicy::MANUAL || innerStateMachine_ == nullptr) {
        HIVIEW_LOGW(":%{public}s, policy:%{public}d is error", GetTag().c_str(), static_cast<int>(policy_));
        return TraceRet(TraceStateCode::POLICY_ERROR);
    }
    return innerStateMachine_->TraceOff();
}

TraceRet TelemetryState::PostTelemetryOn(uint64_t time)
{
    if (policy_ != TelemetryPolicy::MANUAL || innerStateMachine_ == nullptr) {
        HIVIEW_LOGW(":%{public}s, policy:%{public}d is error", GetTag().c_str(), static_cast<int>(policy_));
        return TraceRet(TraceStateCode::POLICY_ERROR);
    }
    return innerStateMachine_->PostOn(time);
}

TraceRet TelemetryState::PostTelemetryTimeOut()
{
    if (policy_ != TelemetryPolicy::MANUAL || innerStateMachine_ == nullptr) {
        HIVIEW_LOGW(":%{public}s, policy:%{public}d is error", GetTag().c_str(), static_cast<int>(policy_));
        return TraceRet(TraceStateCode::POLICY_ERROR);
    }
    return innerStateMachine_->TimeOut();
}

void TelemetryState::InitTelemetryStatus(bool isStatusOn)
{
    if (stateCallback_== nullptr) {
        HIVIEW_LOGW(":%{public}s, stateCallback_== nullptr", GetTag().c_str());
        return;
    }
    HIVIEW_LOGI(":%{public}s, isStatusOn:%{public}d", GetTag().c_str(), isStatusOn);
    innerStateMachine_ = std::make_shared<TeleMetryStateMachine>();
    auto initState = std::make_shared<InitState>(innerStateMachine_);
    innerStateMachine_->SetInitState(initState);
    auto traceOnState = std::make_shared<TraceOnState>(innerStateMachine_);
    innerStateMachine_->SetTraceOnState(traceOnState);

    innerStateMachine_->RegisterTelemetryCallback(stateCallback_);
    if (isStatusOn) {
        innerStateMachine_->TransToTraceOnState(1, 0);
        stateCallback_->OnTelemetryTraceOn();
    } else {
        innerStateMachine_->TransToInitState();
    }
}
}