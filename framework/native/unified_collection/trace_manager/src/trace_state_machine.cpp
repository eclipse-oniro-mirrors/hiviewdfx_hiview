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
#include "trace_state_machine.h"

#include "hiview_logger.h"
#include "unistd.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("TraceStateMachine");
const std::vector<std::string> TAG_GROUPS = {"scene_performance"};
}

TraceStateMachine::TraceStateMachine()
{
    currentState_ = std::make_shared<CloseState>();
}

TraceRet TraceStateMachine::OpenDynamicTrace(int32_t appid)
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    return currentState_->OpenAppTrace(appid);
}

TraceRet TraceStateMachine::DumpTrace(TraceScenario scenario, int maxDuration, uint64_t happenTime, TraceRetInfo &info)
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    auto ret = currentState_->DumpTrace(scenario, maxDuration, happenTime, info);
    if (ret.GetCodeError() == TraceErrorCode::TRACE_IS_OCCUPIED) {
        RecoverState();
    }
    return ret;
}

TraceRet TraceStateMachine::OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups)
{
    HIVIEW_LOGI("TraceStateMachine OpenTrace scenario:%{public}d", static_cast<int>(scenario));
    std::lock_guard<std::mutex> lock(traceMutex_);
    return currentState_->OpenTrace(scenario, tagGroups);
}

TraceRet TraceStateMachine::OpenTrace(TraceScenario scenario, const std::string &args)
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    HIVIEW_LOGI("TraceStateMachine OpenTrace scenario:%{public}d", static_cast<int>(scenario));
    return currentState_->OpenTrace(scenario, args);
}

TraceRet TraceStateMachine::TraceDropOn(TraceScenario scenario)
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    return currentState_->TraceDropOn(scenario);
}

TraceRet TraceStateMachine::TraceDropOff(TraceScenario scenario, TraceRetInfo &info)
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    return currentState_->TraceDropOff(scenario, info);
}

TraceRet TraceStateMachine::CloseTrace(TraceScenario scenario)
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    if (auto ret = currentState_->CloseTrace(scenario); !ret.IsSuccess()) {
        HIVIEW_LOGW("fail stateError:%{public}d, codeError%{public}d", static_cast<int >(ret.stateError_),
            ret.codeError_);
        return ret;
    }
    return RecoverState();
}

TraceRet TraceStateMachine::InitOrUpdateState()
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    if (Hitrace::GetTraceMode() != Hitrace::TraceMode::CLOSE) {
        if (auto ret = Hitrace::CloseTrace(); ret != TraceErrorCode::SUCCESS) {
            HIVIEW_LOGE("Hitrace close error:%{public}d", ret);
            return TraceRet(ret);
        }
    }
    return RecoverState();
}

void TraceStateMachine::TransToDynamicState(int32_t appPid)
{
    HIVIEW_LOGI("to app state");
    currentState_ = std::make_shared<DynamicState>(appPid);
}

void TraceStateMachine::TransToCommonState()
{
    HIVIEW_LOGI("to common state");
    currentState_ = std::make_shared<CommonState>();
}

void TraceStateMachine::TransToCommonDropState()
{
    HIVIEW_LOGI("to common drop state");
    currentState_ = std::make_shared<CommonDropState>();
}

void TraceStateMachine::TransToCloseState()
{
    HIVIEW_LOGI("to close state");
    currentState_ = std::make_shared<CloseState>();
}

void TraceStateMachine::TransToCommandState()
{
    HIVIEW_LOGI("to command state");
    SetCommandState(true);
    currentState_ = std::make_shared<CommandState>();
}

void TraceStateMachine::TransToCommandDropState()
{
    HIVIEW_LOGI("to command drop state");
    currentState_ = std::make_shared<CommandDropState>();
}

TraceRet TraceStateMachine::InitCommonDropState()
{
    if (auto ret = Hitrace::OpenTrace(TAG_GROUPS); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("OpenTrace error:%{public}d", ret);
        return TraceRet(ret);
    }
    if (auto retTraceOn = Hitrace::RecordTraceOn(); retTraceOn != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("DumpTraceOn error:%{public}d", retTraceOn);
        return TraceRet(retTraceOn);
    }
    TransToCommonDropState();
    return {};
}

TraceRet TraceStateMachine::InitCommonState()
{
    if (auto ret = Hitrace::OpenTrace(TAG_GROUPS); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("OpenTrace error:%{public}d", ret);
        return TraceRet(ret);
    }
    TransToCommonState();
    return {};
}

TraceRet TraceStateMachine::TraceCacheOff()
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    return currentState_->TraceCacheOff();
}

TraceRet TraceStateMachine::TraceCacheOn(uint64_t totalFileSize, uint64_t sliceMaxDuration)
{
    std::lock_guard<std::mutex> lock(traceMutex_);
    return currentState_->TraceCacheOn(totalFileSize, sliceMaxDuration);
}

TraceRet TraceStateMachine::RecoverState()
{
    // All switch is closed
    if (traceSwitchState_ == 0) {
        HIVIEW_LOGI("commercial version and all switch is closed, trans to CloseState");
        TransToCloseState();
        return {};
    }

    // If trace recording switch is open
    if ((traceSwitchState_ & 1) == 1) {
        return InitCommonDropState();
    }

    // trace froze or UCollection switch is open or isBetaVersion
    return InitCommonState();
}

TraceRet TraceBaseState::CloseTrace(TraceScenario scenario)
{
    if (TraceErrorCode ret = Hitrace::CloseTrace(); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("%{public}s: CloseTrace error:%{public}d", GetTag().c_str(), ret);
        return TraceRet(ret);
    }
    return {};
}

TraceRet TraceBaseState::OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups)
{
    if (auto ret = Hitrace::OpenTrace(tagGroups); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("%{public}s: scenario:%{public}d error:%{public}d", GetTag().c_str(), static_cast<int>(scenario),
            ret);
        return TraceRet(ret);
    }
    HIVIEW_LOGI("%{public}s: OpenTrace success with scenario:%{public}d", GetTag().c_str(), static_cast<int>(scenario));
    switch (scenario) {
        case TraceScenario::TRACE_COMMAND:
            TraceStateMachine::GetInstance().TransToCommandState();
            break;
        case TraceScenario::TRACE_COMMON:
            TraceStateMachine::GetInstance().TransToCommonState();
            break;
        default:
            break;
    }
    return {};
}

TraceRet TraceBaseState::OpenTrace(TraceScenario scenario, const std::string &args)
{
    if (TraceErrorCode ret = Hitrace::OpenTrace(args); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("%{public}s: scenario:%{public}d error:%{public}d", GetTag().c_str(), static_cast<int>(scenario),
            ret);
        return TraceRet(ret);
    }
    HIVIEW_LOGI("%{public}s: OpenTrace success with scenario:%{public}d", GetTag().c_str(), static_cast<int>(scenario));
    switch (scenario) {
        case TraceScenario::TRACE_COMMAND:
            TraceStateMachine::GetInstance().TransToCommandState();
            break;
        case TraceScenario::TRACE_COMMON:
            TraceStateMachine::GetInstance().TransToCommonState();
            break;
        default:
            break;
    }
    return {};
}

TraceRet TraceBaseState::TraceCacheOn(uint64_t totalFileSize, uint64_t sliceMaxDuration)
{
    HIVIEW_LOGW("%{public}s: invoke state fail", GetTag().c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::TraceCacheOff()
{
    HIVIEW_LOGW("%{public}s: invoke state fail", GetTag().c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::OpenAppTrace(int32_t appPid)
{
    HIVIEW_LOGW("%{public}s: invoke state deny", GetTag().c_str());
    return TraceRet(TraceStateCode::DENY);
}

TraceRet TraceBaseState::DumpTrace(TraceScenario scenario, int maxDuration, uint64_t happenTime, TraceRetInfo &info)
{
    HIVIEW_LOGW("%{public}s: scenario:%{public}d is fail", GetTag().c_str(), static_cast<int>(scenario));
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::TraceDropOn(TraceScenario scenario)
{
    HIVIEW_LOGW("%{public}s: scenario:%{public}d is fail", GetTag().c_str(), static_cast<int>(scenario));
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet TraceBaseState::TraceDropOff(TraceScenario scenario, TraceRetInfo &info)
{
    HIVIEW_LOGW("%{public}s: scenario:%{public}d is fail", GetTag().c_str(), static_cast<int>(scenario));
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet CommandState::OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups)
{
    HIVIEW_LOGW("%{public}s: scenario:%{public}d is deny", GetTag().c_str(), static_cast<int>(scenario));
    return TraceRet(TraceStateCode::DENY);
}

TraceRet CommandState::OpenTrace(TraceScenario scenario, const std::string &args)
{
    HIVIEW_LOGW("%{public}s: scenario:%{public}d is deny", GetTag().c_str(), static_cast<int>(scenario));
    return TraceRet(TraceStateCode::DENY);
}

TraceRet CommandState::DumpTrace(TraceScenario scenario, int maxDuration, uint64_t happenTime, TraceRetInfo &info)
{
    if (scenario != TraceScenario::TRACE_COMMAND) {
        HIVIEW_LOGW("%{public}s: scenario:%{public}d is fail", GetTag().c_str(), static_cast<int>(scenario));
        return TraceRet(TraceStateCode::FAIL);
    }
    info = Hitrace::DumpTrace(maxDuration, happenTime);
    HIVIEW_LOGI("%{public}s:  DumpTrace result:%{public}d", GetTag().c_str(), info.errorCode);
    return TraceRet(info.errorCode);
}

TraceRet CommandState::TraceDropOn(TraceScenario scenario)
{
    if (scenario != TraceScenario::TRACE_COMMAND) {
        HIVIEW_LOGW("%{public}s: scenario:%{public}d is deny", GetTag().c_str(), static_cast<int>(scenario));
        return TraceRet(TraceStateCode::FAIL);
    }
    if (TraceErrorCode ret = Hitrace::RecordTraceOn(); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("%{public}s:  TraceDropOn error:%{public}d", GetTag().c_str(), ret);
        return TraceRet(ret);
    }
    TraceStateMachine::GetInstance().TransToCommandDropState();
    return {};
}

TraceRet CommandState::CloseTrace(TraceScenario scenario)
{
    if (scenario != TraceScenario::TRACE_COMMAND) {
        HIVIEW_LOGW("%{public}s: scenario:%{public}d is deny", GetTag().c_str(), static_cast<int>(scenario));
        return TraceRet(TraceStateCode::DENY);
    }
    auto ret = TraceBaseState::CloseTrace(scenario);
    if (ret.IsSuccess()) {
        TraceStateMachine::GetInstance().SetCommandState(false);
    }
    return ret;
}

TraceRet CommandDropState::OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups)
{
    HIVIEW_LOGW("%{public}s: scenario:%{public}d is deny", GetTag().c_str(), static_cast<int>(scenario));
    return TraceRet(TraceStateCode::DENY);
}

TraceRet CommandDropState::OpenTrace(TraceScenario scenario, const std::string &args)
{
    HIVIEW_LOGW("%{public}s: scenario:%{public}d is deny", GetTag().c_str(), static_cast<int>(scenario));
    return TraceRet(TraceStateCode::DENY);
}

TraceRet CommandDropState::TraceDropOff(TraceScenario scenario, TraceRetInfo &info)
{
    if (scenario != TraceScenario::TRACE_COMMAND) {
        HIVIEW_LOGW("%{public}s: scenario:%{public}d is fail", GetTag().c_str(), static_cast<int>(scenario));
        return TraceRet(TraceStateCode::FAIL);
    }
    if (info = Hitrace::RecordTraceOff(); info.errorCode != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("%{public}s: TraceDropOff error:%{public}d", GetTag().c_str(), info.errorCode);
        return TraceRet(info.errorCode);
    }
    TraceStateMachine::GetInstance().TransToCommandState();
    return {};
}

TraceRet CommandDropState::CloseTrace(TraceScenario scenario)
{
    if (scenario == TraceScenario::TRACE_COMMAND) {
        return TraceBaseState::CloseTrace(TraceScenario::TRACE_COMMAND);
    }
    HIVIEW_LOGW("%{public}s: invoke state fail", GetTag().c_str());
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet CommonState::OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups)
{
    if (scenario != TraceScenario::TRACE_COMMAND) {
        HIVIEW_LOGW("%{public}s: invoke state deny", GetTag().c_str());
        return TraceRet(TraceStateCode::DENY);
    }

    // If is command open, first close common state trace
    if (auto closeRet = Hitrace::CloseTrace(); closeRet != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("%{public}s: CloseTrace error:%{public}d", GetTag().c_str(), closeRet);
        return TraceRet(closeRet);
    }

    // Second open command state trace
    return TraceBaseState::OpenTrace(TraceScenario::TRACE_COMMAND, tagGroups);
}

TraceRet CommonState::OpenTrace(TraceScenario scenario, const std::string &args)
{
    if (scenario != TraceScenario::TRACE_COMMAND) {
        HIVIEW_LOGW("%{public}s: invoke state deny", GetTag().c_str());
        return TraceRet(TraceStateCode::DENY);
    }

    // Is command open, first close common state trace
    if (auto closeRet = Hitrace::CloseTrace(); closeRet != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("%{public}s: CloseTrace error:%{public}d", GetTag().c_str(), closeRet);
        return TraceRet(closeRet);
    }

    // Second open command state trace
    return TraceBaseState::OpenTrace(TraceScenario::TRACE_COMMAND, args);
}

TraceRet CommonState::DumpTrace(TraceScenario scenario, int maxDuration, uint64_t happenTime, TraceRetInfo &info)
{
    if (scenario == TraceScenario::TRACE_COMMON || scenario == TraceScenario::TRACE_COMMAND) {
        HIVIEW_LOGI("%{public}s: DumpTrace result:%{public}d", GetTag().c_str(), info.errorCode);
        info = Hitrace::DumpTrace(maxDuration, happenTime);
        return TraceRet(info.errorCode);
    }
    HIVIEW_LOGW("%{public}s: scenario:%{public}d is fail", GetTag().c_str(), static_cast<int>(scenario));
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet CommonState::TraceDropOn(TraceScenario scenario)
{
    if (scenario != TraceScenario::TRACE_COMMON) {
        HIVIEW_LOGW("%{public}s: scenario:%{public}d is fail", GetTag().c_str(), static_cast<int>(scenario));
        return TraceRet(TraceStateCode::FAIL);
    }
    if (auto ret = Hitrace::RecordTraceOn(); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("%{public}s: TraceDropOn error:%{public}d", GetTag().c_str(), ret);
        return TraceRet(ret);
    }
    TraceStateMachine::GetInstance().TransToCommonDropState();
    return {};
}

TraceRet CommonState::CloseTrace(TraceScenario scenario)
{
    if (scenario == TraceScenario::TRACE_COMMON) {
        return TraceBaseState::CloseTrace(scenario);
    }

    // beta version can close trace by trace command
    if (scenario == TraceScenario::TRACE_COMMAND && !TraceStateMachine::GetInstance().GetCommandState()) {
        // Prevent traceStateMachine recovery to beta common state after close
        TraceStateMachine::GetInstance().CloseVersionBeta();
        return TraceBaseState::CloseTrace(scenario);
    }
    HIVIEW_LOGW("%{public}s: scenario:%{public}d is fail", GetTag().c_str(), static_cast<int>(scenario));
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet CommonState::TraceCacheOn(uint64_t totalFileSize, uint64_t sliceMaxDuration)
{
    return TraceRet(Hitrace::CacheTraceOn(totalFileSize, sliceMaxDuration));
}

TraceRet CommonState::TraceCacheOff()
{
    return TraceRet(Hitrace::CacheTraceOff());
}

TraceRet CommonDropState::OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups)
{
    if (scenario != TraceScenario::TRACE_COMMAND) {
        HIVIEW_LOGW("%{public}s: scenario:%{public}d is deny", GetTag().c_str(), static_cast<int>(scenario));
        return TraceRet(TraceStateCode::DENY);
    }
    if (auto dropRet = Hitrace::RecordTraceOff(); dropRet.errorCode != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("%{public}s: DumpTraceOff error:%{public}d", GetTag().c_str(), dropRet.errorCode);
        return TraceRet(dropRet.errorCode);
    }
    if (auto closeRet = Hitrace::CloseTrace(); closeRet != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("%{public}s: CloseTrace error:%{public}d", GetTag().c_str(), closeRet);
        TraceStateMachine::GetInstance().TransToCommonState();
        return TraceRet(closeRet);
    }
    return TraceBaseState::OpenTrace(TraceScenario::TRACE_COMMAND, tagGroups);
}

TraceRet CommonDropState::OpenTrace(TraceScenario scenario, const std::string &args)
{
    if (scenario != TraceScenario::TRACE_COMMAND) {
        HIVIEW_LOGW("%{public}s: scenario:%{public}d is deny", GetTag().c_str(), static_cast<int>(scenario));
        return TraceRet(TraceStateCode::DENY);
    }
    if (auto dropRet = Hitrace::RecordTraceOff(); dropRet.errorCode != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("%{public}s: DumpTraceOff error:%{public}d", GetTag().c_str(), dropRet.errorCode);
        return TraceRet(dropRet.errorCode);
    }
    if (auto closeRet = Hitrace::CloseTrace(); closeRet != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("%{public}s: CloseTrace error:%{public}d", GetTag().c_str(), closeRet);
        TraceStateMachine::GetInstance().TransToCommonState();
        return TraceRet(closeRet);
    }
    return TraceBaseState::OpenTrace(TraceScenario::TRACE_COMMAND, args);
}

TraceRet CommonDropState::TraceDropOff(TraceScenario scenario, TraceRetInfo &info)
{
    if (scenario != TraceScenario::TRACE_COMMON) {
        HIVIEW_LOGW("%{public}s: scenario:%{public}d is fail", GetTag().c_str(), static_cast<int>(scenario));
        return TraceRet(TraceStateCode::FAIL);
    }
    if (info = Hitrace::RecordTraceOff(); info.errorCode != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("%{public}s: TraceDropOff error:%{public}d", GetTag().c_str(), info.errorCode);
        return TraceRet(info.errorCode);
    }
    TraceStateMachine::GetInstance().TransToCommonState();
    return {};
}

TraceRet CommonDropState::CloseTrace(TraceScenario scenario)
{
    return TraceRet(TraceStateCode::FAIL);
}

TraceRet DynamicState::DumpTrace(TraceScenario scenario, int maxDuration, uint64_t happenTime, TraceRetInfo &info)
{
    if (scenario != TraceScenario::TRACE_DYNAMIC) {
        HIVIEW_LOGW("%{public}s: scenario:%{public}d is fail", GetTag().c_str(), static_cast<int>(scenario));
        return TraceRet(TraceStateCode::FAIL);
    }
    info = Hitrace::DumpTrace(maxDuration, happenTime);
    HIVIEW_LOGI("%{public}s: DumpTrace result:%{public}d", GetTag().c_str(), info.errorCode);
    return TraceRet(info.errorCode);
}

TraceRet DynamicState::CloseTrace(TraceScenario scenario)
{
    if (scenario != TraceScenario::TRACE_DYNAMIC) {
        HIVIEW_LOGW("%{public}s: scenario:%{public}d is fail", GetTag().c_str(), static_cast<int>(scenario));
        return TraceRet(TraceStateCode::FAIL);
    }
    return TraceBaseState::CloseTrace(scenario);
}

TraceRet DynamicState::OpenTrace(TraceScenario scenario, const std::vector<std::string> &tagGroups)
{
    // OpenTrace interface only invoke in command or common scenario, so excute switching state directly
    if (auto closeRet = Hitrace::CloseTrace(); closeRet != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("%{public}s:  CloseTrace result:%{public}d", GetTag().c_str(), closeRet);
        return TraceRet(closeRet);
    }
    return TraceBaseState::OpenTrace(scenario, tagGroups);
}

TraceRet DynamicState::OpenTrace(TraceScenario scenario, const std::string &args)
{
    // OpenTrace interface only invoke in command or common scenario, so excute switching state directly
    if (auto closeRet = Hitrace::CloseTrace(); closeRet != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("%{public}s: CloseTrace result:%{public}d", GetTag().c_str(), closeRet);
        return TraceRet(closeRet);
    }
    return TraceBaseState::OpenTrace(scenario, args);
}

TraceRet DynamicState::OpenAppTrace(int32_t appPid)
{
    HIVIEW_LOGW("DynamicState already open, occupied by appid:%{public}d", appPid);
    return TraceRet(TraceStateCode::DENY);
}

TraceRet CloseState::OpenAppTrace(int32_t appPid)
{
    std::string appArgs = "tags:graphic,ace,app clockType:boot bufferSize:10240 overwrite:1 fileLimit:20 ";
    appArgs.append("appPid:").append(std::to_string(appPid));
    if (auto ret = Hitrace::OpenTrace(appArgs); ret != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("%{public}s: args:%{public}s, result:%{public}d", GetTag().c_str(), appArgs.c_str(), ret);
        return TraceRet(ret);
    }
    TraceStateMachine::GetInstance().TransToDynamicState(appPid);
    return {};
}

TraceRet CloseState::CloseTrace(TraceScenario scenario)
{
    return {};
}
}
}
