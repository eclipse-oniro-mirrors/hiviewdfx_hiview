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
#include "hitrace_dump.h"
#include "hiview_logger.h"
#include "telemetry_state_machine.h"
#include "time_util.h"

namespace OHOS::HiviewDFX::Telemetry {
namespace {
DEFINE_LOG_TAG("TeleMetryStateMachine");
}

bool InitState::IsTraceOn()
{
    return false;
}

TraceRet InitState::TraceOn()
{
    auto ptr = stateMachine_.lock();
    if (ptr == nullptr) {
        HIVIEW_LOGE(":%{public}s, stateMachine_ null", GetTag().c_str());
        return TraceRet(TraceStateCode::FAIL);
    }
    auto ret = Hitrace::SetTraceStatus(true);
    if (ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE(":%{public}s, result:%{public}d", GetTag().c_str(), ret);
        return TraceRet{ret};
    }
    ptr->TransToTraceOnState(1, 0);
    if (ptr->stateCallback_ != nullptr) {
        ptr->stateCallback_->OnTelemetryTraceOn();
    }
    return {};
}

TraceRet InitState::TraceOff()
{
    HIVIEW_LOGW(":%{public}s, already trace off", GetTag().c_str());
    return {};
}

TraceRet InitState::PostOn(uint64_t time)
{
    auto ptr = stateMachine_.lock();
    if (ptr == nullptr) {
        HIVIEW_LOGE(":%{public}s, stateMachine_ null", GetTag().c_str());
        return TraceRet(TraceStateCode::FAIL);
    }
    auto ret = Hitrace::SetTraceStatus(true);
    if (ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE(":%{public}s, result:%{public}d", GetTag().c_str(), ret);
        return TraceRet{ret};
    }
    auto postEndTime = TimeUtil::GetBootTimeMs() + time;
    ptr->TransToTraceOnState(0, postEndTime);
    if (ptr->stateCallback_ != nullptr) {
        ptr->stateCallback_->OnTelemetryTraceOn();
    }
    HIVIEW_LOGI("state:%{public}s time:%{public}" PRId64 "", GetTag().c_str(), time);
    return TraceRet(TraceStateCode::UPDATE_TIME);
}

TraceRet InitState::TimeOut()
{
    HIVIEW_LOGI(":%{public}s, do nothing", GetTag().c_str());
    return {};
}

std::string InitState::GetTag()
{
    return "InitState";
}

bool TraceOnState::IsTraceOn()
{
    return true;
}

TraceRet TraceOnState::TraceOn()
{
    traceOnCount_++;
    HIVIEW_LOGI("state:%{public}s, traceOnCount_:%{public}d", GetTag().c_str(), traceOnCount_);
    return {};
}

TraceRet TraceOnState::TraceOff()
{
    auto ptr = stateMachine_.lock();
    if (ptr == nullptr) {
        HIVIEW_LOGE(":%{public}s, stateMachine_ null", GetTag().c_str());
        return TraceRet(TraceStateCode::FAIL);
    }
    if (traceOnCount_ == 0) {
        HIVIEW_LOGE(":%{public}s, post on running, still trace on", GetTag().c_str());
        return TraceRet(TraceStateCode::NO_TRIGGER);
    }
    traceOnCount_--;
    if (traceOnCount_ > 0 || postEndTime_ > 0) {
        HIVIEW_LOGW(":%{public}s, traceOnCount_:%{public}d still trace on", GetTag().c_str(), traceOnCount_);
        return TraceRet(TraceStateCode::NO_TRIGGER);
    }
    auto ret = Hitrace::SetTraceStatus(false);
    if (ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE(":%{public}s, result:%{public}d", GetTag().c_str(), ret);
        return TraceRet{ret};
    }
    ptr->TransToInitState();
    if (stateMachine_.lock()->stateCallback_ != nullptr) {
        stateMachine_.lock()->stateCallback_->OnTelemetryTraceOff();
    }
    return {};
}

TraceRet TraceOnState::PostOn(uint64_t time)
{
    auto postEndTime = TimeUtil::GetBootTimeMs() + time;
    if (postEndTime < postEndTime_) {
        HIVIEW_LOGW("already post on, no need to update time out");
        return {};
    }
    postEndTime_ = postEndTime;
    HIVIEW_LOGI("state:%{public}s time:%{public}" PRId64 "", GetTag().c_str(), time);
    return TraceRet(TraceStateCode::UPDATE_TIME);
}

TraceRet TraceOnState::TimeOut()
{
    auto ptr = stateMachine_.lock();
    if (ptr == nullptr) {
        HIVIEW_LOGE(":%{public}s, stateMachine_ null", GetTag().c_str());
        return TraceRet(TraceStateCode::FAIL);
    }
    auto now = TimeUtil::GetBootTimeMs();
    if (now < postEndTime_) {
        HIVIEW_LOGW(":%{public}s do nothing, time out invalid", GetTag().c_str());
        return TraceRet(TraceStateCode::NO_TRIGGER);
    }
    postEndTime_ = 0;
    if (traceOnCount_ > 0) {
        HIVIEW_LOGW(":%{public}s trace on running count:%{public}d still trace on", GetTag().c_str(), traceOnCount_);
        return TraceRet(TraceStateCode::NO_TRIGGER);
    }
    auto ret = Hitrace::SetTraceStatus(false);
    if (ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE(":%{public}s, result:%{public}d", GetTag().c_str(), ret);
        return TraceRet{ret};
    }
    ptr->TransToInitState();
    if (stateMachine_.lock()->stateCallback_ != nullptr) {
        stateMachine_.lock()->stateCallback_->OnTelemetryTraceOff();
    }
    HIVIEW_LOGI(":%{public}s, result:%{public}d", GetTag().c_str(), ret);
    return {};
}

std::string TraceOnState::GetTag()
{
    return "TraceOnState";
}

void TeleMetryStateMachine::TransToInitState()
{
    HIVIEW_LOGI("init");
    currentState_ = initStateState_;
}

void TeleMetryStateMachine::TransToTraceOnState(uint32_t traceOnCount, uint64_t postEndTime)
{
    HIVIEW_LOGI("traceOnCount:%{public}d", traceOnCount);
    traceOnState_->SetTraceOnCount(traceOnCount);
    traceOnState_->SetPostEndTime(postEndTime);
    currentState_ = traceOnState_;
}
}
