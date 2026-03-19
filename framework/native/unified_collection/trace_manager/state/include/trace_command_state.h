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
#ifndef HIVIEWDFX_HIVIEW_TRACE_COMMAND_STATE_H
#define HIVIEWDFX_HIVIEW_TRACE_COMMAND_STATE_H
#include <memory>

#include "trace_common.h"
#include "trace_base_state.h"

namespace OHOS::HiviewDFX {
class CommandState : public TraceBaseState {
public:
    TraceRet DumpTrace(const std::string& scenarioName, uint32_t maxDuration, uint64_t happenTime,
        TraceRetInfo &info) override;
    TraceRet TraceDropOn(const std::string& scenarioName) override;
    TraceRet CloseTrace(const std::string& scenarioName) override;

protected:
    std::string GetStateScenario() const override
    {
        return ScenarioName::COMMAND;
    }

    uint32_t GetStateLevel() const override
    {
        return ScenarioLevel::COMMAND;
    }
};

class CommandDropState : public TraceBaseState {
public:
    TraceRet TraceDropOff(const std::string& scenarioName, TraceRetInfo &info) override;

    TraceRet CloseTrace(const std::string& scenarioName) override;

protected:
    std::string GetStateScenario() const override
    {
        return ScenarioName::COMMAND_DROP;
    }

    uint32_t GetStateLevel() const override
    {
        return ScenarioLevel::COMMAND_DROP;
    }
};
}
#endif //HIVIEWDFX_HIVIEW_TRACE_COMMAND_STATE_H
