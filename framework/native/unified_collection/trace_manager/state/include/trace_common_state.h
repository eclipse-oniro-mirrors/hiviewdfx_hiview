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
#ifndef HIVIEWDFX_HIVIEW_TRACE_COMMON_STATE_H
#define HIVIEWDFX_HIVIEW_TRACE_COMMON_STATE_H
#include <memory>

#include "trace_common.h"
#include "trace_base_state.h"
namespace OHOS::HiviewDFX {
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
}
#endif //HIVIEWDFX_HIVIEW_TRACE_COMMON_STATE_H
