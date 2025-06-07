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
#include "trace_state_machine.h"

#include "trace_command_state.h"
#include "trace_common_state.h"
#include "trace_telemetry_state.h"
#include "trace_dynamic_state.h"
#include "hiview_logger.h"
#include "unistd.h"

namespace OHOS::HiviewDFX {
namespace {
DEFINE_LOG_TAG("TraceStateMachine");
const std::vector<std::string> TAG_GROUPS = {"scene_performance"};
}

TraceStateMachine::TraceStateMachine()
{
    currentState_ = std::make_shared<CloseState>();
}

TraceRet TraceStateMachine::OpenDynamicTrace(int32_t appid)
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    return currentState_->OpenAppTrace(appid);
}

TraceRet TraceStateMachine::OpenTelemetryTrace(const std::string &args, TelemetryPolicy policy)
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    return currentState_->OpenTelemetryTrace(args, policy);
}

TraceRet TraceStateMachine::DumpTrace(TraceScenario scenario, int maxDuration, uint64_t happenTime, TraceRetInfo &info)
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    auto ret = currentState_->DumpTrace(scenario, maxDuration, happenTime, info);
    if (ret.GetCodeError() == TraceErrorCode::TRACE_IS_OCCUPIED ||
        ret.GetCodeError() == TraceErrorCode::WRONG_TRACE_MODE) {
        RecoverState();
    }
    return ret;
}

TraceRet TraceStateMachine::DumpTraceAsync(const DumpTraceArgs &args, int64_t fileSizeLimit,
    TraceRetInfo &info, DumpTraceCallback callback)
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    auto ret = currentState_->DumpTraceAsync(args, fileSizeLimit, info, callback);
    if (ret.GetCodeError() == TraceErrorCode::TRACE_IS_OCCUPIED ||
        ret.GetCodeError() == TraceErrorCode::WRONG_TRACE_MODE) {
        RecoverState();
    }
    return ret;
}

TraceRet TraceStateMachine::DumpTraceWithFilter(int maxDuration, uint64_t happenTime, TraceRetInfo &info)
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    return currentState_->DumpTraceWithFilter(maxDuration, happenTime, info);
}

TraceRet TraceStateMachine::OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups)
{
    HIVIEW_LOGI("TraceStateMachine OpenTrace scenario:%{public}d", static_cast<int>(scenario));
    std::lock_guard<std::mutex> lock(traceMutex_);
    return currentState_->OpenTrace(scenario, tagGroups);
}

TraceRet TraceStateMachine::OpenTrace(TraceScenario scenario, const std::string &args)
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    HIVIEW_LOGI("TraceStateMachine OpenTrace scenario:%{public}d", static_cast<int>(scenario));
    return currentState_->OpenTrace(scenario, args);
}

TraceRet TraceStateMachine::TraceDropOn(TraceScenario scenario)
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    return currentState_->TraceDropOn(scenario);
}

TraceRet TraceStateMachine::TraceDropOff(TraceScenario scenario, TraceRetInfo &info)
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    return currentState_->TraceDropOff(scenario, info);
}

TraceRet TraceStateMachine::TraceTelemetryOn()
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    return currentState_->TraceTelemetryOn();
}

TraceRet TraceStateMachine::TraceTelemetryOff()
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    return currentState_->TraceTelemetryOff();
}

TraceRet TraceStateMachine::CloseTrace(TraceScenario scenario)
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    if (auto ret = currentState_->CloseTrace(scenario); !ret.IsSuccess()) {
        HIVIEW_LOGW("fail stateError:%{public}d, codeError%{public}d", static_cast<int >(ret.stateError_),
            ret.codeError_);
        return ret;
    }
    return RecoverState();
}

void TraceStateMachine::TransToDynamicState(int32_t appPid)
{
    HIVIEW_LOGI("to app state");
    currentState_ = std::make_shared<DynamicState>(appPid);
}

void TraceStateMachine::TransToCommonState()
{
    HIVIEW_LOGI("to common state");
    currentState_ = std::make_shared<CommonState>(isCachSwitchOn_, cacheSizeLimit_, cacheSliceSpan_);
}

void TraceStateMachine::TransToCommonDropState()
{
    HIVIEW_LOGI("to common drop state");
    currentState_ = std::make_shared<CommonDropState>();
}


void TraceStateMachine::TransToTeleMetryState(TelemetryPolicy policy)
{
    HIVIEW_LOGI("to telemetry state");
    currentState_ = std::make_shared<TelemetryState>(policy);
}

void TraceStateMachine::TransToCloseState()
{
    HIVIEW_LOGI("to close state");
    currentState_ = std::make_shared<CloseState>();
}

void TraceStateMachine::TransToCommandState()
{
    HIVIEW_LOGI("to command state");
    SetCommandState(true);
    currentState_ = std::make_shared<CommandState>();
}

void TraceStateMachine::TransToCommandDropState()
{
    HIVIEW_LOGI("to command drop state");
    currentState_ = std::make_shared<CommandDropState>();
}

TraceRet TraceStateMachine::InitCommonDropState()
{
    if (auto ret = Hitrace::OpenTrace(TAG_GROUPS); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("OpenTrace error:%{public}d", ret);
        return TraceRet(ret);
    }
    if (auto retTraceOn = Hitrace::RecordTraceOn(); retTraceOn != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("DumpTraceOn error:%{public}d", retTraceOn);
        return TraceRet(retTraceOn);
    }
    TransToCommonDropState();
    return {};
}

TraceRet TraceStateMachine::InitCommonState()
{
    if (auto ret = Hitrace::OpenTrace(TAG_GROUPS); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("OpenTrace error:%{public}d", ret);
        return TraceRet(ret);
    }
    TransToCommonState();
    return {};
}

TraceRet TraceStateMachine::TraceCacheOn()
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    isCachSwitchOn_ = true;
    return currentState_->TraceCacheOn();
}

TraceRet TraceStateMachine::TraceCacheOff()
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    isCachSwitchOn_ = false;
    return currentState_->TraceCacheOff();
}

int32_t TraceStateMachine::SetAppFilterInfo(const std::string &bundleName)
{
    return currentState_->SetAppFilterInfo(bundleName);
}

int32_t TraceStateMachine::SetFilterPidInfo(pid_t pid)
{
    return currentState_->SetFilterPidInfo(pid);
}

TraceRet TraceStateMachine::PowerTelemetryOn()
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    return currentState_->PowerTelemetryOn();
}

TraceRet TraceStateMachine::PowerTelemetryOff()
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    return currentState_->PowerTelemetryOff();
}

TraceRet TraceStateMachine::RecoverState()
{
    if (Hitrace::GetTraceMode() != Hitrace::TraceMode::CLOSE) {
        if (auto ret = Hitrace::CloseTrace(); ret != TraceErrorCode::SUCCESS) {
            HIVIEW_LOGE("Hitrace close error:%{public}d", ret);
            return TraceRet(ret);
        }
    }

    // All switch is closed
    if (traceSwitchState_ == 0) {
        HIVIEW_LOGI("commercial version and all switch is closed, trans to CloseState");
        TransToCloseState();
        return {};
    }

    // If trace recording switch is open
    if ((traceSwitchState_ & 1) == 1) {
        return InitCommonDropState();
    }

    // trace froze or UCollection switch is open or isBetaVersion
    return InitCommonState();
}

bool TraceStateMachine::RegisterTelemetryCallback(std::shared_ptr<TelemetryCallback> stateCallback)
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    return currentState_->RegisterTelemetryCallback(stateCallback);
}

TraceRet TraceStateMachine::PostTelemetryOn(uint64_t time)
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    return currentState_->PostTelemetryOn(time);
}

TraceRet TraceStateMachine::PostTelemetryTimeOut()
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    return currentState_->PostTelemetryTimeOut();
}

void TraceStateMachine::InitTelemetryStatus(bool isStatusOn)
{
    HIVIEW_LOGI("isStatusOn %{public}d", isStatusOn);
    currentState_->InitTelemetryStatus(isStatusOn);
}
}
