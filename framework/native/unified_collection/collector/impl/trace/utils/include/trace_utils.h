/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
#ifndef FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_FILE_UTILS_H
#define FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_FILE_UTILS_H

#include <map>
#include <string>

#include "trace_collector.h"
#include "trace_state_machine.h"
#include "trace_flow_controller.h"

using OHOS::HiviewDFX::UCollectUtil::TraceCollector;
using OHOS::HiviewDFX::Hitrace::TraceErrorCode;
using OHOS::HiviewDFX::UCollect::UcError;

namespace OHOS {
namespace HiviewDFX {
namespace {
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
    {TraceErrorCode::PIPE_CREATE_ERROR, UcError::TRACE_PIPE_CREATE_ERROR},
    {TraceErrorCode::INVALID_MAX_DURATION, UcError::TRACE_INVALID_MAX_DURATION},
};

const std::map<TraceStateCode, UcError> TRACE_STATE_MAP = {
    {TraceStateCode::SUCCESS, UcError::SUCCESS},
    {TraceStateCode::FAIL,    UcError::TRACE_STATE_ERROR},
    {TraceStateCode::DENY,    UcError::TRACE_OPEN_ERROR},
};

const std::map<TraceFlowCode, UcError> TRACE_FLOW_MAP = {
    {TraceFlowCode::TRACE_ALLOW, UcError::SUCCESS},
    {TraceFlowCode::TRACE_DUMP_DENY,    UcError::TRACE_DUMP_OVER_FLOW},
    {TraceFlowCode::TRACE_UPLOAD_DENY,    UcError::TRACE_OVER_FLOW},
    {TraceFlowCode::TRACE_HAS_CAPTURED_TRACE, UcError::HAD_CAPTURED_TRACE}
};
}

struct DumpEvent {
    std::string caller;
    int32_t errorCode = 0;
    uint64_t ipcTime = 0;
    uint64_t reqTime = 0;
    int32_t reqDuration = 0;
    uint64_t execTime = 0;
    int32_t execDuration = 0;
    int32_t coverDuration = 0;
    int32_t coverRatio = 0;
    std::vector<std::string> tags;
    int32_t fileSize = 0;
    int32_t sysMemTotal = 0;
    int32_t sysMemFree = 0;
    int32_t sysMemAvail = 0;
    int32_t sysCpu = 0;
    uint8_t traceMode = 0;
};

namespace TelemetryEvent {
const std::string TELEMETRY_START = "telemetryStart";
const std::string TELEMETRY_STOP = "telemetryStop";
const std::string TELEMETRY_TIMEOUT = "telemetryTimeout";
}

UcError TransCodeToUcError(TraceErrorCode ret);
UcError TransStateToUcError(TraceStateCode ret);
UcError TransFlowToUcError(TraceFlowCode ret);
const std::string EnumToString(UCollect::TraceCaller caller);
const std::string ClientToString(UCollect::TraceClient client);
const std::string ModuleToString(UCollect::TeleModule module);
void CopyFile(const std::string &src, const std::string &dst);
std::vector<std::string> GetUnifiedZipFiles(const std::vector<std::string> outputFiles, const std::string &destDir);
std::vector<std::string> GetUnifiedSpecialFiles(const std::vector<std::string>& outputFiles, const std::string& prefix);
void ZipTraceFile(const std::string &srcSysPath, const std::string &destZipPath);
std::string AddVersionInfoToZipName(const std::string &srcZipPath);
void WriteDumpTraceHisysevent(DumpEvent &dumpEvent);
bool CreateMultiDirectory(const std::string &dirPath);
int64_t GetTraceSize(TraceRetInfo &ret);
UcError GetUcError(TraceRet ret);
bool ParseAndFilterTraceArgs(const std::unordered_set<std::string> &filterList, std::string &jsonArgs);
} // HiViewDFX
} // OHOS
#endif // FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_FILE_UTILS_H
