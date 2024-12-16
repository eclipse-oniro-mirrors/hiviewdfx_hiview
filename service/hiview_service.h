/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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
#ifndef HIVIEW_PLATFORM_MANAGEMENT_INFO_EXPORT_SERVICE_H
#define HIVIEW_PLATFORM_MANAGEMENT_INFO_EXPORT_SERVICE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "audit_log_parser.h"
#include "utility/trace_collector.h"
#include "utility/cpu_collector.h"
#include "client/trace_collector.h"
#include "client/memory_collector.h"
#include "utility/graphic_memory_collector.h"

namespace OHOS {
namespace HiviewDFX {
class HiviewService {
public:
    HiviewService();
    ~HiviewService(){};

    void StartService();
    // Dump interface
    void DumpRequestDispatcher(int fd, const std::vector<std::string>& cmds);

    int32_t Copy(const std::string& srcFilePath, const std::string& destFilePath);
    int32_t Move(const std::string& srcFilePath, const std::string& destFilePath);
    int32_t Remove(const std::string& filePath);

    // Trace interfaces
    CollectResult<int32_t> OpenSnapshotTrace(const std::vector<std::string>& tagGroups);
    CollectResult<std::vector<std::string>> DumpSnapshotTrace(UCollect::TraceCaller caller);
    CollectResult<int32_t> OpenRecordingTrace(const std::string& tags);
    CollectResult<int32_t> RecordingTraceOn();
    CollectResult<std::vector<std::string>> RecordingTraceOff();
    CollectResult<int32_t> CloseTrace();
    CollectResult<int32_t> RecoverTrace();
    CollectResult<int32_t> CaptureDurationTrace(UCollectClient::AppCaller &appCaller);
    CollectResult<double> GetSysCpuUsage();
    CollectResult<int32_t> SetAppResourceLimit(UCollectClient::MemoryCaller& memoryCaller);
    CollectResult<int32_t> GetGraphicUsage(int32_t pid);

private:
    void DumpDetailedInfo(int fd);
    void DumpPluginInfo(int fd, const std::vector<std::string>& cmds) const;
    void DumpLoadedPluginInfo(int fd) const;
    void DumpPluginUsageInfo(int fd);
    void DumpPluginUsageInfo(int fd, const std::string& pluginName) const;

    void DumpThreadUsageInfo(int fd) const;
    void DumpThreadUsageInfo(int fd, const std::string& threadName) const;

    void DumpPipelineUsageInfo(int fd) const;
    void DumpPipelineUsageInfo(int fd, const std::string& pipelineName) const;

    int CopyFile(const std::string& srcFilePath, const std::string& destFilePath);

    void PrintUsage(int fd) const;

private:
    std::unique_ptr<AuditLogParser> parser_ = nullptr;
    std::shared_ptr<UCollectUtil::TraceCollector> traceCollector_;
    std::shared_ptr<UCollectUtil::CpuCollector> cpuCollector_;
    std::shared_ptr<UCollectUtil::GraphicMemoryCollector> graphicMemoryCollector_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLATFORM_MANAGEMENT_INFO_EXPORT_SERVICE_H
