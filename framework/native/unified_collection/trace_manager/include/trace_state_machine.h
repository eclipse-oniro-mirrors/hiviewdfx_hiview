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
#include "time_util.h"
#include "trace_common.h"

namespace OHOS::HiviewDFX {
class TraceBaseState {
public:
    virtual ~TraceBaseState() = default;
    virtual TraceRet OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups);
    virtual TraceRet OpenTrace(TraceScenario scenario, const std::string &args);
    virtual TraceRet OpenTelemetryTrace(const std::string &args, const std::string &telemetryId);
    virtual TraceRet OpenAppTrace(int32_t appPid);
    virtual TraceRet DumpTrace(TraceScenario scenario, int maxDuration, uint64_t happenTime, TraceRetInfo &info);
    virtual TraceRet DumpTraceWithFilter(const std::vector<int32_t> &pidList, int maxDuration, uint64_t happenTime,
        TraceRetInfo &info);
    virtual TraceRet TraceDropOn(TraceScenario scenario);
    virtual TraceRet TraceDropOff(TraceScenario scenario, TraceRetInfo &info);
    virtual TraceRet TraceCacheOn();
    virtual TraceRet SetCacheParams(int32_t totalFileSize, int32_t sliceMaxDuration);
    virtual TraceRet TraceCacheOff();
    virtual TraceRet CloseTrace(TraceScenario scenario);

    virtual bool RegisterTelemetryCallback(std::function<void()> func)
    {
        return false;
    }

    virtual uint64_t GetTaskBeginTime()
    {
        return 0;
    }

    virtual int32_t GetAppPid()
    {
        return -1;
    }

protected:
    virtual std::string GetTag() = 0;
};

class CommandState : public TraceBaseState {
public:
    TraceRet OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups) override;
    TraceRet OpenTrace(TraceScenario scenario, const std::string &args) override;
    TraceRet DumpTrace(TraceScenario scenario, int maxDuration, uint64_t happenTime, TraceRetInfo &info) override;
    TraceRet TraceDropOn(TraceScenario scenario) override;
    TraceRet CloseTrace(TraceScenario scenario) override;

protected:
    std::string GetTag() override
    {
        return "CommandState";
    }
};

class CommandDropState : public TraceBaseState {
public:
    TraceRet OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups) override;
    TraceRet OpenTrace(TraceScenario scenario, const std::string &args) override;
    TraceRet TraceDropOff(TraceScenario scenario, TraceRetInfo &info) override;
    TraceRet CloseTrace(TraceScenario scenario) override;

protected:
    std::string GetTag() override
    {
        return "CommandDropState";
    }
};

class CommonState : public TraceBaseState {
public:
    explicit CommonState(bool isCachOn, int32_t totalFileSize, int32_t sliceMaxDuration);
    TraceRet OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups) override;
    TraceRet OpenTrace(TraceScenario scenario, const std::string &args) override;
    TraceRet DumpTrace(TraceScenario scenario, int maxDuration, uint64_t happenTime, TraceRetInfo &info) override;
    TraceRet TraceDropOn(TraceScenario scenario) override;
    TraceRet TraceCacheOn() override;
    TraceRet TraceCacheOff() override;
    TraceRet SetCacheParams(int32_t totalFileSize, int32_t sliceMaxDuration) override;
    TraceRet CloseTrace(TraceScenario scenario) override;

protected:
    std::string GetTag() override
    {
        return "CommonState";
    }

private:
    int32_t totalFileSize_ ;
    int32_t sliceMaxDuration_;
};

class CommonDropState : public TraceBaseState {
public:
    TraceRet OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups) override;
    TraceRet OpenTrace(TraceScenario scenario, const std::string &args) override;
    TraceRet TraceDropOff(TraceScenario scenario, TraceRetInfo &info) override;
    TraceRet CloseTrace(TraceScenario scenario) override;

protected:
    std::string GetTag() override
    {
        return "CommonDropState";
    }
};

class TelemetryState : public TraceBaseState {
public:
    ~TelemetryState() override
    {
        if (func_) {
            func_();
        }
    }

    bool RegisterTelemetryCallback(std::function<void()> func) override
    {
        func_ = std::move(func);
        return true;
    }

    TraceRet OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups) override;
    TraceRet OpenTrace(TraceScenario scenario, const std::string &args) override;
    TraceRet DumpTraceWithFilter(const std::vector<int32_t> &pidList, int maxDuration, uint64_t happenTime,
        TraceRetInfo &info) override;
    TraceRet CloseTrace(TraceScenario scenario) override;

protected:
    std::string GetTag() override
    {
        return "TelemetryState";
    }

private:
    std::function<void()> func_;
};

class DynamicState : public TraceBaseState {
public:
    explicit DynamicState(int32_t appPid) : appPid_(appPid)
    {
        taskBeginTime_ = TimeUtil::GetMilliseconds();
    }

    TraceRet OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups) override;
    TraceRet OpenTrace(TraceScenario scenario, const std::string &args) override;
    TraceRet OpenAppTrace(int32_t appPid) override;
    TraceRet OpenTelemetryTrace(const std::string &args, const std::string &telemetryId) override;
    TraceRet DumpTrace(TraceScenario scenario, int maxDuration, uint64_t happenTime, TraceRetInfo &info) override;
    TraceRet CloseTrace(TraceScenario scenario) override;

    uint64_t GetTaskBeginTime() override
    {
        return taskBeginTime_;
    }

    int32_t GetAppPid() override
    {
        return appPid_;
    }

protected:
    std::string GetTag() override
    {
        return "DynamicState";
    }

private:
    int32_t appPid_ = -1;
    uint64_t taskBeginTime_ = 0;
};

class CloseState : public TraceBaseState {
public:
    TraceRet OpenTelemetryTrace(const std::string &args, const std::string &telemetryId) override;

    TraceRet OpenAppTrace(int32_t appPid) override;

    TraceRet CloseTrace(TraceScenario scenario) override;

protected:
    std::string GetTag() override
    {
        return "CloseState";
    }
};

class TraceStateMachine : public OHOS::DelayedRefSingleton<TraceStateMachine> {
public:
    TraceStateMachine();

    TraceRet OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups);

    TraceRet OpenTrace(TraceScenario scenario, const std::string &args);

    TraceRet OpenTelemetryTrace(const std::string &args, const std::string &telemetryId);

    TraceRet OpenDynamicTrace(int32_t appid);

    TraceRet DumpTrace(TraceScenario scenario, int maxDuration, uint64_t happenTime, TraceRetInfo &info);

    TraceRet DumpTraceWithFilter(const std::vector<int32_t> &pidList, int maxDuration, uint64_t happenTime,
        TraceRetInfo &info);

    TraceRet TraceDropOn(TraceScenario scenario);

    TraceRet TraceDropOff(TraceScenario scenario, TraceRetInfo &info);

    TraceRet CloseTrace(TraceScenario scenario);

    TraceRet TraceCacheOn();

    TraceRet TraceCacheOff();

    TraceRet InitOrUpdateState();

    TraceRet InitCommonDropState();

    TraceRet InitCommonState();

    void TransToCommonState();

    void TransToCommandState();

    void TransToCommandDropState();

    void TransToCommonDropState();

    void TransToTeleMetryState(const std::string &telemetryId);

    void TransToDynamicState(int32_t appid);

    void TransToCloseState();

    bool RegisterTelemetryCallback(std::function<void()> func);

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
