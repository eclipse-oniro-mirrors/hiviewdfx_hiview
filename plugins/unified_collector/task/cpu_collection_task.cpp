/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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
#include "cpu_collection_task.h"

#include <unistd.h>

#include "common_utils.h"
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "trace_collector.h"

using namespace OHOS::HiviewDFX::UCollectUtil;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("Hiview-CpuCollectionTask");
struct CpuThresholdItem {
    std::string processName;
    UCollect::TraceCaller caller;
    double cpuLoadThreshold = 0.0;
    bool hasOverThreshold = false;
};

CpuCollectionTask::CpuCollectionTask(const std::string& workPath) : workPath_(workPath)
{
    InitCpuCollector();
    if (Parameter::IsBetaVersion()) {
        InitCpuStorage();
    }
#ifdef HAS_HIPERF
    InitCpuPerfDump();
#endif
}

void CpuCollectionTask::Collect()
{
    if (Parameter::IsBetaVersion()) {
        ReportCpuCollectionEvent();
    }
    CollectCpuData();
}

void CpuCollectionTask::InitCpuCollector()
{
    cpuCollector_ = UCollectUtil::CpuCollector::Create();
    threadCpuCollector_ = cpuCollector_->CreateThreadCollector(getprocpid());
}

void CpuCollectionTask::InitCpuStorage()
{
    cpuStorage_ = std::make_shared<CpuStorage>(workPath_);
}

#ifdef HAS_HIPERF
void CpuCollectionTask::InitCpuPerfDump()
{
    cpuPerfDump_ = std::make_shared<CpuPerfDump>();
}
#endif

void CpuCollectionTask::ReportCpuCollectionEvent()
{
    cpuStorage_->Report();
}

void CpuCollectionTask::CheckAndDumpTraceData()
{
    static std::vector<CpuThresholdItem> checkItems = {
        {"hiview", UCollect::TraceCaller::HIVIEW, 0.07, false}, // 0.07 : 7% cpu load
    };

    for (auto &item : checkItems) {
        int32_t pid = CommonUtils::GetPidByName(item.processName);
        if (pid <= 0) {
            HIVIEW_LOGW("get pid failed, process:%{public}s", item.processName.c_str());
            continue;
        }
        auto processCpuStatInfo = cpuCollector_->CollectProcessCpuStatInfo(pid);
        double cpuLoad = processCpuStatInfo.data.cpuLoad;
        if (!item.hasOverThreshold && cpuLoad >= item.cpuLoadThreshold) {
            HIVIEW_LOGI("over threshold, current cpu load:%{public}f", cpuLoad);
            item.hasOverThreshold = true;
            return;
        }

        // when cpu load restore to normal, start capture history trace
        if (item.hasOverThreshold && cpuLoad < item.cpuLoadThreshold) {
            HIVIEW_LOGI("capture history trace");
            auto traceCollector = TraceCollector::Create();
            traceCollector->DumpTrace(item.caller);
            item.hasOverThreshold = false;
        }
    }
}

void CpuCollectionTask::CollectCpuData()
{
    auto cpuCollectionsResult = cpuCollector_->CollectProcessCpuStatInfos(true);
    if (cpuCollectionsResult.retCode == UCollect::UcError::SUCCESS) {
        if (Parameter::IsBetaVersion()) {
            cpuStorage_->StoreProcessDatas(cpuCollectionsResult.data);
        }

#ifdef HAS_HIPERF
        cpuPerfDump_->CheckAndDumpPerfData(cpuCollectionsResult.data);
#endif
    }
    if (threadCpuCollector_ != nullptr) {
        auto threadCpuCollectResult = threadCpuCollector_ ->CollectThreadStatInfos(true);
        if (Parameter::IsBetaVersion() && threadCpuCollectResult.retCode == UCollect::UcError::SUCCESS) {
            cpuStorage_->StoreThreadDatas(threadCpuCollectResult.data);
        }
    }
    // collect the system cpu usage periodically for hidumper
    cpuCollector_->CollectSysCpuUsage(true);

    if (Parameter::IsBetaVersion()) {
        CheckAndDumpTraceData();
    }
}
}  // namespace HiviewDFX
}  // namespace OHOS
