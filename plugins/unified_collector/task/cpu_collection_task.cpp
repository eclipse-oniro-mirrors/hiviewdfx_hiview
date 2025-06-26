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
#ifdef CATCH_TRACE_FOR_CPU_HIGH_LOAD
#include "dump_trace_controller.h"
#endif
#include "hiview_logger.h"
#include "parameter_ex.h"
#include "time_util.h"
#include "trace_collector.h"

using namespace OHOS::HiviewDFX::UCollectUtil;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("Hiview-CpuCollectionTask");

CpuCollectionTask::CpuCollectionTask(const std::string& workPath) : workPath_(workPath)
{
    InitCpuCollector();
    if (Parameter::IsBetaVersion()) {
        InitCpuStorage();
    }
#ifdef HAS_HIPERF
    InitCpuPerfDump();
#endif

#ifdef CATCH_TRACE_FOR_CPU_HIGH_LOAD
    InitDumpTraceController();
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

#ifdef CATCH_TRACE_FOR_CPU_HIGH_LOAD
void CpuCollectionTask::InitDumpTraceController()
{
    CpuThresholdItem hiviewCpuThresholdItem = {
        .caller = UCollect::TraceCaller::HIVIEW,
        .dumpTraceInterval = 120, // 120 : two minutes
        .cpuLoadThreshold = 0.1, // 0.1 : 10% cpu load
        .processName = "hiview"
    };
    std::vector<CpuThresholdItem> items = {hiviewCpuThresholdItem};
    dumpTraceController_ = std::make_shared<DumpTraceController>(items);
}
#endif

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

#ifdef CATCH_TRACE_FOR_CPU_HIGH_LOAD
    if (Parameter::IsBetaVersion()) {
        dumpTraceController_->CheckAndDumpTrace();
    }
#endif
}
}  // namespace HiviewDFX
}  // namespace OHOS
