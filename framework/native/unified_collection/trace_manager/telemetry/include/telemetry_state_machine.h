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
#ifndef HIVIEWDFX_HIVIEW_TELEMETRY_STATE_MACHINE_H
#define HIVIEWDFX_HIVIEW_TELEMETRY_STATE_MACHINE_H
#include <memory>
#include <utility>

#include "trace_common.h"

namespace OHOS::HiviewDFX::Telemetry {
class TeleMetryStateMachine;
class BaseState {
public:
    friend class TeleMetryStateMachine;
    virtual ~BaseState() = default;
    virtual bool IsTraceOn() = 0;
    virtual TraceRet TraceOn() = 0;
    virtual TraceRet TraceOff() = 0;
    virtual TraceRet PostOn(uint64_t time) = 0;
    virtual TraceRet TimeOut() = 0;

protected:
    virtual std::string GetTag() = 0;
};

class InitState : public BaseState {
public:
    explicit InitState(std::shared_ptr<TeleMetryStateMachine> stateMachine) : stateMachine_(stateMachine) {}

    bool IsTraceOn() override;
    TraceRet TraceOn() override;
    TraceRet TraceOff() override;
    TraceRet PostOn(uint64_t time) override;
    TraceRet TimeOut() override;
protected:
    std::string GetTag() override;

private:
    std::weak_ptr<TeleMetryStateMachine> stateMachine_;
};

class TraceOnState : public BaseState {
public:
    explicit TraceOnState(std::weak_ptr<TeleMetryStateMachine> stateMachine) : stateMachine_(stateMachine) {}

    bool IsTraceOn() override;
    TraceRet TraceOn() override;
    TraceRet TraceOff() override;
    TraceRet PostOn(uint64_t time) override;
    TraceRet TimeOut() override;

    void SetPostEndTime(uint64_t postEndTime)
    {
        postEndTime_ = postEndTime;
    }

    void SetTraceOnCount(uint32_t traceOnCount)
    {
        traceOnCount_ = traceOnCount;
    }

protected:
    std::string GetTag() override;
private:
    std::weak_ptr<TeleMetryStateMachine> stateMachine_;
    uint32_t traceOnCount_ = 0;
    uint64_t postEndTime_ = 0;
};

class TeleMetryStateMachine {
public:
    friend class InitState;
    friend class TraceOnState;

    bool IsTraceOn()
    {
        return currentState_->IsTraceOn();
    }

    TraceRet TraceOn()
    {
        return currentState_->TraceOn();
    }

    TraceRet TraceOff()
    {
        return currentState_->TraceOff();
    }

    TraceRet PostOn(uint64_t time)
    {
        return currentState_->PostOn(time);
    }

    TraceRet TimeOut()
    {
        return currentState_->TimeOut();
    }

    void RegisterTelemetryCallback(std::shared_ptr<TelemetryCallback> stateCallback)
    {
        stateCallback_ = stateCallback;
    }

    void SetInitState(std::shared_ptr<InitState> initStateState)
    {
        initStateState_ = initStateState;
    }

    void SetTraceOnState(std::shared_ptr<TraceOnState> traceOnState)
    {
        traceOnState_ = traceOnState;
    }

    void TransToInitState();
    void TransToTraceOnState(uint32_t traceOnCount, uint64_t postEndTime);

private:
    std::shared_ptr<BaseState> currentState_;
    std::shared_ptr<InitState> initStateState_;
    std::shared_ptr<TraceOnState> traceOnState_;
    std::shared_ptr<TelemetryCallback> stateCallback_;
};
}
#endif //HIVIEWDFX_HIVIEW_TELEMETRY_STATE_MACHINE_H
