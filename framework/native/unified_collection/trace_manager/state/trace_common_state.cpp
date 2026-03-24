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

#include "trace_common_state.h"

#include "hiview_logger.h"
#include "trace_state_machine.h"

namespace OHOS::HiviewDFX {
namespace {
DEFINE_LOG_TAG("TraceStateMachine");
}
CommonState::CommonState(bool isCachOn, int32_t totalFileSize, int32_t sliceMaxDuration)
    : totalFileSize_(totalFileSize), sliceMaxDuration_(sliceMaxDuration)
{
    if (isCachOn) {
        Hitrace::CacheTraceOn(totalFileSize_, sliceMaxDuration_);
    }
}

TraceRet CommonState::DumpTrace(const std::string& scenarioName, uint32_t maxDuration, uint64_t happenTime,
    TraceRetInfo &info)
{
    if (scenarioName == ScenarioName::COMMON_BETA || scenarioName == ScenarioName::COMMAND) {
        const auto& outputPath = TraceStateMachine::GetInstance().GetOutputPath();
        if (!outputPath.empty()) {
            HIVIEW_LOGI("CommonState, DumpTrace outputPath:%{public}s", outputPath.c_str());
        }
        info = Hitrace::DumpTrace(maxDuration, happenTime, outputPath);
        HIVIEW_LOGI("CommonState, DumpTrace result:%{public}d", info.errorCode);
        return TraceRet(info.errorCode);
    }
    HIVIEW_LOGW("CommonState scenario:%{public}s is fail", scenarioName.c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet CommonState::DumpTraceAsync(const DumpTraceArgs &args, int64_t fileSizeLimit,
    TraceRetInfo &info, DumpTraceCallback callback)
{
    if (args.scenarioName != ScenarioName::COMMON_BETA) {
        HIVIEW_LOGW("CommonState scenario:%{public}s is fail", args.scenarioName.c_str());
        return TraceRet(TraceStateCode::FAIL);
    }
    info = Hitrace::DumpTraceAsync(args.maxDuration, args.happenTime, fileSizeLimit, callback);
    HIVIEW_LOGI("CommonState DumpTrace result:%{public}d", info.errorCode);
    return TraceRet(info.errorCode);
}

TraceRet CommonState::CloseTrace(const std::string& scenarioName)
{
    if (scenarioName == ScenarioName::COMMON_BETA) {
        return TraceBaseState::CloseTrace(scenarioName);
    }

    // beta version can close trace by trace command
    if (scenarioName == ScenarioName::COMMAND && !TraceStateMachine::GetInstance().GetCommandState()) {
        // Prevent traceStateMachine recovery to beta common state after close
        TraceStateMachine::GetInstance().CloseVersionBeta();
        HIVIEW_LOGI("closed common beta from command");
        return TraceBaseState::CloseTrace(ScenarioName::COMMON_BETA);
    }
    HIVIEW_LOGW("CommonState scenario:%{public}s is fail", scenarioName.c_str());
    return TraceRet(TraceStateCode::DENY);
}

TraceRet CommonState::TraceCacheOn()
{
    return TraceRet(Hitrace::CacheTraceOn(totalFileSize_, sliceMaxDuration_));
}

TraceRet CommonState::TraceCacheOff()
{
    return TraceRet(Hitrace::CacheTraceOff());
}

TraceRet CommonState::SetCacheParams(int32_t totalFileSize, int32_t sliceMaxDuration)
{
    totalFileSize_ = totalFileSize;
    sliceMaxDuration_ = sliceMaxDuration;
    return {};
}
}
