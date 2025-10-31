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
#ifndef FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_HANDLER_H
#define FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_HANDLER_H
#include <string>
#include <vector>

#include "trace_utils.h"
#include "file_util.h"
#include "string_util.h"
#include "ffrt.h"
#include "singleton.h"

namespace OHOS::HiviewDFX {
using HandleCallback = std::function<void(int64_t)>;
using UcollectionTask = std::function<void ()>;

class TraceWorker : public DelayedRefSingleton<TraceWorker> {
public:
    void HandleUcollectionTask(UcollectionTask ucollectionTask);

private:
    std::unique_ptr<ffrt::queue> ffrtQueue_ = std::make_unique<ffrt::queue>("dft_trace_worker");
};

class TraceHandler {
public:
    TraceHandler(const std::string& tracePath, const std::string& caller, uint32_t cleanThreshold)
        : tracePath_(tracePath), caller_(caller), cleanThreshold_(cleanThreshold) {}
    virtual ~TraceHandler() = default;
    virtual auto HandleTrace(const std::vector<std::string>& outputFiles, HandleCallback callback = {},
        std::shared_ptr<AppCallerEvent> appCallerEvent = nullptr) -> std::vector<std::string> = 0;
    virtual std::string GetTraceFinalPath(const std::string& tracePath, const std::string& prefix) = 0;

protected:
    virtual void DoClean();

protected:
    std::string tracePath_;
    std::string caller_;
    uint32_t cleanThreshold_;
};

class TraceZipHandler : public TraceHandler, public std::enable_shared_from_this<TraceZipHandler> {
public:
    TraceZipHandler(const std::string& tracePath, const std::string& caller, uint32_t cleanThreshold)
        : TraceHandler(tracePath, caller, cleanThreshold) {}
    auto HandleTrace(const std::vector<std::string>& outputFiles, HandleCallback callback = {},
        std::shared_ptr<AppCallerEvent> appCallerEvent = nullptr) -> std::vector<std::string> override;
    std::string GetTraceFinalPath(const std::string& fileName, const std::string& prefix) override
    {
        auto tempZipName = tracePath_ + StringUtil::ReplaceStr(FileUtil::ExtractFileName(fileName), ".sys", ".zip");
        return AddVersionInfoToZipName(tempZipName);
    }

private:
    std::string GetTraceZipTmpPath(const std::string& fileName);
    void AddZipFile(const std::string& srcPath, const std::string& traceZipFile);
    void ZipTraceFile(const std::string& srcPath, const std::string& traceZipFile, const std::string& tmpZipFile);
};

class TraceLinkHandler : public TraceHandler, public std::enable_shared_from_this<TraceLinkHandler> {
public:
    TraceLinkHandler(const std::string& tracePath, const std::string& caller, uint32_t cleanThreshold)
        : TraceHandler(tracePath, caller, cleanThreshold) {}
    auto HandleTrace(const std::vector<std::string>& outputFiles, HandleCallback callback,
        std::shared_ptr<AppCallerEvent> appCallerEvent) -> std::vector<std::string> override;

protected:
    void LinkTraceFile(const std::string& src, const std::string& dst);

    std::string GetTraceFinalPath(const std::string& tracePath, const std::string& prefix) override
    {
        return tracePath_ + prefix + "_" + FileUtil::ExtractFileName(tracePath);
    }

private:
    void DoLinkClean(const std::string& prefix);
};

class TraceAppHandler : public TraceHandler {
public:
    explicit TraceAppHandler(const std::string& tracePath, uint32_t cleanThreshold)
        : TraceHandler(tracePath, ClientName::APP, cleanThreshold) {}
    auto HandleTrace(const std::vector<std::string>& outputFiles, HandleCallback callback,
        std::shared_ptr<AppCallerEvent> appCallerEvent) -> std::vector<std::string> override;

    std::string GetTraceFinalPath(const std::string& tracePath, const std::string& prefix) override
    {
        return "";
    }

private:
    std::string MakeTraceFileName(std::shared_ptr<AppCallerEvent> appCallerEvent);
};
}
#endif
