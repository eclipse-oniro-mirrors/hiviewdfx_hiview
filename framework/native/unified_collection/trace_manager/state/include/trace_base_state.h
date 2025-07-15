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
#ifndef HIVIEWDFX_HIVIEW_TRACE_BASE_STATE_H
#define HIVIEWDFX_HIVIEW_TRACE_BASE_STATE_H
#include <memory>

#include "trace_common.h"

namespace OHOS::HiviewDFX {
class TraceBaseState {
public:
    virtual ~TraceBaseState() = default;
    virtual TraceRet OpenTrace(TraceScenario scenario, const std::vector <std::string> &tagGroups);
    virtual TraceRet OpenTrace(TraceScenario scenario, const std::string &args);
    virtual TraceRet OpenTelemetryTrace(const std::string &args, TelemetryPolicy policy);
    virtual TraceRet OpenAppTrace(int32_t appPid);
    virtual TraceRet DumpTrace(TraceScenario scenario, uint32_t maxDuration, uint64_t happenTime, TraceRetInfo &info);
    virtual TraceRet DumpTraceAsync(const DumpTraceArgs &args, int64_t fileSizeLimit,
        TraceRetInfo &info, DumpTraceCallback callback);
    virtual TraceRet DumpTraceWithFilter(uint32_t maxDuration, uint64_t happenTime, TraceRetInfo &info);
    virtual TraceRet TraceDropOn(TraceScenario scenario);
    virtual TraceRet TraceDropOff(TraceScenario scenario, TraceRetInfo &info);
    virtual TraceRet TraceCacheOn();
    virtual TraceRet SetCacheParams(int32_t totalFileSize, int32_t sliceMaxDuration);
    virtual TraceRet TraceCacheOff();
    virtual TraceRet CloseTrace(TraceScenario scenario);
    virtual TraceRet TraceTelemetryOn();
    virtual TraceRet TraceTelemetryOff();
    virtual TraceRet PostTelemetryOn(uint64_t time);
    virtual TraceRet PostTelemetryTimeOut();
    virtual TraceRet PowerTelemetryOn();
    virtual TraceRet PowerTelemetryOff();
    virtual int32_t SetAppFilterInfo(const std::string &bundleName);
    virtual int32_t SetFilterPidInfo(pid_t pid);
    virtual void InitTelemetryStatus(bool isStatusOn);

    virtual bool RegisterTelemetryCallback(std::shared_ptr <TelemetryCallback> stateCallback)
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

class CloseState : public TraceBaseState {
public:
    TraceRet OpenTelemetryTrace(const std::string &args, TelemetryPolicy policy) override;
    TraceRet OpenAppTrace(int32_t appPid) override;
    TraceRet CloseTrace(TraceScenario scenario) override;

protected:
    std::string GetTag() override
    {
        return "CloseState";
    }
};
}
#endif //HIVIEWDFX_HIVIEW_TRACE_BASE_STATE_H
