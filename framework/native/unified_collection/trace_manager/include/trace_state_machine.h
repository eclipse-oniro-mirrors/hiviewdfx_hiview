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
#include <string>
#include <vector>

#include "ffrt.h"
#include "singleton.h"
#include "trace_common.h"
#include "trace_base_state.h"

namespace OHOS::HiviewDFX {
class TraceStateMachine : public OHOS::DelayedRefSingleton<TraceStateMachine> {
public:
    TraceStateMachine();
    TraceRet OpenTrace(const ScenarioInfo& scenarioInfo);
    TraceRet DumpTrace(TraceScenario scenario, uint32_t maxDuration, uint64_t happenTime, TraceRetInfo &info);
    TraceRet DumpTraceAsync(const DumpTraceArgs &args, int64_t fileSizeLimit,
        TraceRetInfo &info, DumpTraceCallback callback);
    TraceRet DumpTraceWithFilter(uint32_t maxDuration, uint64_t happenTime, TraceRetInfo &info);
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
    int32_t SetFilterInfoToAppSwan(const std::vector<std::string>& appFilterNames);
    int32_t SetFilterPidInfo(pid_t pid);
    bool AddSymlinkXattr(const std::string& fileName);
    bool RemoveSymlinkXattr(const std::string& fileName);
    void InitTelemetryStatus(bool isStatusOn);
    void TransToCommonState();
    void TransToCommandState();
    void TransToCommandDropState();
    void TransToCommonDropState();
    void TransToTelemetryState(TelemetryPolicy policy);
    void TransToDynamicState(int32_t appid);
    void TransToCloseState();
    bool RegisterTelemetryCallback(std::shared_ptr<TelemetryCallback> stateCallback);

    void SetCacheParams(int32_t totalFileSize, int32_t sliceMaxDuration)
    {
        cacheSizeLimit_ = totalFileSize;
        cacheSliceSpan_ = sliceMaxDuration;
    }

    std::pair<int32_t, uint64_t> GetCurrentAppInfo()
    {
        return currentState_->GetCurrentAppInfo();
    }

    void SetTraceVersionBeta()
    {
        std::lock_guard<ffrt::mutex> lock(traceMutex_);
        uint8_t beta = 1 << 3;
        traceSwitchState_ = traceSwitchState_ | beta;
        RefreshState();
    }

    void CloseVersionBeta()
    {
        uint8_t beta = 1 << 3;
        traceSwitchState_ = traceSwitchState_ & (~beta);
    }

    void SetTraceSwitchUcOn()
    {
        std::lock_guard<ffrt::mutex> lock(traceMutex_);
        uint8_t ucollection = 1 << 2;
        traceSwitchState_ = traceSwitchState_ | ucollection;
        RefreshState();
    }

    void SetTraceSwitchUcOff()
    {
        std::lock_guard<ffrt::mutex> lock(traceMutex_);
        uint8_t ucollection = 1 << 2;
        traceSwitchState_ = traceSwitchState_ & (~ucollection);
        RefreshState();
    }

    void SetTraceSwitchFreezeOn()
    {
        std::lock_guard<ffrt::mutex> lock(traceMutex_);
        uint8_t freeze = 1 << 1;
        traceSwitchState_ = traceSwitchState_ | freeze;
        RefreshState();
    }

    void SetTraceSwitchFreezeOff()
    {
        std::lock_guard<ffrt::mutex> lock(traceMutex_);
        uint8_t freeze = 1 << 1;
        traceSwitchState_ = traceSwitchState_ & (~freeze);
        RefreshState();
    }

    void SetTraceSwitchDevOn()
    {
        std::lock_guard<ffrt::mutex> lock(traceMutex_);
        uint8_t dev = 1;
        traceSwitchState_ = traceSwitchState_ | dev;
        RefreshState();
    }

    void SetTraceSwitchDevOff()
    {
        std::lock_guard<ffrt::mutex> lock(traceMutex_);
        uint8_t dev = 1;
        traceSwitchState_ = traceSwitchState_ & (~dev);
        RefreshState();
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
    TraceRet RefreshState();

private:
    std::shared_ptr<TraceBaseState> currentState_;
    int32_t cacheSizeLimit_ = 800; // 800MB;
    int32_t cacheSliceSpan_ = 10; // 10 seconds
    uint8_t traceSwitchState_ = 0;
    bool isCommandState_ = false;
    bool isCachSwitchOn_ = false;
    ffrt::mutex traceMutex_;
};
}

#endif // HIVIEWDFX_HIVIEW_TRACE_STATE_MACHINE_H
