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
#ifndef HIVIEWDFX_HIVIEW_TRACE_STATE_MACHINE_H
#define HIVIEWDFX_HIVIEW_TRACE_STATE_MACHINE_H

#include <cinttypes>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "singleton.h"

#include "trace_common.h"
#include "trace_base_state.h"

namespace OHOS::HiviewDFX {
class TraceStateMachine : public OHOS::DelayedRefSingleton<TraceStateMachine> {
public:
    TraceStateMachine();
    TraceRet OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups);
    TraceRet OpenTrace(TraceScenario scenario, const std::string &args);
    TraceRet OpenTelemetryTrace(const std::string &args, TelemetryPolicy policy);
    TraceRet OpenDynamicTrace(int32_t appid);
    TraceRet DumpTrace(TraceScenario scenario, int maxDuration, uint64_t happenTime, TraceRetInfo &info);
    TraceRet DumpTraceWithFilter(const std::vector<int32_t> &pidList, int maxDuration, uint64_t happenTime,
        TraceRetInfo &info);
    TraceRet TraceDropOn(TraceScenario scenario);
    TraceRet TraceDropOff(TraceScenario scenario, TraceRetInfo &info);
    TraceRet CloseTrace(TraceScenario scenario);
    TraceRet TraceCacheOn();
    TraceRet TraceCacheOff();
    TraceRet TraceTelemetryOn();
    TraceRet TraceTelemetryOff();
    TraceRet PostTelemetryOn(uint64_t time);
    TraceRet PostTelemetryTimeOut();
    TraceRet PowerTelemetryOn();
    TraceRet PowerTelemetryOff();
    int32_t SetAppFilterInfo(const std::string &bundleName);
    int32_t SetFilterPidInfo(pid_t pid);
    void InitTelemetryStatus(bool isStatusOn);
    void TransToCommonState();
    void TransToCommandState();
    void TransToCommandDropState();
    void TransToCommonDropState();
    void TransToTeleMetryState(TelemetryPolicy policy);
    void TransToDynamicState(int32_t appid);
    void TransToCloseState();
    bool RegisterTelemetryCallback(std::shared_ptr<TelemetryCallback> stateCallback);

    void SetCacheParams(int32_t totalFileSize, int32_t sliceMaxDuration)
    {
        cacheSizeLimit_ = totalFileSize;
        cacheSliceSpan_ = sliceMaxDuration;
    }

    int32_t GetCurrentAppPid()
    {
        return currentState_->GetAppPid();
    }

    uint64_t GetTaskBeginTime()
    {
        return currentState_->GetTaskBeginTime();
    }

    void SetTraceVersionBeta()
    {
        std::lock_guard<std::mutex> lock(traceMutex_);
        uint8_t beta = 1 << 3;
        traceSwitchState_ = traceSwitchState_ | beta;
        RecoverState();
    }

    void CloseVersionBeta()
    {
        uint8_t beta = 1 << 3;
        traceSwitchState_ = traceSwitchState_ & (~beta);
    }

    void SetTraceSwitchUcOn()
    {
        std::lock_guard<std::mutex> lock(traceMutex_);
        uint8_t ucollection = 1 << 2;
        traceSwitchState_ = traceSwitchState_ | ucollection;
        RecoverState();
    }

    void SetTraceSwitchUcOff()
    {
        std::lock_guard<std::mutex> lock(traceMutex_);
        uint8_t ucollection = 1 << 2;
        traceSwitchState_ = traceSwitchState_ & (~ucollection);
        RecoverState();
    }

    void SetTraceSwitchFreezeOn()
    {
        std::lock_guard<std::mutex> lock(traceMutex_);
        uint8_t freeze = 1 << 1;
        traceSwitchState_ = traceSwitchState_ | freeze;
        RecoverState();
    }

    void SetTraceSwitchFreezeOff()
    {
        std::lock_guard<std::mutex> lock(traceMutex_);
        uint8_t freeze = 1 << 1;
        traceSwitchState_ = traceSwitchState_ & (~freeze);
        RecoverState();
    }

    void SetTraceSwitchDevOn()
    {
        std::lock_guard<std::mutex> lock(traceMutex_);
        uint8_t dev = 1;
        traceSwitchState_ = traceSwitchState_ | dev;
        RecoverState();
    }

    void SetTraceSwitchDevOff()
    {
        std::lock_guard<std::mutex> lock(traceMutex_);
        uint8_t dev = 1;
        traceSwitchState_ = traceSwitchState_ & (~dev);
        RecoverState();
    }

    void SetCommandState(bool isCommandState)
    {
        isCommandState_ = isCommandState;
    }

    bool GetCommandState()
    {
        return isCommandState_;
    }

private:
    TraceRet RecoverState();
    TraceRet InitCommonDropState();
    TraceRet InitCommonState();

private:
    std::shared_ptr<TraceBaseState> currentState_;
    int32_t cacheSizeLimit_ = 800; // 800MB;
    int32_t cacheSliceSpan_ = 10; // 10 seconds
    uint8_t traceSwitchState_ = 0;
    bool isCommandState_ = false;
    bool isCachSwitchOn_ = false;
    std::mutex traceMutex_;
};
}

#endif // HIVIEWDFX_HIVIEW_TRACE_STATE_MACHINE_H
