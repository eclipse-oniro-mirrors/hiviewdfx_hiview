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
#include <contrib/minizip/zip.h>
#include <unistd.h>
#include <vector>

#include "cpu_collector.h"
#include "hisysevent.h"
#include "file_util.h"
#include "ffrt.h"
#include "hiview_logger.h"
#include "hiview_event_report.h"
#include "hiview_zip_util.h"
#include "memory_collector.h"
#include "parameter_ex.h"
#include "securec.h"
#include "string_util.h"
#include "trace_decorator.h"
#include "trace_worker.h"
#include "time_util.h"

using namespace std::chrono_literals;
using OHOS::HiviewDFX::TraceWorker;

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("UCollectUtil-TraceCollector");
constexpr uint32_t READ_MORE_LENGTH = 100 * 1024;
const double CPU_LOAD_THRESHOLD = 0.03;
const uint32_t MAX_TRY_COUNT = 6;
constexpr uint32_t MB_TO_KB = 1024;
constexpr uint32_t KB_TO_BYTE = 1024;
const uint32_t UNIFIED_SHARE_COUNTS = 25;
const uint32_t UNIFIED_TELEMETRY_COUNTS = 20;
const uint32_t UNIFIED_APP_SHARE_COUNTS = 40;
const std::map<std::string, uint32_t> TRACE_COUNT_THRESHOLD_FOR_CALLER = {
    {CallerName::XPERF, 3}, {CallerName::RELIABILITY, 3},
    {CallerName::OTHER, 5}, {CallerName::SCREEN, 1}, {ClientName::BETACLUB, 2}
};

uint32_t GetTraceThreshold(const std::string &tracePath, const std::string &prefix)
{
    if (tracePath == UNIFIED_SHARE_PATH) {
        return prefix == ClientName::APP ? UNIFIED_APP_SHARE_COUNTS : UNIFIED_SHARE_COUNTS;
    } else if (tracePath == UNIFIED_TELEMETRY_PATH) {
        return UNIFIED_TELEMETRY_COUNTS;
    } else if (tracePath == UNIFIED_SPECIAL_PATH &&
        TRACE_COUNT_THRESHOLD_FOR_CALLER.find(prefix) != TRACE_COUNT_THRESHOLD_FOR_CALLER.end()) {
        return TRACE_COUNT_THRESHOLD_FOR_CALLER.at(prefix);
    } else {
        HIVIEW_LOGW("lack count config : %{public}s", prefix.c_str());
        return 0;
    }
}
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
        case UCollect::TraceCaller::OTHER:
            return CallerName::OTHER;
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
        case UCollect::TraceClient::BETACLUB:
            return ClientName::BETACLUB;
        default:
            return "UNKNOWN";
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

std::string GetTraceZipTmpPath(const std::string &tracePath)
{
    return UNIFIED_SHARE_TEMP_PATH + StringUtil::ReplaceStr(FileUtil::ExtractFileName(tracePath), ".sys", ".zip");
}

std::string GetTraceZipFinalPath(const std::string &tracePath, const std::string &destDir)
{
    auto destZipName = destDir + StringUtil::ReplaceStr(FileUtil::ExtractFileName(tracePath), ".sys", ".zip");
    return AddVersionInfoToZipName(destZipName);
}

std::string GetTraceSpecialPath(const std::string &tracePath, const std::string &prefix)
{
    return UNIFIED_SPECIAL_PATH + prefix + "_" + FileUtil::ExtractFileName(tracePath);
}

void ZipTraceFile(const std::string &srcSysPath, const std::string &destDir)
{
    std::string destZipPathWithVersion = GetTraceZipFinalPath(srcSysPath, destDir);
    std::string dstZipName = FileUtil::ExtractFileName(destZipPathWithVersion);
    if (FileUtil::FileExists(destZipPathWithVersion)) {
        HIVIEW_LOGI("dst: %{public}s already exist", dstZipName.c_str());
        return;
    }
    CheckCurrentCpuLoad();
    HiviewEventReport::ReportCpuScene("5");
    std::string tmpDestZipPath = GetTraceZipTmpPath(srcSysPath);
    HIVIEW_LOGI("start ZipTraceFile, dst: %{public}s", FileUtil::ExtractFileName(tmpDestZipPath).c_str());
    HiviewZipUnit zipUnit(tmpDestZipPath);
    if (int32_t ret = zipUnit.AddFileInZip(srcSysPath, ZipFileLevel::KEEP_NONE_PARENT_PATH); ret != 0) {
        HIVIEW_LOGW("zip trace failed, ret: %{public}d.", ret);
        return;
    }
    FileUtil::RenameFile(tmpDestZipPath, destZipPathWithVersion);
    HIVIEW_LOGI("finish rename file %{public}s", dstZipName.c_str());
}

void CopyFile(const std::string &src, const std::string &dst)
{
    std::string dstFileName = FileUtil::ExtractFileName(dst);
    if (FileUtil::FileExists(dst)) {
        HIVIEW_LOGI("copy already, file : %{public}s.", dstFileName.c_str());
        return;
    }
    HIVIEW_LOGI("copy start, file : %{public}s.", dstFileName.c_str());
    int ret = FileUtil::CopyFileFast(src, dst);
    if (ret != 0) {
        HIVIEW_LOGE("copy failed, file : %{public}s, errno : %{public}d", src.c_str(), errno);
    } else {
        HIVIEW_LOGI("copy end, file : %{public}s.", dstFileName.c_str());
    }
}

void DoClean(const std::string &tracePath, const std::string &prefix)
{
    // Load all files under the path
    std::vector<std::string> files;
    FileUtil::GetDirFiles(tracePath, files);

    // Filter files that belong to me
    std::deque<std::string> filteredFiles;
    for (const auto &file : files) {
        if (prefix.empty() || file.find(prefix) != std::string::npos) {
            filteredFiles.emplace_back(file);
        }
    }
    std::sort(filteredFiles.begin(), filteredFiles.end(), [](const auto& a, const auto& b) {
        return a < b;
    });
    auto threshold = GetTraceThreshold(tracePath, prefix);
    HIVIEW_LOGI("myFiles size : %{public}zu, MyThreshold : %{public}u.", filteredFiles.size(), threshold);

    // Clean up old files, new copied file is still working in sub thread now, only can clean old files here
    while (filteredFiles.size() > threshold) {
        FileUtil::RemoveFile(filteredFiles.front());
        HIVIEW_LOGI("remove file : %{public}s is deleted.", filteredFiles.front().c_str());
        filteredFiles.pop_front();
    }
}

/*
 * apply to xperf, xpower and reliability
 * trace path eg.:
 *     /data/log/hiview/unified_collection/trace/share/
 *     trace_20230906111617@8290-81765922_{device}_{version}.zip
*/
std::vector<std::string> GetUnifiedZipFiles(TraceRetInfo &traceRetInfo, const std::string &destDir,
    const std::string &caller)
{
    if (!FileUtil::FileExists(UNIFIED_SHARE_TEMP_PATH)) {
        if (!CreateMultiDirectory(UNIFIED_SHARE_TEMP_PATH)) {
            HIVIEW_LOGE("failed to create multidirectory.");
            return {};
        }
    }

    std::vector<std::string> files;
    for (const auto &tracePath : traceRetInfo.outputFiles) {
        const std::string tempDestZipPath = GetTraceZipTmpPath(tracePath);
        const std::string destZipPathWithVersion = GetTraceZipFinalPath(tracePath, destDir);
        // for zip if the file has not been compressed
        if (!FileUtil::FileExists(destZipPathWithVersion) && !FileUtil::FileExists(tempDestZipPath)) {
            // new empty file is used to restore tasks in queue
            FileUtil::SaveStringToFile(tempDestZipPath, " ", true);
            UcollectionTask traceTask = [=]() {
                ZipTraceFile(tracePath, destDir);
                UCollectUtil::TraceDecorator::WriteTrafficAfterZip(caller, destZipPathWithVersion);
                DoClean(destDir, "");
            };
            TraceWorker::GetInstance().HandleUcollectionTask(traceTask);
        }
        files.push_back(destZipPathWithVersion);
        HIVIEW_LOGI("trace file : %{public}s.", FileUtil::ExtractFileName(destZipPathWithVersion).c_str());
    }
    return files;
}

/*
 * apply to BetaClub and Other Scenes
 * trace path eg.:
 * /data/log/hiview/unified_collection/trace/special/BetaClub_trace_20230906111633@8306-299900816.sys
*/
std::vector<std::string> GetUnifiedSpecialFiles(TraceRetInfo &traceRetInfo, const std::string& prefix)
{
    if (!FileUtil::FileExists(UNIFIED_SPECIAL_PATH)) {
        if (!CreateMultiDirectory(UNIFIED_SPECIAL_PATH)) {
            HIVIEW_LOGE("create dir %{public}s fail", UNIFIED_SPECIAL_PATH.c_str());
            return {};
        }
    }

    std::vector<std::string> files;
    for (const auto &trace : traceRetInfo.outputFiles) {
        std::string dst = GetTraceSpecialPath(trace, prefix);
        files.push_back(dst);
        // copy trace immediately for betaclub and screen recording
        if (prefix == CallerName::SCREEN || prefix == ClientName::BETACLUB) {
            CopyFile(trace, dst);
            DoClean(UNIFIED_SPECIAL_PATH, prefix);
            continue;
        }
        if (!FileUtil::FileExists(dst)) {
            // copy trace in ffrt asynchronously
            UcollectionTask traceTask = [=]() {
                CopyFile(trace, dst);
                DoClean(UNIFIED_SPECIAL_PATH, prefix);
            };
            TraceWorker::GetInstance().HandleUcollectionTask(traceTask);
        }
    }
    return files;
}

DumpTraceCallback CreateDumpTraceCallback(const std::string &caller)
{
    if (caller == CallerName::RELIABILITY) {
        return [caller] (TraceRetInfo traceRetInfo) {
            if (traceRetInfo.errorCode == TraceErrorCode::SUCCESS) {
                GetUnifiedSpecialFiles(traceRetInfo, caller);
                GetUnifiedZipFiles(traceRetInfo, UNIFIED_SHARE_PATH, caller);
            } else if (traceRetInfo.errorCode == TraceErrorCode::SIZE_EXCEED_LIMIT) {
                GetUnifiedSpecialFiles(traceRetInfo, caller);
            }
        };
    } else {
        return [caller] (TraceRetInfo traceRetInfo) {
            HIVIEW_LOGW("caller %{public}s callback not implement", caller.c_str());
        };
    }
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

void LoadMemoryInfo(DumpEvent &dumpEvent)
{
    std::shared_ptr<UCollectUtil::MemoryCollector> collector = UCollectUtil::MemoryCollector::Create();
    CollectResult<SysMemory> data = collector->CollectSysMemory();
    dumpEvent.sysMemTotal = data.data.memTotal / MB_TO_KB;
    dumpEvent.sysMemFree = data.data.memFree / MB_TO_KB;
    dumpEvent.sysMemAvail = data.data.memAvailable / MB_TO_KB;
}

void WriteDumpTraceHisysevent(DumpEvent &dumpEvent)
{
    LoadMemoryInfo(dumpEvent);
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
        "SYS_CPU", dumpEvent.sysCpu,
        "TRACE_MODE", dumpEvent.traceMode);
    if (ret != 0) {
        HIVIEW_LOGE("HiSysEventWrite failed, ret is %{public}d", ret);
    }
}

UcError GetUcError(TraceRet ret)
{
    if (ret.stateError_ != TraceStateCode::SUCCESS && ret.stateError_ != TraceStateCode::UPDATE_TIME) {
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

std::vector<std::string> ParseAndFilterTraceArgs(const std::unordered_set<std::string> &filterList,
    cJSON* root, const std::string &key)
{
    if (!cJSON_IsObject(root)) {
        HIVIEW_LOGE("trace jsonArgs parse error");
        return {};
    }
    std::vector<std::string> traceArgs;
    CJsonUtil::GetStringArray(root, key, traceArgs);
    auto new_end = std::remove_if(traceArgs.begin(), traceArgs.end(), [&filterList](const std::string& tag) {
        return filterList.find(tag) == filterList.end();
    });
    traceArgs.erase(new_end, traceArgs.end());
    return traceArgs;
}
} // HiViewDFX
} // OHOS
