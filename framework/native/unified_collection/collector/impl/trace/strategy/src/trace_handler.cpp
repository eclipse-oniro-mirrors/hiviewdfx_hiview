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
#include "trace_handler.h"

#include <deque>

#include "hiview_logger.h"
#include "file_util.h"
#include "trace_worker.h"
#include "trace_decorator.h"
#include "hisysevent.h"
#include "hiview_event_report.h"
#include "hiview_zip_util.h"

using namespace OHOS::HiviewDFX::Hitrace;
namespace OHOS::HiviewDFX {
namespace {
DEFINE_LOG_TAG("UCollectUtil-TraceCollector");
constexpr uint32_t UNIFIED_SHARE_COUNTS = 25;
constexpr uint32_t UNIFIED_TELEMETRY_COUNTS = 20;
constexpr uint32_t UNIFIED_APP_SHARE_COUNTS = 40;

const std::map<std::string, uint32_t> TRACE_CLEAN_THRESHOLD = {
    {CallerName::XPERF,       3},
    {CallerName::RELIABILITY, 3},
    {CallerName::OTHER,       5},
    {CallerName::SCREEN,      1},
    {ClientName::BETACLUB,    2}
};
}

void TraceHandler::DoClean(const std::string &prefix)
{
    // Load all files under the path
    std::vector<std::string> files;
    FileUtil::GetDirFiles(tracePath_, files);

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
    auto threshold = GetTraceCleanThreshold(prefix);
    HIVIEW_LOGI("myFiles size : %{public}zu, MyThreshold : %{public}u.", filteredFiles.size(), threshold);

    while (filteredFiles.size() > threshold) {
        FileUtil::RemoveFile(filteredFiles.front());
        HIVIEW_LOGI("remove file : %{public}s is deleted.", filteredFiles.front().c_str());
        filteredFiles.pop_front();
    }
}

void TraceHandler::FilterExistFile(std::vector<std::string>& outputFiles)
{
    auto newEnd = std::remove_if(outputFiles.begin(), outputFiles.end(), [this](const std::string& filename) {
        const std::string traceZipFile = GetTraceFinalPath(filename, "");
        return FileUtil::FileExists(traceZipFile);
    });
    outputFiles.erase(newEnd, outputFiles.end());
}

auto TraceZipHandler::HandleTrace(const std::vector<std::string>& outputFiles) -> std::vector<std::string>
{
    if (!FileUtil::FileExists(tracePath_) && !FileUtil::CreateMultiDirectory(tracePath_)) {
        HIVIEW_LOGE("failed to create multidirectory.");
        return {};
    }
    auto zipFileInfos = outputFiles;
    FilterExistFile(zipFileInfos);
    std::vector<std::string> files;
    for (const auto &filename : zipFileInfos) {
        const std::string traceZipFile = GetTraceFinalPath(filename, "");
        const std::string tmpZipFile = GetTraceZipTmpPath(filename);
        if (!tmpZipFile.empty()) {
            // a trace producted, just make a marking
            FileUtil::SaveStringToFile(tmpZipFile, " ", true);
        }
        UcollectionTask traceTask = [filename, traceZipFile, tmpZipFile, handler = shared_from_this()] {
            handler->ZipTraceFile(filename, traceZipFile, tmpZipFile);
            UCollectUtil::TraceDecorator::WriteTrafficAfterZip(handler->caller_, traceZipFile);
            handler->DoClean("");
        };
        TraceWorker::GetInstance().HandleUcollectionTask(traceTask);
        files.push_back(traceZipFile);
        HIVIEW_LOGI("insert zip file : %{public}s.", traceZipFile.c_str());
    }
    return files;
}

uint32_t TraceZipHandler::GetTraceCleanThreshold(const std::string &)
{
    if (tracePath_ == UNIFIED_SHARE_PATH) {
        return UNIFIED_SHARE_COUNTS;
    }
    if (tracePath_ == UNIFIED_TELEMETRY_PATH) {
        return UNIFIED_TELEMETRY_COUNTS;
    }
    return 0;
}

std::string TraceZipHandler::GetTraceZipTmpPath(const std::string &fileName)
{
    std::string tempPath = tracePath_ + "temp/";
    if (!FileUtil::FileExists(tempPath)) {
        return "";
    }
    return tempPath + StringUtil::ReplaceStr(FileUtil::ExtractFileName(fileName), ".sys", ".zip");
}

void TraceZipHandler::AddZipFile(const std::string &srcPath, const std::string &traceZipFile)
{
    HiviewZipUnit zipUnit(traceZipFile);
    if (int32_t ret = zipUnit.AddFileInZip(srcPath, ZipFileLevel::KEEP_NONE_PARENT_PATH); ret != 0) {
        HIVIEW_LOGW("zip trace failed, ret: %{public}d.", ret);
    }
}

void TraceZipHandler::ZipTraceFile(const std::string &srcPath, const std::string &traceZipFile,
    const std::string &tmpZipFile)
{
    if (FileUtil::FileExists(traceZipFile)) {
        HIVIEW_LOGI("trace zip file : %{public}s already exist", traceZipFile.c_str());
        return;
    }
    CheckCurrentCpuLoad();
    HiviewEventReport::ReportCpuScene("5");
    if (tmpZipFile.empty()) {
        AddZipFile(srcPath, traceZipFile);
    } else {
        AddZipFile(srcPath, tmpZipFile);
        FileUtil::RenameFile(tmpZipFile, traceZipFile);
    }
    HIVIEW_LOGI("finish rename file %{public}s", traceZipFile.c_str());
}

void TraceCopyHandler::CopyTraceFile(const std::string &src, const std::string &dst)
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

uint32_t TraceCopyHandler::GetTraceCleanThreshold(const std::string &prefix)
{
    if (TRACE_CLEAN_THRESHOLD.find(prefix) == TRACE_CLEAN_THRESHOLD.end()) {
        HIVIEW_LOGW("lack count config : %{public}s", prefix.c_str());
        return 0;
    }
    return TRACE_CLEAN_THRESHOLD.at(prefix);
}

auto TraceCopyHandler::HandleTrace(const std::vector<std::string>& outputFiles) -> std::vector<std::string>
{
    if (!FileUtil::FileExists(tracePath_) && !FileUtil::CreateMultiDirectory(tracePath_)) {
        HIVIEW_LOGE("create dir %{public}s fail", tracePath_.c_str());
        return {};
    }
    std::vector<std::string> files;
    for (const auto &trace : outputFiles) {
        std::string dst = GetTraceFinalPath(trace, caller_);
        files.push_back(dst);
        if (!FileUtil::FileExists(dst)) {
            // copy trace in ffrt asynchronously
            UcollectionTask traceTask = [trace, dst, handler = shared_from_this()]() {
                handler->CopyTraceFile(trace, dst);
                handler->DoClean(handler->caller_);
            };
            TraceWorker::GetInstance().HandleUcollectionTask(traceTask);
        }
    }
    return files;
}

auto TraceSyncCopyHandler::HandleTrace(const std::vector<std::string>& outputFiles) -> std::vector<std::string>
{
    if (!FileUtil::FileExists(tracePath_) && !FileUtil::CreateMultiDirectory(tracePath_)) {
        HIVIEW_LOGE("create dir %{public}s fail", tracePath_.c_str());
        return {};
    }
    std::vector<std::string> files;

    // copy trace immediately for betaclub and screen recording
    for (const auto &trace : outputFiles) {
        std::string dst = GetTraceFinalPath(trace, caller_);
        files.push_back(dst);
        CopyTraceFile(trace, dst);
        DoClean(caller_);
    }
    return files;
}

auto TraceAppHandler::HandleTrace(const std::vector<std::string>& outputFiles) -> std::vector<std::string>
{
    DoClean(caller_);
    return {};
}

std::string TraceAppHandler::GetTraceFinalPath(const std::string &tracePath, const std::string &prefix)
{
    return "";
}

uint32_t TraceAppHandler::GetTraceCleanThreshold(const std::string &business)
{
    return UNIFIED_APP_SHARE_COUNTS;
}
}
