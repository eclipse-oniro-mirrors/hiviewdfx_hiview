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

TraceRet TraceBaseState::OpenTrace(const ScenarioInfo& scenarioInfo)
{
    // check state whether allow open
    if (scenarioInfo.scenario <= GetCurrentScenario()) {
        HIVIEW_LOGW(":%{public}s, scenario:%{public}d deny", GetTag().c_str(), static_cast<int>(scenarioInfo.scenario));
        return TraceRet(TraceStateCode::DENY);
    }

    // close current trace scenario
    if (TraceErrorCode ret = Hitrace::CloseTrace(); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE(":%{public}s, CloseTrace error:%{public}d", GetTag().c_str(), ret);
        return TraceRet(ret);
    }

    // open new trace trace scenario
    if (auto ret = Hitrace::OpenTrace(scenarioInfo.args); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE(":%{public}s, scenario:%{public}d error:%{public}d", GetTag().c_str(),
            static_cast<int>(scenarioInfo.scenario), ret);
        return TraceRet(ret);
    }
    HIVIEW_LOGI(":%{public}s, OpenTrace success scenario:%{public}d", GetTag().c_str(),
        static_cast<int>(scenarioInfo.scenario));
    switch (scenarioInfo.scenario) {
        case TraceScenario::TRACE_COMMAND:
            TraceStateMachine::GetInstance().TransToCommandState();
            break;
        case TraceScenario::TRACE_COMMON:
            TraceStateMachine::GetInstance().TransToCommonState();
            break;
        case TraceScenario::TRACE_COMMON_DROP:
            if (auto retTraceOn = Hitrace::RecordTraceOn(); retTraceOn != TraceErrorCode::SUCCESS) {
                HIVIEW_LOGE("RecordTraceOn error:%{public}d", retTraceOn);
                return TraceRet(retTraceOn);
            }
            TraceStateMachine::GetInstance().TransToCommonDropState();
            break;
        case TraceScenario::TRACE_TELEMETRY:
            TraceStateMachine::GetInstance().TransToTelemetryState(scenarioInfo.tracePolicy);
            break;
        case TraceScenario::TRACE_DYNAMIC:
            TraceStateMachine::GetInstance().TransToDynamicState(scenarioInfo.args.appPid);
        default:
            break;
    }
    return {};
}

TraceRet TraceBaseState::CloseTrace(TraceScenario scenario)
{
    if (GetCurrentScenario() == TraceScenario::TRACE_CLOSE) {
        HIVIEW_LOGW("trace aleady close");
        return {};
    }
    if (scenario != GetCurrentScenario()) {
        HIVIEW_LOGW(":%{public}s, close scenario:%{public}d deny", GetTag().c_str(),
            static_cast<int>(scenario));
        return TraceRet(TraceStateCode::DENY);
    }
    if (TraceErrorCode ret = Hitrace::CloseTrace(); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE(":%{public}s, CloseTrace error:%{public}d", GetTag().c_str(), ret);
        return TraceRet(ret);
    }
    TraceStateMachine::GetInstance().TransToCloseState();
    return {};
}

TraceRet TraceBaseState::TraceCacheOn()
{
    HIVIEW_LOGW(":%{public}s,  invoke state fail", GetTag().c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::TraceCacheOff()
{
    HIVIEW_LOGW(":%{public}s, invoke state fail", GetTag().c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::SetCacheParams(int32_t totalFileSize, int32_t sliceMaxDuration)
{
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::DumpTrace(TraceScenario scenario, uint32_t maxDuration, uint64_t happenTime,
    TraceRetInfo &info)
{
    HIVIEW_LOGW(":%{public}s, scenario:%{public}d is fail", GetTag().c_str(), static_cast<int>(scenario));
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::DumpTraceAsync(const DumpTraceArgs &args, int64_t fileSizeLimit,
    TraceRetInfo &info, DumpTraceCallback callback)
{
    HIVIEW_LOGW(":%{public}s, scenario:%{public}d is fail", GetTag().c_str(), static_cast<int>(args.scenario));
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::TraceDropOn(TraceScenario scenario)
{
    HIVIEW_LOGW(":%{public}s, scenario:%{public}d is fail", GetTag().c_str(), static_cast<int>(scenario));
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::TraceDropOff(TraceScenario scenario, TraceRetInfo &info)
{
    HIVIEW_LOGW(":%{public}s, scenario:%{public}d is fail", GetTag().c_str(), static_cast<int>(scenario));
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::DumpTraceWithFilter(uint32_t maxDuration, uint64_t happenTime, TraceRetInfo &info)
{
    HIVIEW_LOGW("%{public}s is fail", GetTag().c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::TraceTelemetryOn()
{
    HIVIEW_LOGW("%{public}s, state fail", GetTag().c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::TraceTelemetryOff()
{
    HIVIEW_LOGW("%{public}s, state fail", GetTag().c_str());
    return TraceRet(TraceStateCode::FAIL);
}

int32_t TraceBaseState::SetAppFilterInfo(const std::string &bundleName)
{
    return Hitrace::SetFilterAppName(bundleName);
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
    HIVIEW_LOGW("%{public}s, state fail", GetTag().c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::PowerTelemetryOff()
{
    HIVIEW_LOGW("%{public}s, state fail", GetTag().c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::PostTelemetryOn(uint64_t time)
{
    HIVIEW_LOGW("%{public}s, state fail", GetTag().c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::PostTelemetryTimeOut()
{
    HIVIEW_LOGW("%{public}s, state fail", GetTag().c_str());
    return TraceRet(TraceStateCode::FAIL);
}

void TraceBaseState::InitTelemetryStatus(bool isStatusOn)
{
    HIVIEW_LOGW("%{public}s, state fail", GetTag().c_str());
}
}