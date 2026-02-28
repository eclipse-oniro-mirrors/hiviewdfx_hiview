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

#include "trace_command_state.h"

#include "trace_state_machine.h"
#include "hiview_logger.h"

namespace OHOS::HiviewDFX {
namespace {
DEFINE_LOG_TAG("TraceStateMachine");
}

TraceRet CommandState::DumpTrace(const std::string& scenarioName, uint32_t maxDuration, uint64_t happenTime,
    TraceRetInfo &info)
{
    if (scenarioName != ScenarioName::COMMAND) {
        HIVIEW_LOGW("command state, scenario:%{public}s is fail", scenarioName.c_str());
        return TraceRet(TraceStateCode::FAIL);
    }
    info = Hitrace::DumpTrace(maxDuration, happenTime);
    HIVIEW_LOGI("command state, DumpTrace result:%{public}d", info.errorCode);
    return TraceRet(info.errorCode);
}

TraceRet CommandState::TraceDropOn(const std::string& scenarioName)
{
    if (scenarioName != ScenarioName::COMMAND) {
        HIVIEW_LOGW("command state, scenario:%{public}s is deny", scenarioName.c_str());
        return TraceRet(TraceStateCode::FAIL);
    }
    if (TraceErrorCode ret = Hitrace::RecordTraceOn(); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("command state, TraceDropOn error:%{public}d", ret);
        return TraceRet(ret);
    }
    TraceStateMachine::GetInstance().TransToCommandDropState();
    return {};
}

TraceRet CommandState::CloseTrace(const std::string& scenarioName)
{
    auto ret = TraceBaseState::CloseTrace(scenarioName);
    if (ret.IsSuccess()) {
        TraceStateMachine::GetInstance().SetCommandState(false);
    }
    return ret;
}

TraceRet CommandDropState::TraceDropOff(const std::string& scenarioName, TraceRetInfo &info)
{
    if (scenarioName != ScenarioName::COMMAND_DROP) {
        HIVIEW_LOGW("CommandDropState, scenario:%{public}s is fail", scenarioName.c_str());
        return TraceRet(TraceStateCode::FAIL);
    }
    if (info = Hitrace::RecordTraceOff(); info.errorCode != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("CommandDropState, TraceDropOff error:%{public}d", info.errorCode);
        return TraceRet(info.errorCode);
    }
    TraceStateMachine::GetInstance().TransToCommandState();
    return {};
}

TraceRet CommandDropState::CloseTrace(const std::string& scenarioName)
{
    if (scenarioName != ScenarioName::COMMAND_DROP && scenarioName != ScenarioName::COMMAND) {
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
}
