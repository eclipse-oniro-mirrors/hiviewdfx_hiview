/*
* Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef HIVIEWDFX_HIVIEW_TRACE_APP_STATE_H
#define HIVIEWDFX_HIVIEW_TRACE_APP_STATE_H
#include "trace_common.h"
#include "trace_base_state.h"

namespace OHOS::HiviewDFX {
class AppSystemState : public TraceBaseState {
public:
    explicit AppSystemState(int32_t appPid) : appPid_(appPid) {}

    TraceRet DumpTrace(const std::string& scenarioName, uint32_t maxDuration, uint64_t happenTime,
        TraceRetInfo &info) override;

    std::pair<int32_t, uint64_t> GetCurrentAppInfo() override
    {
        return {appPid_, 0};
    }

protected:
    std::string GetStateScenario() const override
    {
        return ScenarioName::APP_SYSTEM;
    }

    uint32_t GetStateLevel() const override
    {
        return ScenarioLevel::APP_SYSTEM;
    }

private:
    int32_t appPid_ = -1;
};
}
#endif
