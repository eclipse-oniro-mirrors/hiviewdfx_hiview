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

TraceRet CommandState::DumpTrace(TraceScenario scenario, uint32_t maxDuration, uint64_t happenTime, TraceRetInfo &info)
{
    if (scenario != TraceScenario::TRACE_COMMAND) {
        HIVIEW_LOGW(":%{public}s, scenario:%{public}d is fail", GetTag().c_str(), static_cast<int>(scenario));
        return TraceRet(TraceStateCode::FAIL);
    }
    info = Hitrace::DumpTrace(maxDuration, happenTime);
    HIVIEW_LOGI(":%{public}s, DumpTrace result:%{public}d", GetTag().c_str(), info.errorCode);
    return TraceRet(info.errorCode);
}

TraceRet CommandState::TraceDropOn(TraceScenario scenario)
{
    if (scenario != TraceScenario::TRACE_COMMAND) {
        HIVIEW_LOGW(":%{public}s, scenario:%{public}d is deny", GetTag().c_str(), static_cast<int>(scenario));
        return TraceRet(TraceStateCode::FAIL);
    }
    if (TraceErrorCode ret = Hitrace::RecordTraceOn(); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE(":%{public}s, TraceDropOn error:%{public}d", GetTag().c_str(), ret);
        return TraceRet(ret);
    }
    TraceStateMachine::GetInstance().TransToCommandDropState();
    return {};
}

TraceRet CommandState::CloseTrace(TraceScenario scenario)
{
    auto ret = TraceBaseState::CloseTrace(scenario);
    if (ret.IsSuccess()) {
        TraceStateMachine::GetInstance().SetCommandState(false);
    }
    return ret;
}

TraceRet CommandDropState::TraceDropOff(TraceScenario scenario, TraceRetInfo &info)
{
    if (scenario != TraceScenario::TRACE_COMMAND) {
        HIVIEW_LOGW(":%{public}s, scenario:%{public}d is fail", GetTag().c_str(), static_cast<int>(scenario));
        return TraceRet(TraceStateCode::FAIL);
    }
    if (info = Hitrace::RecordTraceOff(); info.errorCode != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE(":%{public}s, TraceDropOff error:%{public}d", GetTag().c_str(), info.errorCode);
        return TraceRet(info.errorCode);
    }
    TraceStateMachine::GetInstance().TransToCommandState();
    return {};
}
}
