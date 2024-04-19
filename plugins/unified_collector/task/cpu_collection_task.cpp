/*
 * Copyright (C) 2023-2024 Huawei Device Co., Ltd.
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

#include "parameter_ex.h"

namespace OHOS {
namespace HiviewDFX {
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
    auto threadCpuCollectResult = threadCpuCollector_ ->CollectThreadStatInfos(true);
    if (Parameter::IsBetaVersion() && threadCpuCollectResult.retCode == UCollect::UcError::SUCCESS) {
        cpuStorage_->StoreThreadDatas(threadCpuCollectResult.data);
    }
    // collect the system cpu usage periodically for hidumper
    cpuCollector_->CollectSysCpuUsage(true);
}
}  // namespace HiviewDFX
}  // namespace OHOS
