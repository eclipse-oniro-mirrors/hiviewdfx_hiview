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
#include "hitrace_define.h"

namespace OHOS::HiviewDFX {

class TraceHandler {
public:
    TraceHandler(const std::string &tracePath, const std::string& caller)
        : tracePath_(tracePath), caller_(caller) {}
    virtual ~TraceHandler() = default;
    virtual auto HandleTrace(const std::vector<std::string>& outputFiles) -> std::vector<std::string> = 0;
    virtual std::string GetTraceFinalPath(const std::string& tracePath, const std::string& prefix) = 0;

protected:
    virtual uint32_t GetTraceCleanThreshold(const std::string&) = 0;
    virtual void DoClean(const std::string& business);
    virtual void FilterExistFile(std::vector<std::string>& outputFiles);

protected:
    std::string tracePath_;
    std::string caller_;
};

class TraceZipHandler : public TraceHandler, public std::enable_shared_from_this<TraceZipHandler> {
public:
    TraceZipHandler(const std::string& tracePath, const std::string& caller) : TraceHandler(tracePath, caller) {}
    auto HandleTrace(const std::vector<std::string>& outputFiles) -> std::vector<std::string> override;
    std::string GetTraceFinalPath(const std::string& fileName, const std::string& prefix) override
    {
        auto tempZipName = tracePath_ + StringUtil::ReplaceStr(FileUtil::ExtractFileName(fileName), ".sys", ".zip");
        return AddVersionInfoToZipName(tempZipName);
    }

private:
    uint32_t GetTraceCleanThreshold(const std::string&) override;
    std::string GetTraceZipTmpPath(const std::string& fileName);
    void AddZipFile(const std::string& srcPath, const std::string& traceZipFile);
    void ZipTraceFile(const std::string& srcPath, const std::string& traceZipFile, const std::string& tmpZipFile);
};

class TraceCopyHandler : public TraceHandler, public std::enable_shared_from_this<TraceCopyHandler> {
public:
    TraceCopyHandler(const std::string& tracePath, const std::string& caller)
        : TraceHandler(tracePath, caller) {}
    auto HandleTrace(const std::vector<std::string>& outputFiles) -> std::vector<std::string> override;

protected:
    void CopyTraceFile(const std::string& src, const std::string& dst);
    uint32_t GetTraceCleanThreshold(const std::string& prefix) override;

    std::string GetTraceFinalPath(const std::string& tracePath, const std::string& prefix) override
    {
        return tracePath_ + prefix + "_" + FileUtil::ExtractFileName(tracePath);
    }
};

class TraceSyncCopyHandler : public TraceCopyHandler {
public:
    TraceSyncCopyHandler(const std::string& tracePath, const std::string& caller)
        : TraceCopyHandler(tracePath, caller) {}
    auto HandleTrace(const std::vector<std::string>& outputFiles) -> std::vector<std::string> override;
};

class TraceAppHandler : public TraceHandler {
public:
    explicit TraceAppHandler(const std::string& tracePath) : TraceHandler(tracePath, ClientName::APP) {}
    auto HandleTrace(const std::vector<std::string>& outputFiles) -> std::vector<std::string> override;
    std::string GetTraceFinalPath(const std::string& tracePath, const std::string& prefix) override;

protected:
    uint32_t GetTraceCleanThreshold(const std::string& prefix) override;
};
}
#endif
