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
#ifndef FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_FILE_UTILS_H
#define FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_FILE_UTILS_H

#include <string>

#include "collect_result.h"
#include "trace_caller.h"
#include "trace_common.h"
#include "trace_flow_controller.h"

using OHOS::HiviewDFX::Hitrace::TraceErrorCode;
using OHOS::HiviewDFX::UCollect::UcError;

namespace OHOS {
namespace HiviewDFX {

extern const char* const UNIFIED_SHARE_PATH;
extern const char* const UNIFIED_SPECIAL_PATH;
extern const char* const UNIFIED_TELEMETRY_PATH;
extern const char* const UNIFIED_SHARE_TEMP_PATH;

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
    int64_t fileSize = 0;
    int32_t sysMemTotal = 0;
    int32_t sysMemFree = 0;
    int32_t sysMemAvail = 0;
    int32_t sysCpu = 0;
    uint8_t traceMode = 0;
};

UcError TransCodeToUcError(TraceErrorCode ret);
UcError TransStateToUcError(TraceStateCode ret);
UcError TransFlowToUcError(TraceFlowCode ret);
const std::string EnumToString(UCollect::TraceCaller caller);
const std::string ClientToString(UCollect::TraceClient client);
const std::string ModuleToString(UCollect::TeleModule module);
UcError GetUcError(TraceRet ret);
std::string AddVersionInfoToZipName(const std::string &srcZipPath);
void CheckCurrentCpuLoad();
} // HiViewDFX
} // OHOS
#endif // FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_FILE_UTILS_H
