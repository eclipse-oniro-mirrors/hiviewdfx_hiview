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
#include "trace_utils.h"

#include <algorithm>
#include <chrono>
#include <unistd.h>
#include <vector>

#include "cpu_collector.h"
#include "file_util.h"
#include "ffrt.h"
#include "hiview_logger.h"
#include "hiview_event_report.h"
#include "memory_collector.h"
#include "parameter_ex.h"
#include "securec.h"
#include "string_util.h"
#include "trace_common.h"
#include "trace_worker.h"
#include "time_util.h"

using namespace std::chrono_literals;
using OHOS::HiviewDFX::TraceWorker;

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("UCollectUtil-TraceCollector");
const std::string UNIFIED_SHARE_PATH = "/data/log/hiview/unified_collection/trace/share/";
const std::string UNIFIED_SPECIAL_PATH = "/data/log/hiview/unified_collection/trace/special/";
const std::string UNIFIED_SHARE_TEMP_PATH = UNIFIED_SHARE_PATH + "temp/";
constexpr uint32_t READ_MORE_LENGTH = 100 * 1024;
const double CPU_LOAD_THRESHOLD = 0.03;
const uint32_t MAX_TRY_COUNT = 6;
constexpr uint32_t MB_TO_KB = 1024;
constexpr uint32_t KB_TO_BYTE = 1024;
}

UcError TransCodeToUcError(TraceErrorCode ret)
{
    if (CODE_MAP.find(ret) == CODE_MAP.end()) {
        HIVIEW_LOGE("ErrorCode is not exists.");
        return UcError::UNSUPPORT;
    } else {
        return CODE_MAP.at(ret);
    }
}

UcError TransStateToUcError(TraceStateCode ret)
{
    if (TRACE_STATE_MAP.find(ret) == TRACE_STATE_MAP.end()) {
        HIVIEW_LOGE("ErrorCode is not exists.");
        return UcError::UNSUPPORT;
    } else {
        return TRACE_STATE_MAP.at(ret);
    }
}

UcError TransFlowToUcError(TraceFlowCode ret)
{
    if (TRACE_FLOW_MAP.find(ret) == TRACE_FLOW_MAP.end()) {
        HIVIEW_LOGE("ErrorCode is not exists.");
        return UcError::UNSUPPORT;
    } else {
        return TRACE_FLOW_MAP.at(ret);
    }
}

const std::string EnumToString(UCollect::TraceCaller &caller)
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
        case UCollect::TraceCaller::FOUNDATION:
            return CallerName::FOUNDATION;
        case UCollect::TraceCaller::OTHER:
            return CallerName::OTHER;
        case UCollect::TraceCaller::BETACLUB:
            return CallerName::BETACLUB;
        default:
            return "";
    }
}

const std::string ClientToString(UCollect::TraceClient &client)
{
    switch (client) {
        case UCollect::TraceClient::COMMAND:
            return ClientName::COMMAND;
        case UCollect::TraceClient::COMMON_DEV:
            return ClientName::COMMON_DEV;
        case UCollect::TraceClient::APP:
            return ClientName::APP;
        default:
            return "";
    }
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

std::string AddVersionInfoToZipName(const std::string &srcZipPath)
{
    std::string displayVersion = Parameter::GetDisplayVersionStr();
    std::string versionStr = StringUtil::ReplaceStr(StringUtil::ReplaceStr(displayVersion, "_", "-"), " ", "_");
    return StringUtil::ReplaceStr(srcZipPath, ".zip", "@" + versionStr + ".zip");
}

void ZipTraceFile(const std::string &srcSysPath, const std::string &destZipPath)
{
    HIVIEW_LOGI("start ZipTraceFile src: %{public}s, dst: %{public}s", srcSysPath.c_str(), destZipPath.c_str());
    FILE *srcFp = fopen(srcSysPath.c_str(), "rb");
    if (srcFp == nullptr) {
        return;
    }
    zip_fileinfo zipInfo;
    errno_t result = memset_s(&zipInfo, sizeof(zipInfo), 0, sizeof(zipInfo));
    if (result != EOK) {
        (void)fclose(srcFp);
        return;
    }
    std::string zipFileName = FileUtil::ExtractFileName(destZipPath);
    zipFile zipFile = zipOpen((UNIFIED_SHARE_TEMP_PATH + zipFileName).c_str(), APPEND_STATUS_CREATE);
    if (zipFile == nullptr) {
        HIVIEW_LOGE("zipOpen failed");
        (void)fclose(srcFp);
        return;
    }
    CheckCurrentCpuLoad();
    HiviewEventReport::ReportCpuScene("5");
    std::string sysFileName = FileUtil::ExtractFileName(srcSysPath);
    zipOpenNewFileInZip(
        zipFile, sysFileName.c_str(), &zipInfo, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
    int errcode = 0;
    char buf[READ_MORE_LENGTH] = {0};
    while (!feof(srcFp)) {
        size_t numBytes = fread(buf, 1, sizeof(buf), srcFp);
        if (numBytes <= 0) {
            HIVIEW_LOGE("zip file failed, size is zero");
            errcode = -1;
            break;
        }
        zipWriteInFileInZip(zipFile, buf, static_cast<unsigned int>(numBytes));
        if (ferror(srcFp)) {
            HIVIEW_LOGE("zip file failed: %{public}s, errno: %{public}d.", srcSysPath.c_str(), errno);
            errcode = -1;
            break;
        }
    }
    (void)fclose(srcFp);
    zipCloseFileInZip(zipFile);
    zipClose(zipFile, nullptr);
    if (errcode != 0) {
        return;
    }
    std::string destZipPathWithVersion = AddVersionInfoToZipName(destZipPath);
    FileUtil::RenameFile(UNIFIED_SHARE_TEMP_PATH + zipFileName, destZipPathWithVersion);
    HIVIEW_LOGI("finish rename file %{public}s", destZipPathWithVersion.c_str());
}

void CopyFile(const std::string &src, const std::string &dst)
{
    int ret = FileUtil::CopyFile(src, dst);
    if (ret != 0) {
        HIVIEW_LOGE("copy file failed, file is %{public}s.", src.c_str());
    }
}

/*
 * apply to xperf, xpower and reliability
 * trace path eg.:
 *     /data/log/hiview/unified_collection/trace/share/
 *     trace_20230906111617@8290-81765922_{device}_{version}.zip
*/
std::vector<std::string> GetUnifiedShareFiles(const std::vector<std::string> outputFiles)
{
    if (!FileUtil::FileExists(UNIFIED_SHARE_TEMP_PATH)) {
        if (!CreateMultiDirectory(UNIFIED_SHARE_TEMP_PATH)) {
            HIVIEW_LOGE("failed to create multidirectory.");
            return {};
        }
    }

    std::vector<std::string> files;
    for (const auto &tracePath : outputFiles) {
        std::string traceFile = FileUtil::ExtractFileName(tracePath);
        const std::string destZipPath = UNIFIED_SHARE_PATH + StringUtil::ReplaceStr(traceFile, ".sys", ".zip");
        const std::string tempDestZipPath = UNIFIED_SHARE_TEMP_PATH + FileUtil::ExtractFileName(destZipPath);
        const std::string destZipPathWithVersion = AddVersionInfoToZipName(destZipPath);
        // for zip if the file has not been compressed
        if (!FileUtil::FileExists(destZipPathWithVersion) && !FileUtil::FileExists(tempDestZipPath)) {
            // new empty file is used to restore tasks in queue
            FileUtil::SaveStringToFile(tempDestZipPath, " ", true);
            UcollectionTask traceTask = [=]() {
                ZipTraceFile(tracePath, destZipPath);
            };
            TraceWorker::GetInstance().HandleUcollectionTask(traceTask);
        }
        files.push_back(destZipPathWithVersion);
        HIVIEW_LOGI("trace file : %{public}s.", destZipPathWithVersion.c_str());
    }
    return files;
}

/*
 * apply to BetaClub and Other Scenes
 * trace path eg.:
 * /data/log/hiview/unified_collection/trace/special/BetaClub_trace_20230906111633@8306-299900816.sys
*/
std::vector<std::string> GetUnifiedSpecialFiles(const std::vector<std::string>& outputFiles, const std::string& prefix)
{
    if (!FileUtil::FileExists(UNIFIED_SPECIAL_PATH)) {
        if (!CreateMultiDirectory(UNIFIED_SPECIAL_PATH)) {
            HIVIEW_LOGE("create dir %{public}s fail", UNIFIED_SPECIAL_PATH.c_str());
            return {};
        }
    }

    std::vector<std::string> files;
    for (const auto &trace :outputFiles) {
        std::string traceFile = FileUtil::ExtractFileName(trace);
        const std::string dst = UNIFIED_SPECIAL_PATH + prefix + "_" + traceFile;
        // for copy if the file has not been copied
        if (!FileUtil::FileExists(dst)) {
            UcollectionTask traceTask = [=]() {
                CopyFile(trace, dst);
            };
            TraceWorker::GetInstance().HandleUcollectionTask(traceTask);
        }
        files.push_back(dst);
        HIVIEW_LOGI("trace file : %{public}s.", dst.c_str());
    }
    return files;
}

int64_t GetTraceSize(TraceRetInfo &ret)
{
    struct stat fileInfo;
    int64_t traceSize = 0;
    for (const auto &tracePath : ret.outputFiles) {
        int ret = stat(tracePath.c_str(), &fileInfo);
        if (ret != 0) {
            HIVIEW_LOGE("%{public}s is not exists, ret = %{public}d.", tracePath.c_str(), ret);
            continue;
        }
        traceSize += fileInfo.st_size;
    }
    return traceSize;
}

void WriteDumpTraceHisysevent(DumpEvent &dumpEvent, int32_t retCode)
{
    LoadMemoryInfo(dumpEvent);
    dumpEvent.errorCode = retCode;
    int ret = HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::PROFILER, "DUMP_TRACE",
        OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        "CALLER", dumpEvent.caller,
        "ERROR_CODE", dumpEvent.errorCode,
        "IPC_TIME", dumpEvent.ipcTime,
        "REQ_TIME", dumpEvent.reqTime,
        "REQ_DURATION", dumpEvent.reqDuration,
        "EXEC_TIME", dumpEvent.execTime,
        "EXEC_DURATION", dumpEvent.execDuration,
        "COVER_DURATION", dumpEvent.coverDuration,
        "COVER_RATIO", dumpEvent.coverRatio,
        "TAGS", dumpEvent.tags,
        "FILE_SIZE", dumpEvent.fileSize,
        "SYS_MEM_TOTAL", dumpEvent.sysMemTotal,
        "SYS_MEM_FREE", dumpEvent.sysMemFree,
        "SYS_MEM_AVAIL", dumpEvent.sysMemAvail,
        "SYS_CPU", dumpEvent.sysCpu);
    if (ret != 0) {
        HIVIEW_LOGE("HiSysEventWrite failed, ret is %{public}d", ret);
    }
}

void LoadMemoryInfo(DumpEvent &dumpEvent)
{
    std::shared_ptr<UCollectUtil::MemoryCollector> collector = UCollectUtil::MemoryCollector::Create();
    CollectResult<SysMemory> data = collector->CollectSysMemory();
    dumpEvent.sysMemTotal = data.data.memTotal / MB_TO_KB;
    dumpEvent.sysMemFree = data.data.memFree / MB_TO_KB;
    dumpEvent.sysMemAvail = data.data.memAvailable / MB_TO_KB;
}

UcError GetUcError(TraceRet ret)
{
    if (ret.stateError_ != TraceStateCode::SUCCESS) {
        return TransStateToUcError(ret.stateError_);
    } else if (ret.codeError_!= TraceErrorCode::SUCCESS) {
        return TransCodeToUcError(ret.codeError_);
    } else if (ret.flowError_ != TraceFlowCode::TRACE_ALLOW) {
        return TransFlowToUcError(ret.flowError_);
    } else {
        return UcError::SUCCESS;
    }
}

void CheckAndCreateDirectory(const std::string &tmpDirPath)
{
    if (!FileUtil::FileExists(tmpDirPath)) {
        if (FileUtil::ForceCreateDirectory(tmpDirPath, FileUtil::FILE_PERM_775)) {
            HIVIEW_LOGD("create listener log directory %{public}s succeed.", tmpDirPath.c_str());
        } else {
            HIVIEW_LOGE("create listener log directory %{public}s failed.", tmpDirPath.c_str());
        }
    }
}

bool CreateMultiDirectory(const std::string &dirPath)
{
    uint32_t dirPathLen = dirPath.length();
    if (dirPathLen > PATH_MAX) {
        return false;
    }
    char tmpDirPath[PATH_MAX] = { 0 };
    for (uint32_t i = 0; i < dirPathLen; ++i) {
        tmpDirPath[i] = dirPath[i];
        if (tmpDirPath[i] == '/') {
            CheckAndCreateDirectory(tmpDirPath);
        }
    }
    return true;
}

void CreateTracePath(const std::string &filePath)
{
    if (FileUtil::FileExists(filePath)) {
        return;
    }
    if (!CreateMultiDirectory(filePath)) {
        HIVIEW_LOGE("failed to create multidirectory %{public}s.", filePath.c_str());
        return;
    }
}
} // HiViewDFX
} // OHOS
