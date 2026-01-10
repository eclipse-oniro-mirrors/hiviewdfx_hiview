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
#include "parameter_ex.h"
#include "unistd.h"

namespace OHOS::HiviewDFX {
namespace {
DEFINE_LOG_TAG("TraceStateMachine");
const uint32_t FILE_SIZE_LITMIT = 100 * 1024;

const ScenarioInfo SCENARIO_COMMON_DROP_INFO {
    .scenario = TraceScenario::TRACE_COMMON_DROP,
    .args = {
        .tags = {
            "net", "dsched", "graphic", "multimodalinput", "dinput", "ark", "ace", "window", "zaudio", "daudio",
            "zmedia", "dcamera", "zcamera", "dhfwk", "app", "gresource", "ability", "power", "samgr", "ffrt", "nweb",
            "hdf", "virse", "workq", "ipa", "sched", "freq", "disk", "sync", "binder", "mmc", "membus", "load"
        },
        .fileSizeLimit = FILE_SIZE_LITMIT
    }
};

const ScenarioInfo SCENARIO_COMMON_INFO {
    .scenario = TraceScenario::TRACE_COMMON,
    .args = {
        .tags = {"net", "dsched", "graphic", "multimodalinput", "dinput", "ark", "ace", "window", "zaudio", "daudio",
            "zmedia", "dcamera", "zcamera", "dhfwk", "app", "gresource", "ability", "power", "samgr", "ffrt", "nweb",
            "hdf", "virse", "workq", "ipa", "sched", "freq", "disk", "sync", "binder", "mmc", "membus", "load"
        },
        .fileSizeLimit = FILE_SIZE_LITMIT
    }
};

const ScenarioInfo SCENARIO_COMMON_SCHEDLT_INFO {
    .scenario = TraceScenario::TRACE_COMMON,
    .args = {
        .tags = {"net", "dsched", "graphic", "multimodalinput", "dinput", "ark", "ace", "window", "zaudio", "daudio",
            "zmedia", "dcamera", "zcamera", "dhfwk", "app", "gresource", "ability", "power", "samgr", "ffrt", "nweb",
            "hdf", "virse", "workq", "ipa", "schedlt", "freq", "disk", "sync", "binder", "mmc", "membus", "load"
        },
        .fileSizeLimit = FILE_SIZE_LITMIT
    }
};
}

TraceStateMachine::TraceStateMachine()
{
    currentState_ = std::make_shared<TraceBaseState>();
}

TraceRet TraceStateMachine::DumpTrace(TraceScenario scenario, uint32_t maxDuration, uint64_t happenTime,
    TraceRetInfo &info)
{
    std::lock_guard<ffrt::mutex> lock(traceMutex_);
    auto ret = currentState_->DumpTrace(scenario, maxDuration, happenTime, info);
    if (ret.GetCodeError() == TraceErrorCode::TRACE_IS_OCCUPIED ||
        ret.GetCodeError() == TraceErrorCode::WRONG_TRACE_MODE) {
        RefreshState();
    }
    return ret;
}

TraceRet TraceStateMachine::DumpTraceAsync(const DumpTraceArgs &args, int64_t fileSizeLimit,
    TraceRetInfo &info, DumpTraceCallback callback)
{
    std::lock_guard<ffrt::mutex> lock(traceMutex_);
    auto ret = currentState_->DumpTraceAsync(args, fileSizeLimit, info, callback);
    if (ret.GetCodeError() == TraceErrorCode::TRACE_IS_OCCUPIED ||
        ret.GetCodeError() == TraceErrorCode::WRONG_TRACE_MODE) {
        RefreshState();
    }
    return ret;
}

TraceRet TraceStateMachine::DumpTraceWithFilter(uint32_t maxDuration, uint64_t happenTime, TraceRetInfo &info)
{
    std::lock_guard<ffrt::mutex> lock(traceMutex_);
    return currentState_->DumpTraceWithFilter(maxDuration, happenTime, info);
}

TraceRet TraceStateMachine::OpenTrace(const ScenarioInfo& scenarioInfo)
{
    HIVIEW_LOGI("TraceStateMachine OpenTrace scenario:%{public}d", static_cast<int>(scenarioInfo.scenario));
    std::lock_guard<ffrt::mutex> lock(traceMutex_);
    return currentState_->OpenTrace(scenarioInfo);
}

TraceRet TraceStateMachine::TraceDropOn(TraceScenario scenario)
{
    std::lock_guard<ffrt::mutex> lock(traceMutex_);
    return currentState_->TraceDropOn(scenario);
}

TraceRet TraceStateMachine::TraceDropOff(TraceScenario scenario, TraceRetInfo &info)
{
    std::lock_guard<ffrt::mutex> lock(traceMutex_);
    return currentState_->TraceDropOff(scenario, info);
}

TraceRet TraceStateMachine::TraceTelemetryOn()
{
    std::lock_guard<ffrt::mutex> lock(traceMutex_);
    return currentState_->TraceTelemetryOn();
}

TraceRet TraceStateMachine::TraceTelemetryOff()
{
    std::lock_guard<ffrt::mutex> lock(traceMutex_);
    return currentState_->TraceTelemetryOff();
}

TraceRet TraceStateMachine::CloseTrace(TraceScenario scenario)
{
    std::lock_guard<ffrt::mutex> lock(traceMutex_);
    if (auto ret = currentState_->CloseTrace(scenario); !ret.IsSuccess()) {
        HIVIEW_LOGW("fail stateError:%{public}d, codeError%{public}d", static_cast<int >(ret.stateError_),
            ret.codeError_);
        return ret;
    }
    return RefreshState();
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


void TraceStateMachine::TransToTelemetryState(TelemetryPolicy policy)
{
    HIVIEW_LOGI("to telemetry state");
    currentState_ = std::make_shared<TelemetryState>(policy);
}

void TraceStateMachine::TransToCloseState()
{
    HIVIEW_LOGI("to close state");
    currentState_ = std::make_shared<TraceBaseState>();
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

TraceRet TraceStateMachine::TraceCacheOn()
{
    std::lock_guard<ffrt::mutex> lock(traceMutex_);
    isCachSwitchOn_ = true;
    return currentState_->TraceCacheOn();
}

TraceRet TraceStateMachine::TraceCacheOff()
{
    std::lock_guard<ffrt::mutex> lock(traceMutex_);
    isCachSwitchOn_ = false;
    return currentState_->TraceCacheOff();
}

int32_t TraceStateMachine::SetFilterInfoToAppSwan(const std::vector<std::string>& appFilterNames)
{
    return currentState_->SetFilterInfoToAppSwan(appFilterNames);
}

int32_t TraceStateMachine::SetFilterPidInfo(pid_t pid)
{
    return currentState_->SetFilterPidInfo(pid);
}

bool TraceStateMachine::AddSymlinkXattr(const std::string &fileName)
{
    return currentState_->AddSymlinkXattr(fileName);
}

bool TraceStateMachine::RemoveSymlinkXattr(const std::string &fileName)
{
    return currentState_->RemoveSymlinkXattr(fileName);
}

TraceRet TraceStateMachine::PowerTelemetryOn()
{
    std::lock_guard<ffrt::mutex> lock(traceMutex_);
    return currentState_->PowerTelemetryOn();
}

TraceRet TraceStateMachine::PowerTelemetryOff()
{
    std::lock_guard<ffrt::mutex> lock(traceMutex_);
    return currentState_->PowerTelemetryOff();
}

TraceRet TraceStateMachine::RefreshState()
{
    auto currentScenario = currentState_->GetCurrentScenario();
    if (currentScenario == TraceScenario::TRACE_COMMON_DROP || currentScenario == TraceScenario::TRACE_COMMON) {
        HIVIEW_LOGI("trans to CloseState");
        currentState_->CloseTrace(currentScenario);
    }

    // All switch is closed
    if (traceSwitchState_ == 0) {
        HIVIEW_LOGI("commercial version and all switch is closed return");
        return {};
    }

    // If trace recording switch is open
    if ((traceSwitchState_ & 1) == 1) {
        HIVIEW_LOGI("common record on");
        return currentState_->OpenTrace(SCENARIO_COMMON_DROP_INFO);
    }

    // trace froze or UCollection switch is open or isBetaVersion
    if (Parameter::IsDeveloperMode()) {
        return currentState_->OpenTrace(SCENARIO_COMMON_INFO);
    }
    return currentState_->OpenTrace(SCENARIO_COMMON_SCHEDLT_INFO);
}

bool TraceStateMachine::RegisterTelemetryCallback(std::shared_ptr<TelemetryCallback> stateCallback)
{
    std::lock_guard<ffrt::mutex> lock(traceMutex_);
    return currentState_->RegisterTelemetryCallback(stateCallback);
}

TraceRet TraceStateMachine::PostTelemetryOn(uint64_t time)
{
    std::lock_guard<ffrt::mutex> lock(traceMutex_);
    return currentState_->PostTelemetryOn(time);
}

TraceRet TraceStateMachine::PostTelemetryTimeOut()
{
    std::lock_guard<ffrt::mutex> lock(traceMutex_);
    return currentState_->PostTelemetryTimeOut();
}

void TraceStateMachine::InitTelemetryStatus(bool isStatusOn)
{
    HIVIEW_LOGI("isStatusOn %{public}d", isStatusOn);
    currentState_->InitTelemetryStatus(isStatusOn);
}
}
