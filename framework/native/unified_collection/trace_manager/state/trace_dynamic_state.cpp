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
#include "trace_dynamic_state.h"

#include "trace_state_machine.h"
#include "hiview_logger.h"

namespace OHOS::HiviewDFX {
namespace {
    DEFINE_LOG_TAG("TraceStateMachine");
    const std::vector<std::string> TELEMETRY_TAG_GROUPS_DEFAULT = {"telemetry"};
}
TraceRet DynamicState::OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups)
{
    if (auto closeRet = Hitrace::CloseTrace(); closeRet != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("%{public}s:  CloseTrace result:%{public}d", GetTag().c_str(), closeRet);
        return TraceRet(closeRet);
    }
    return TraceBaseState::OpenTrace(scenario, tagGroups);
}

TraceRet DynamicState::OpenTrace(TraceScenario scenario, const std::string &args)
{
    if (auto closeRet = Hitrace::CloseTrace(); closeRet != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("%{public}s: CloseTrace result:%{public}d", GetTag().c_str(), closeRet);
        return TraceRet(closeRet);
    }
    return TraceBaseState::OpenTrace(scenario, args);
}

TraceRet DynamicState::OpenTelemetryTrace(const std::string &args, TelemetryPolicy policy)
{
    if (auto closeRet = Hitrace::CloseTrace(); closeRet != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("%{public}s:  CloseTrace result:%{public}d", GetTag().c_str(), closeRet);
        return TraceRet(closeRet);
    }
    auto ret = args.empty() ? Hitrace::OpenTrace(TELEMETRY_TAG_GROUPS_DEFAULT) : Hitrace::OpenTrace(args);
    HIVIEW_LOGI("%{public}s: args:%{public}s: result:%{public}d", GetTag().c_str(), args.c_str(), ret);
    if (ret != TraceErrorCode::SUCCESS) {
        return TraceRet(ret);
    }
    TraceStateMachine::GetInstance().TransToTeleMetryState(policy);
    return {};
}

TraceRet DynamicState::OpenAppTrace(int32_t appPid)
{
    HIVIEW_LOGW("DynamicState already open, occupied by appid:%{public}d", appPid);
    return TraceRet(TraceStateCode::DENY);
}

TraceRet DynamicState::DumpTrace(TraceScenario scenario, int maxDuration, uint64_t happenTime, TraceRetInfo &info)
{
    if (scenario != TraceScenario::TRACE_DYNAMIC) {
        HIVIEW_LOGW(":%{public}s, scenario:%{public}d is fail", GetTag().c_str(), static_cast<int>(scenario));
        return TraceRet(TraceStateCode::FAIL);
    }
    info = Hitrace::DumpTrace(maxDuration, happenTime);
    HIVIEW_LOGI(":%{public}s, DumpTrace result:%{public}d", GetTag().c_str(), info.errorCode);
    return TraceRet(info.errorCode);
}

TraceRet DynamicState::CloseTrace(TraceScenario scenario)
{
    if (scenario != TraceScenario::TRACE_DYNAMIC) {
        HIVIEW_LOGW(":%{public}s, scenario:%{public}d is fail", GetTag().c_str(), static_cast<int>(scenario));
        return TraceRet(TraceStateCode::FAIL);
    }
    return TraceBaseState::CloseTrace(scenario);
}
}