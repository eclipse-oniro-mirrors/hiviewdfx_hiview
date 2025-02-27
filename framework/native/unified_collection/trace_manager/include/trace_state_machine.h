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
#include <vector>

#include "singleton.h"
#include "time_util.h"
#include "trace_common.h"

namespace OHOS {
namespace HiviewDFX {

class TraceBaseState {
public:
    virtual ~TraceBaseState() = default;
    virtual TraceRet OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups);
    virtual TraceRet OpenTrace(TraceScenario scenario, const std::string &args);
    virtual TraceRet OpenAppTrace(int32_t appPid);
    virtual TraceRet DumpTrace(TraceScenario scenario, int maxDuration, uint64_t happenTime, TraceRetInfo &info);
    virtual TraceRet TraceDropOn(TraceScenario scenario);
    virtual TraceRet TraceDropOff(TraceScenario scenario, TraceRetInfo &info);
    virtual TraceRet TraceCacheOn(uint64_t totalFileSize, uint64_t sliceMaxDuration);
    virtual TraceRet TraceCacheOff();
    virtual TraceRet CloseTrace(TraceScenario scenario);

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
    TraceRet OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups) override;
    TraceRet OpenTrace(TraceScenario scenario, const std::string &args) override;
    TraceRet DumpTrace(TraceScenario scenario, int maxDuration, uint64_t happenTime, TraceRetInfo &info) override;
    TraceRet TraceDropOn(TraceScenario scenario) override;
    TraceRet TraceCacheOn(uint64_t totalFileSize, uint64_t sliceMaxDuration) override;
    TraceRet TraceCacheOff() override;
    TraceRet CloseTrace(TraceScenario scenario) override;

protected:
    std::string GetTag() override
    {
        return "CommonState";
    }
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

class DynamicState : public TraceBaseState {
public:
    explicit DynamicState(int32_t appPid) : appPid_(appPid)
    {
        taskBeginTime_ = TimeUtil::GetMilliseconds();
    }

    TraceRet OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups) override;
    TraceRet OpenTrace(TraceScenario scenario, const std::string &args) override;
    TraceRet OpenAppTrace(int32_t appPid) override;
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
    TraceRet OpenDynamicTrace(int32_t appid);
    TraceRet DumpTrace(TraceScenario scenario, int maxDuration, uint64_t happenTime, TraceRetInfo &info);
    TraceRet TraceDropOn(TraceScenario scenario);
    TraceRet TraceDropOff(TraceScenario scenario, TraceRetInfo &info);
    TraceRet CloseTrace(TraceScenario scenario);
    TraceRet TraceCacheOn(uint64_t totalFileSize = DEFAULT_TOTAL_CACHE_FILE_SIZE,
        uint64_t sliceMaxDuration = DEFAULT_TRACE_SLICE_DURATION);
    TraceRet TraceCacheOff();
    TraceRet InitOrUpdateState();

    void TransToCommonState();
    void TransToCommandState();
    void TransToCommandDropState();
    void TransToCommonDropState();
    void TransToDynamicState(int32_t appid);
    void TransToCloseState();

    TraceRet InitCommonDropState();
    TraceRet InitCommonState();
    TraceRet RecoverState();

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
        uint8_t beta = 1 << 3;
        traceSwitchState_ = traceSwitchState_ | beta;
    }

    void CloseVersionBeta()
    {
        uint8_t beta = 1 << 3;
        traceSwitchState_ = traceSwitchState_ & (~beta);
    }

    void SetTraceSwitchUcOn()
    {
        uint8_t ucollection = 1 << 2;
        traceSwitchState_ = traceSwitchState_ | ucollection;
    }

    void SetTraceSwitchUcOff()
    {
        uint8_t ucollection = 1 << 2;
        traceSwitchState_ = traceSwitchState_ & (~ucollection);
    }

    void SetTraceSwitchFreezeOn()
    {
        uint8_t freeze = 1 << 1;
        traceSwitchState_ = traceSwitchState_ | freeze;
    }

    void SetTraceSwitchFreezeOff()
    {
        uint8_t freeze = 1 << 1;
        traceSwitchState_ = traceSwitchState_ & (~freeze);
    }

    void SetTraceSwitchDevOn()
    {
        uint8_t dev = 1;
        traceSwitchState_ = traceSwitchState_ | dev;
    }

    void SetTraceSwitchDevOff()
    {
        uint8_t dev = 1;
        traceSwitchState_ = traceSwitchState_ & (~dev);
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
    std::shared_ptr<TraceBaseState> currentState_;
    uint8_t traceSwitchState_ = 0;
    bool isCommandState_ = false;
    std::mutex traceMutex_;
};
}
}
#endif // HIVIEWDFX_HIVIEW_TRACE_STATE_MACHINE_H
