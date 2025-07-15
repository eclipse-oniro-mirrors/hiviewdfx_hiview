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

#ifndef HIVIEWDFX_HIVIEW_TRACE_DYNAMIC_STATE_H
#define HIVIEWDFX_HIVIEW_TRACE_DYNAMIC_STATE_H

#include "time_util.h"
#include "trace_common.h"
#include "trace_base_state.h"

namespace OHOS::HiviewDFX {
class DynamicState : public TraceBaseState {
public:
    explicit DynamicState(int32_t appPid) : appPid_(appPid)
    {
        taskBeginTime_ = TimeUtil::GetMilliseconds();
    }

    TraceRet OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups) override;
    TraceRet OpenTrace(TraceScenario scenario, const std::string &args) override;
    TraceRet OpenAppTrace(int32_t appPid) override;
    TraceRet OpenTelemetryTrace(const std::string &args, TelemetryPolicy policy) override;
    TraceRet DumpTrace(TraceScenario scenario, uint32_t maxDuration, uint64_t happenTime, TraceRetInfo &info) override;
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
}
#endif //HIVIEWDFX_HIVIEW_TRACE_DYNAMIC_STATE_H
