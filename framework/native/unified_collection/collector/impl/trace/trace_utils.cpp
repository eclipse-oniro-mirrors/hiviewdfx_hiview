/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#include "trace_utils.h"

#include <algorithm>
#include <chrono>

#include "file_util.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "string_util.h"
#include "cpu_collector.h"
#include "ffrt.h"

using namespace std::chrono_literals;
namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("UCollectUtil-TraceCollector");
const double CPU_LOAD_THRESHOLD = 0.03;
const uint32_t MAX_TRY_COUNT = 6;

const std::map<TraceErrorCode, UcError> CODE_MAP = {
    {TraceErrorCode::SUCCESS, UcError::SUCCESS},
    {TraceErrorCode::TRACE_NOT_SUPPORTED, UcError::UNSUPPORT},
    {TraceErrorCode::TRACE_IS_OCCUPIED, UcError::TRACE_IS_OCCUPIED},
    {TraceErrorCode::TAG_ERROR, UcError::TRACE_TAG_ERROR},
    {TraceErrorCode::FILE_ERROR, UcError::TRACE_FILE_ERROR},
    {TraceErrorCode::WRITE_TRACE_INFO_ERROR, UcError::TRACE_WRITE_ERROR},
    {TraceErrorCode::WRONG_TRACE_MODE, UcError::TRACE_WRONG_MODE},
    {TraceErrorCode::OUT_OF_TIME, UcError::TRACE_OUT_OF_TIME},
    {TraceErrorCode::FORK_ERROR, UcError::TRACE_FORK_ERROR},
    {TraceErrorCode::EPOLL_WAIT_ERROR, UcError::TRACE_EPOLL_WAIT_ERROR},
    {TraceErrorCode::INVALID_MAX_DURATION, UcError::TRACE_INVALID_MAX_DURATION},
    {TraceErrorCode::ASYNC_DUMP, UcError::ASYNC_DUMP},
    {TraceErrorCode::TRACE_TASK_SUBMIT_ERROR, UcError::TRACE_TASK_SUBMIT_ERROR},
    {TraceErrorCode::TRACE_TASK_DUMP_TIMEOUT, UcError::TRACE_TASK_DUMP_TIMEOUT},
};

const std::map<TraceStateCode, UcError> TRACE_STATE_MAP = {
    {TraceStateCode::SUCCESS, UcError::SUCCESS},
    {TraceStateCode::FAIL,    UcError::TRACE_STATE_ERROR},
    {TraceStateCode::DENY,    UcError::TRACE_OPEN_ERROR},
    {TraceStateCode::POLICY_ERROR,    UcError::TRACE_POLICY_ERROR},
    {TraceStateCode::UPDATE_TIME,    UcError::SUCCESS},
    {TraceStateCode::NO_TRIGGER,    UcError::TRACE_TELEMERTY_NO_TRIGER},
};

const std::map<TraceFlowCode, UcError> TRACE_FLOW_MAP = {
    {TraceFlowCode::TRACE_ALLOW, UcError::SUCCESS},
    {TraceFlowCode::TRACE_DUMP_DENY,    UcError::TRACE_DUMP_OVER_FLOW},
    {TraceFlowCode::TRACE_UPLOAD_DENY,    UcError::TRACE_OVER_FLOW},
    {TraceFlowCode::TRACE_HAS_CAPTURED_TRACE, UcError::HAD_CAPTURED_TRACE}
};
}

UcError TransCodeToUcError(TraceErrorCode ret)
{
    if (CODE_MAP.find(ret) == CODE_MAP.end()) {
        HIVIEW_LOGE("ErrorCode is not exists.");
        return UcError::UNSUPPORT;
    }
    return CODE_MAP.at(ret);
}

UcError TransStateToUcError(TraceStateCode ret)
{
    if (TRACE_STATE_MAP.find(ret) == TRACE_STATE_MAP.end()) {
        HIVIEW_LOGE("ErrorCode is not exists.");
        return UcError::UNSUPPORT;
    }
    return TRACE_STATE_MAP.at(ret);
}

UcError TransFlowToUcError(TraceFlowCode ret)
{
    if (TRACE_FLOW_MAP.find(ret) == TRACE_FLOW_MAP.end()) {
        HIVIEW_LOGE("ErrorCode is not exists.");
        return UcError::UNSUPPORT;
    }
    return TRACE_FLOW_MAP.at(ret);
}

const std::string ModuleToString(UCollect::TeleModule module)
{
    switch (module) {
        case UCollect::TeleModule::XPERF:
            return CallerName::XPERF;
        case UCollect::TeleModule::XPOWER:
            return CallerName::XPOWER;
        case UCollect::TeleModule::RELIABILITY:
            return CallerName::RELIABILITY;
        default:
            return "UNKNOWN";
    }
}

const std::string EnumToString(UCollect::TraceCaller caller)
{
    switch (caller) {
        case UCollect::TraceCaller::RELIABILITY:
            return CallerName::RELIABILITY;
        case UCollect::TraceCaller::XPERF:
            return CallerName::XPERF;
        case UCollect::TraceCaller::XPOWER:
            return CallerName::XPOWER;
        case UCollect::TraceCaller::HIVIEW:
            return CallerName::HIVIEW;
        case UCollect::TraceCaller::XPERF_EX:
            return CallerName::XPERF;
        case UCollect::TraceCaller::SCREEN:
            return CallerName::SCREEN;
        default:
            return "UNKNOWN";
    }
}

const std::string ClientToString(UCollect::TraceClient client)
{
    switch (client) {
        case UCollect::TraceClient::COMMAND:
            return ClientName::COMMAND;
        case UCollect::TraceClient::COMMON_DEV:
            return ClientName::COMMON_DEV;
        default:
            return "UNKNOWN";
    }
}

UcError GetUcError(TraceRet ret)
{
    if (ret.stateError_ != TraceStateCode::SUCCESS && ret.stateError_ != TraceStateCode::UPDATE_TIME) {
        return TransStateToUcError(ret.stateError_);
    }
    if (ret.codeError_!= TraceErrorCode::SUCCESS) {
        return TransCodeToUcError(ret.codeError_);
    }
    if (ret.flowError_ != TraceFlowCode::TRACE_ALLOW) {
        return TransFlowToUcError(ret.flowError_);
    }
    return UcError::SUCCESS;
}

std::string AddVersionInfoToZipName(const std::string &srcZipPath)
{
    std::string displayVersion = Parameter::GetDisplayVersionStr();
    std::string versionStr = StringUtil::ReplaceStr(StringUtil::ReplaceStr(displayVersion, "_", "-"), " ", "_");
    return StringUtil::ReplaceStr(srcZipPath, ".zip", "@" + versionStr + ".zip");
}

void CheckCurrentCpuLoad()
{
    std::shared_ptr<UCollectUtil::CpuCollector> collector = UCollectUtil::CpuCollector::Create();
    int32_t pid = getpid();
    auto collectResult = collector->CollectProcessCpuStatInfo(pid);
    HIVIEW_LOGI("first get cpu load %{public}f", collectResult.data.cpuLoad);
    uint32_t retryTime = 0;
    while (collectResult.data.cpuLoad > CPU_LOAD_THRESHOLD && retryTime < MAX_TRY_COUNT) {
        ffrt::this_task::sleep_for(5s);
        collectResult = collector->CollectProcessCpuStatInfo(pid);
        HIVIEW_LOGI("retry get cpu load %{public}f", collectResult.data.cpuLoad);
        retryTime++;
    }
}
} // HiViewDFX
} // OHOS
