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
#include <filesystem>

#include "hiview_logger.h"
#include "file_util.h"
#include "trace_decorator.h"
#include "hiview_event_report.h"
#include "hiview_zip_util.h"
#include "trace_state_machine.h"

namespace OHOS::HiviewDFX {
namespace {
DEFINE_LOG_TAG("UCollectUtil-TraceCollector");
void WriteTrafficLog(std::chrono::time_point<std::chrono::steady_clock> startTime, const std::string& caller,
    const std::string& srcFile, const std::string& traceFile)
{
    auto endTime = std::chrono::steady_clock::now();
    auto execDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    UCollectUtil::TraceTrafficInfo traceInfo {
        caller,
        traceFile,
        FileUtil::GetFileSize(srcFile),
        0,
        execDuration
    };
    UCollectUtil::TraceDecorator::WriteTrafficAfterHandle(traceInfo);
}

void WriteZipTrafficLog(std::chrono::time_point<std::chrono::steady_clock> startTime, const std::string& caller,
    const std::string& srcFile, const std::string& zipFile)
{
    auto endTime = std::chrono::steady_clock::now();
    auto execDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    UCollectUtil::TraceTrafficInfo traceInfo {
        caller,
        zipFile,
        FileUtil::GetFileSize(srcFile),
        FileUtil::GetFileSize(zipFile),
        execDuration
    };
    UCollectUtil::TraceDecorator::WriteTrafficAfterHandle(traceInfo);
}
}

void TraceWorker::HandleUcollectionTask(UcollectionTask ucollectionTask)
{
    ffrtQueue_->submit(ucollectionTask, ffrt::task_attr().name("dft_uc_trace"));
}

void TraceHandler::DoClean()
{
    // Load all files under the path
    std::vector<std::string> files;
    FileUtil::GetDirFiles(tracePath_, files);

    // Filter files that belong to me
    std::deque<std::string> filteredFiles(files.begin(), files.end());
    std::sort(filteredFiles.begin(), filteredFiles.end(), [](const auto& a, const auto& b) {
        return a < b;
    });
    HIVIEW_LOGI("myFiles size : %{public}zu, MyThreshold : %{public}u.", filteredFiles.size(), cleanThreshold_);

    while (filteredFiles.size() > cleanThreshold_) {
        FileUtil::RemoveFile(filteredFiles.front());
        HIVIEW_LOGI("remove file : %{public}s is deleted.", filteredFiles.front().c_str());
        filteredFiles.pop_front();
    }
}

auto TraceZipHandler::HandleTrace(const std::vector<std::string>& outputFiles, HandleCallback callback,
    std::shared_ptr<AppCallerEvent> appCallerEvent) -> std::vector<std::string>
{
    if (!FileUtil::FileExists(tracePath_) && !FileUtil::CreateMultiDirectory(tracePath_)) {
        HIVIEW_LOGE("failed to create multidirectory.");
        return {};
    }
    std::vector<std::string> files;
    for (const auto &filename : outputFiles) {
        auto startTime = std::chrono::steady_clock::now();
        const std::string traceZipFile = GetTraceFinalPath(filename, "");
        if (FileUtil::FileExists(traceZipFile)) {
            HIVIEW_LOGI("trace:%{public}s already zipped, zip pass", traceZipFile.c_str());
            continue;
        }
        const std::string tmpZipFile = GetTraceZipTmpPath(filename);
        if (!tmpZipFile.empty()) {
            if (FileUtil::FileExists(tmpZipFile)) {
                HIVIEW_LOGI("trace:%{public}s already in zip queue, zip pass", traceZipFile.c_str());
                continue;
            }
            // a trace producted, just make a marking
            FileUtil::SaveStringToFile(tmpZipFile, " ", true);
        }
        UcollectionTask traceTask = [filename, traceZipFile, tmpZipFile, startTime, callback,
            handler = shared_from_this()] {
                handler->ZipTraceFile(filename, traceZipFile, tmpZipFile);
                handler->DoClean();
                if (callback != nullptr) {
                    callback(static_cast<int64_t>(FileUtil::GetFileSize(traceZipFile)));
                }
                WriteZipTrafficLog(startTime, handler->caller_, filename, traceZipFile);
        };
        TraceWorker::GetInstance().HandleUcollectionTask(traceTask);
        files.push_back(traceZipFile);
        HIVIEW_LOGI("insert zip file : %{public}s.", traceZipFile.c_str());
    }
    return files;
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

void TraceLinkHandler::LinkTraceFile(const std::string &src, const std::string &dst)
{
    std::string dstFileName = FileUtil::ExtractFileName(dst);
    if (!FileUtil::FileExists(src)) {
        HIVIEW_LOGE("src file : %{public}s. not exist", src.c_str());
        return;
    }
    if (FileUtil::FileExists(dst)) {
        HIVIEW_LOGI("same link file : %{public}s. return", dst.c_str());
        return;
    }
    std::filesystem::create_symlink(src, dst);
    bool result = TraceStateMachine::GetInstance().AddSymlinkXattr(src);
    HIVIEW_LOGI("Link, src: %{public}s, dst: %{public}s, result:%{public}d", src.c_str(), dst.c_str(), result);
}

void TraceLinkHandler::DoLinkClean(const std::string &prefix)
{
    if (prefix.empty()) {
        return;
    }
    // Load all files under the path
    std::vector<std::string> files;
    FileUtil::GetDirFiles(tracePath_, files);

    std::deque<std::string> filteredLinks;
    for (const auto &link : files) {
        if (link.find(prefix) != std::string::npos) {
            filteredLinks.emplace_back(link);
        }
    }
    std::sort(filteredLinks.begin(), filteredLinks.end(), [](const auto& a, const auto& b) {
        return a < b;
    });
    HIVIEW_LOGI("myFiles size : %{public}zu, MyThreshold : %{public}u.", filteredLinks.size(), cleanThreshold_);
    while (filteredLinks.size() > cleanThreshold_) {
        std::string link = filteredLinks.front();
        filteredLinks.pop_front();
        if (!FileUtil::RemoveFile(link)) {
            HIVIEW_LOGE("file:%{public}s delete failed", link.c_str());
            continue;
        }
        bool result = false;
        std::string src = FileUtil::ReadSymlink(link);
        if (!src.empty()) {
            result = TraceStateMachine::GetInstance().RemoveSymlinkXattr(src);
        }
        HIVIEW_LOGI("file:%{public}s is delete, remove attr:%{public}d", link.c_str(), result);
    }
}

auto TraceLinkHandler::HandleTrace(const std::vector<std::string>& outputFiles, HandleCallback callback,
    std::shared_ptr<AppCallerEvent> appCallerEvent) -> std::vector<std::string>
{
    if (!FileUtil::FileExists(tracePath_) && !FileUtil::CreateMultiDirectory(tracePath_)) {
        HIVIEW_LOGE("create dir %{public}s fail", tracePath_.c_str());
        return {};
    }
    std::vector<std::string> files;
    for (const auto &trace : outputFiles) {
        auto startTime = std::chrono::steady_clock::now();
        std::string dst = GetTraceFinalPath(trace, caller_);
        files.push_back(dst);
        LinkTraceFile(trace, dst);
        DoLinkClean(caller_);
        WriteTrafficLog(startTime, caller_, trace, dst);
    }
    return files;
}

auto TraceAppHandler::HandleTrace(const std::vector<std::string> &outputFiles, HandleCallback callback,
    std::shared_ptr<AppCallerEvent> appCallerEvent) -> std::vector<std::string>
{
    if (appCallerEvent == nullptr || outputFiles.empty()) {
        return {};
    }
    std::string traceFileName = MakeTraceFileName(appCallerEvent);
    HIVIEW_LOGI("src:%{public}s, dir:%{public}s", outputFiles[0].c_str(), traceFileName.c_str());
    FileUtil::RenameFile(outputFiles[0], traceFileName);
    appCallerEvent->externalLog_ = traceFileName;
    DoClean();
    return {traceFileName};
}

std::string TraceAppHandler::MakeTraceFileName(std::shared_ptr<AppCallerEvent> appCallerEvent)
{
    std::string &bundleName = appCallerEvent->bundleName_;
    int32_t pid = appCallerEvent->pid_;
    int64_t beginTime = appCallerEvent->taskBeginTime_;
    int64_t endTime = appCallerEvent->taskEndTime_;
    int32_t costTime = (appCallerEvent->taskEndTime_ - appCallerEvent->taskBeginTime_);

    std::string d1 = TimeUtil::TimestampFormatToDate(beginTime/ TimeUtil::SEC_TO_MILLISEC, "%Y%m%d%H%M%S");
    std::string d2 = TimeUtil::TimestampFormatToDate(endTime/ TimeUtil::SEC_TO_MILLISEC, "%Y%m%d%H%M%S");

    std::string name;
    name.append(tracePath_).append("APP_").append(bundleName).append("_").append(std::to_string(pid));
    name.append("_").append(d1).append("_").append(d2).append("_").append(std::to_string(costTime)).append(".sys");
    return name;
}
}
