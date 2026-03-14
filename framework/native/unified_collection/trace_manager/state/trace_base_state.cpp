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
#include "trace_base_state.h"

#include <hitrace_option/hitrace_option.h>

#include "trace_state_machine.h"
#include "hiview_logger.h"
#include "hitrace_dump.h"

namespace OHOS::HiviewDFX {
namespace {
DEFINE_LOG_TAG("TraceStateMachine");
}

TraceRet TraceBaseState::OpenTrace(const Scenario& scenario)
{
    // check state whether allow open
    if (scenario.level <= GetStateLevel()) {
        HIVIEW_LOGW("state:%{public}s, scenario:%{public}s deny", GetStateScenario().c_str(), scenario.name.c_str());
        return TraceRet(TraceStateCode::DENY);
    }

    // close current trace scenario
    if (TraceErrorCode ret = Hitrace::CloseTrace(); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE(":%{public}s, CloseTrace error:%{public}d", GetStateScenario().c_str(), ret);
        return TraceRet(ret);
    }

    // open new trace trace scenario
    if (auto ret = Hitrace::OpenTrace(scenario.args); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("state:%{public}s, scenario:%{public}s error:%{public}d", GetStateScenario().c_str(),
            scenario.name.c_str(), ret);
        return TraceRet(ret);
    }
    HIVIEW_LOGI("state:%{public}s, OpenTrace success scenario:%{public}s", GetStateScenario().c_str(),
        scenario.name.c_str());
    if (scenario.name == ScenarioName::COMMAND) {
        TraceStateMachine::GetInstance().TransToCommandState();
    }
    if (scenario.name == ScenarioName::COMMON_BETA) {
        TraceStateMachine::GetInstance().TransToCommonState();
    }
    if (scenario.name == ScenarioName::COMMON_DROP) {
        if (auto retTraceOn = Hitrace::RecordTraceOn(); retTraceOn != TraceErrorCode::SUCCESS) {
            HIVIEW_LOGE("RecordTraceOn error:%{public}d", retTraceOn);
            return TraceRet(retTraceOn);
        }
        TraceStateMachine::GetInstance().TransToCommonDropState();
    }
    if (scenario.name == ScenarioName::TELEMETRY) {
        TraceStateMachine::GetInstance().TransToTelemetryState(scenario.tracePolicy);
    }
    if (scenario.name == ScenarioName::APP_SYSTEM) {
        TraceStateMachine::GetInstance().TransToAppSystemState(scenario.args.appPid);
    }
    if (scenario.name == ScenarioName::APP_DYNAMIC) {
        TraceStateMachine::GetInstance().TransToDynamicState(scenario.args.appPid);
    }
    return {};
}

TraceRet TraceBaseState::CloseTrace(const std::string& scenarioName)
{
    if (GetStateScenario() == ScenarioName::CLOSE) {
        HIVIEW_LOGW("trace already close");
        return {};
    }
    if (scenarioName != GetStateScenario()) {
        HIVIEW_LOGW("state:%{public}s, close scenario:%{public}s deny",  GetStateScenario().c_str(),
            scenarioName.c_str());
        return TraceRet(TraceStateCode::DENY);
    }
    if (TraceErrorCode ret = Hitrace::CloseTrace(); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("state:%{public}s, CloseTrace error:%{public}d", GetStateScenario().c_str(), ret);
        return TraceRet(ret);
    }
    TraceStateMachine::GetInstance().TransToCloseState();
    return {};
}

TraceRet TraceBaseState::TraceCacheOn()
{
    HIVIEW_LOGW("state:%{public}s, invoke state fail", GetStateScenario().c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::TraceCacheOff()
{
    HIVIEW_LOGW("state:%{public}s, invoke state fail", GetStateScenario().c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::SetCacheParams(int32_t totalFileSize, int32_t sliceMaxDuration)
{
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::DumpTrace(const std::string& scenarioName, uint32_t maxDuration, uint64_t happenTime,
    TraceRetInfo &info)
{
    HIVIEW_LOGW("state:%{public}s, scenario:%{public}s is fail", GetStateScenario().c_str(), scenarioName.c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::DumpTraceAsync(const DumpTraceArgs &args, int64_t fileSizeLimit,
    TraceRetInfo &info, DumpTraceCallback callback)
{
    HIVIEW_LOGW("state:%{public}s, scenario:%{public}s fail", GetStateScenario().c_str(), args.scenarioName.c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::TraceDropOn(const std::string& scenarioName)
{
    HIVIEW_LOGW("state:%{public}s, scenario:%{public}s is fail", GetStateScenario().c_str(), scenarioName.c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::TraceDropOff(const std::string& scenarioName, TraceRetInfo &info)
{
    HIVIEW_LOGW("state:%{public}s, scenario:%{public}s is fail", GetStateScenario().c_str(), scenarioName.c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::DumpTraceWithFilter(uint32_t maxDuration, uint64_t happenTime, TraceRetInfo &info)
{
    HIVIEW_LOGW("state:%{public}s is fail", GetStateScenario().c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::TraceTelemetryOn()
{
    HIVIEW_LOGW("state:%{public}s, fail", GetStateScenario().c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::TraceTelemetryOff()
{
    HIVIEW_LOGW("state:%{public}s, fail", GetStateScenario().c_str());
    return TraceRet(TraceStateCode::FAIL);
}

int32_t TraceBaseState::SetFilterInfoToAppSwan(const std::vector<std::string>& appfilterNames)
{
    return Hitrace::SetFilterAppName(appfilterNames);
}

int32_t TraceBaseState::SetFilterPidInfo(pid_t pid)
{
    return Hitrace::AddFilterPid(pid);
}

bool TraceBaseState::AddSymlinkXattr(const std::string &fileName)
{
    return Hitrace::AddSymlinkXattr(fileName);
}

bool TraceBaseState::RemoveSymlinkXattr(const std::string &fileName)
{
    return Hitrace::RemoveSymlinkXattr(fileName);
}

TraceRet TraceBaseState::PowerTelemetryOn()
{
    HIVIEW_LOGW("state:%{public}s, fail", GetStateScenario().c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::PowerTelemetryOff()
{
    HIVIEW_LOGW("state:%{public}s, fail", GetStateScenario().c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::PostTelemetryOn(uint64_t time)
{
    HIVIEW_LOGW("state:%{public}s, fail", GetStateScenario().c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::PostTelemetryTimeOut()
{
    HIVIEW_LOGW("state:%{public}s, fail", GetStateScenario().c_str());
    return TraceRet(TraceStateCode::FAIL);
}

void TraceBaseState::InitTelemetryStatus(bool isStatusOn)
{
    HIVIEW_LOGW("state:%{public}s, fail", GetStateScenario().c_str());
}
}