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
}

TraceRet DynamicState::DumpTrace(const std::string& scenarioName, uint32_t maxDuration, uint64_t happenTime,
    TraceRetInfo &info)
{
    if (scenarioName != ScenarioName::APP_DYNAMIC) {
        HIVIEW_LOGW("DynamicState scenario:%{public}s is fail", scenarioName.c_str());
        return TraceRet(TraceStateCode::FAIL);
    }
    info = Hitrace::DumpTrace(maxDuration, happenTime);
    HIVIEW_LOGI("DynamicState DumpTrace result:%{public}d", info.errorCode);
    return TraceRet(info.errorCode);
}
}