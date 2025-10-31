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

namespace OHOS::HiviewDFX {
namespace {
DEFINE_LOG_TAG("TraceStateMachine");
const std::vector<std::string> TELEMETRY_TAG_GROUPS_DEFAULT = {"telemetry"};
}

TraceRet TraceBaseState::CloseTrace(TraceScenario scenario)
{
    if (TraceErrorCode ret = Hitrace::CloseTrace(); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE(":%{public}s, CloseTrace error:%{public}d", GetTag().c_str(), ret);
        return TraceRet(ret);
    }
    return {};
}

TraceRet TraceBaseState::OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups)
{
    if (auto ret = Hitrace::OpenTrace(tagGroups); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE(":%{public}s, scenario:%{public}d error:%{public}d", GetTag().c_str(), static_cast<int>(scenario),
                    ret);
        return TraceRet(ret);
    }
    HIVIEW_LOGI(":%{public}s, OpenTrace success scenario:%{public}d", GetTag().c_str(), static_cast<int>(scenario));
    switch (scenario) {
        case TraceScenario::TRACE_COMMAND:
            TraceStateMachine::GetInstance().TransToCommandState();
            break;
        case TraceScenario::TRACE_COMMON:
            TraceStateMachine::GetInstance().TransToCommonState();
            break;
        default:
            break;
    }
    return {};
}

TraceRet TraceBaseState::OpenTrace(TraceScenario scenario, const std::string &args)
{
    if (TraceErrorCode ret = Hitrace::OpenTrace(args); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE(":%{public}s, scenario:%{public}d error:%{public}d", GetTag().c_str(), static_cast<int>(scenario),
                    ret);
        return TraceRet(ret);
    }
    HIVIEW_LOGI(":%{public}s,OpenTrace success with scenario:%{public}d", GetTag().c_str(), static_cast<int>(scenario));
    switch (scenario) {
        case TraceScenario::TRACE_COMMAND:
            TraceStateMachine::GetInstance().TransToCommandState();
            break;
        case TraceScenario::TRACE_COMMON:
            TraceStateMachine::GetInstance().TransToCommonState();
            break;
        default:
            break;
    }
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

TraceRet TraceBaseState::OpenTelemetryTrace(const std::string &args, TelemetryPolicy policy)
{
    HIVIEW_LOGW(":%{public}s, invoke state deny", GetTag().c_str());
    return TraceRet(TraceStateCode::DENY);
}

TraceRet TraceBaseState::OpenAppTrace(int32_t appPid)
{
    HIVIEW_LOGW(":%{public}s, invoke state deny", GetTag().c_str());
    return TraceRet(TraceStateCode::DENY);
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

TraceRet CloseState::OpenTelemetryTrace(const std::string &args, TelemetryPolicy policy)
{
    auto ret = args.empty() ? Hitrace::OpenTrace(TELEMETRY_TAG_GROUPS_DEFAULT) : Hitrace::OpenTrace(args);
    HIVIEW_LOGI("%{public}s: args:%{public}s: result:%{public}d", GetTag().c_str(), args.c_str(), ret);
    if (ret != TraceErrorCode::SUCCESS) {
        return TraceRet(ret);
    }
    TraceStateMachine::GetInstance().TransToTeleMetryState(policy);
    return {};
}

TraceRet CloseState::OpenAppTrace(int32_t appPid)
{
    std::string appArgs = "tags:graphic,ace,app clockType:boot bufferSize:10240 overwrite:1 fileLimit:20 ";
    appArgs.append("appPid:").append(std::to_string(appPid));
    if (auto ret = Hitrace::OpenTrace(appArgs); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("%{public}s: args:%{public}s, result:%{public}d", GetTag().c_str(), appArgs.c_str(), ret);
        return TraceRet(ret);
    }
    TraceStateMachine::GetInstance().TransToDynamicState(appPid);
    return {};
}

TraceRet CloseState::CloseTrace(TraceScenario scenario)
{
    return {};
}
}