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
#ifndef HIVIEWDFX_HIVIEW_TRACE_TELEMETRY_STATE_H
#define HIVIEWDFX_HIVIEW_TRACE_TELEMETRY_STATE_H
#include <memory>

#include "trace_common.h"
#include "trace_base_state.h"
#include "telemetry_state_machine.h"

namespace OHOS::HiviewDFX {
using namespace Telemetry;
class TelemetryState : public TraceBaseState {
public:
    explicit TelemetryState(TelemetryPolicy policy) : policy_(policy) {}

    ~TelemetryState() override
    {
        if (stateCallback_ != nullptr) {
            stateCallback_->OnTelemetryFinish();
        }
    }

    bool RegisterTelemetryCallback(std::shared_ptr<TelemetryCallback> stateCallback) override;

    TraceRet DumpTraceWithFilter(uint32_t maxDuration, uint64_t happenTime, TraceRetInfo &info) override;
    TraceRet PowerTelemetryOn() override;
    TraceRet PowerTelemetryOff() override;
    TraceRet TraceTelemetryOn() override;
    TraceRet TraceTelemetryOff() override;
    TraceRet PostTelemetryOn(uint64_t time) override;
    TraceRet PostTelemetryTimeOut() override;
    void InitTelemetryStatus(bool isStatusOn) override;

protected:
    std::string GetTag() const override
    {
        return "TelemetryState";
    }

    TraceScenario GetCurrentScenario() const override
    {
        return TraceScenario::TRACE_TELEMETRY;
    }

private:
    std::shared_ptr<TelemetryCallback> stateCallback_;
    TelemetryPolicy policy_;
    std::shared_ptr<TeleMetryStateMachine> innerStateMachine_;
};
}
#endif //HIVIEWDFX_HIVIEW_TRACE_TELEMETRY_STATE_H
