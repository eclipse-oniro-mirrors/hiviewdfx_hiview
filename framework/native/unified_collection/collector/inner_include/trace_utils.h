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
#include <contrib/minizip/zip.h>

#include "hisysevent.h"
#include "hitrace_dump.h"
#include "trace_collector.h"

using OHOS::HiviewDFX::UCollectUtil::TraceCollector;
using OHOS::HiviewDFX::Hitrace::TraceErrorCode;
using OHOS::HiviewDFX::Hitrace::TraceRetInfo;
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
    {TraceErrorCode::SYSINFO_READ_FAILURE, UcError::TRACE_SYSINFO_READ_FAILURE},
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
    std::string tagGroup;
    int32_t fileSize = 0;
    int32_t sysMemTotal = 0;
    int32_t sysMemFree = 0;
    int32_t sysMemAvail = 0;
    int32_t sysCpu = 0;
    int32_t dumpCpu = 0;
};

UcError TransCodeToUcError(TraceErrorCode ret);
void FileRemove(UCollect::TraceCaller &caller);
void CheckAndCreateDirectory(const std::string &tmpDirPath);
bool CreateMultiDirectory(const std::string &dirPath);
const std::string EnumToString(UCollect::TraceCaller &caller);
std::vector<std::string> GetUnifiedFiles(Hitrace::TraceRetInfo ret, UCollect::TraceCaller &caller);
void CopyFile(const std::string &src, const std::string &dst);
std::vector<std::string> GetUnifiedShareFiles(Hitrace::TraceRetInfo ret, UCollect::TraceCaller &caller);
std::vector<std::string> GetUnifiedSpecialFiles(Hitrace::TraceRetInfo ret,
    UCollect::TraceCaller &caller);
void ZipTraceFile(const std::string &srcSysPath, const std::string &destZipPath);
std::string AddVersionInfoToZipName(const std::string &srcZipPath);
void CheckCurrentCpuLoad();
void WriteDumpTraceHisysevent(DumpEvent &dumpEvent);
void LoadMemoryInfo(DumpEvent &dumpEvent);
int64_t GetTraceSize(TraceRetInfo &ret);
} // HiViewDFX
} // OHOS
#endif // FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_FILE_UTILS_H
