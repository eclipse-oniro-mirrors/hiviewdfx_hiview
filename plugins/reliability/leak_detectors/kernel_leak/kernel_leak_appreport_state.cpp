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

#include "kernel_leak_appreport_state.h"

#include "kernel_leak_info.h"
#include "kernel_leak_detector.h"
#include "fault_common.h"
#include "hiview_logger.h"
#include "parameters.h"
#include "dfr_log_filter.h"
#include "dfr_util.h"

using namespace std;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "KernelLeakAppReportState");

ErrCode KernelLeakAppReportState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &obj)
{
    if (faultInfo == nullptr) {
        HIVIEW_LOGE("StateProcess faultInfo is null.");
        return FAILURE;
    }
    HIVIEW_LOGI("current state: %{public}s", STATE_TO_STR[faultInfo->GetState()]);
    unordered_set<string> whiteList = { "ion", "gpu", "ashmem" };
    const string &leakType = faultInfo->GetProcessName();
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    const string &processName = kernelInfo->GetProcessNameForReport();
    if (whiteList.count(leakType) == 0) {
        HIVIEW_LOGI("leakType:%{public}s, processName:%{public}s, do not send to app",
            leakType.c_str(), processName.c_str());
        return ChangeNextState(faultInfo, obj);
    }

    pid_t pid = kernelInfo->GetProcessPidForReport();
    if (!faultInfo->IsApp()) {
        HIVIEW_LOGW("not app, do not send to hiapp");
        return ChangeNextState(faultInfo, obj);
    }

    FilterAppeventFile(faultInfo);

    ffrt::this_task::sleep_for(chrono::microseconds(WAIT_HISYSEVENT_LOGPACK_FINISHED * US_PER_SECOND));
    PostEvent(faultInfo);

    ffrt::this_task::sleep_for(chrono::microseconds(WAIT_HIAPPEVENT_LOGPACK_FINISHED * US_PER_SECOND));
    DfrUtil::GetInstance().RemoveFile(kernelInfo->GetLogFileToApp());
    DfrUtil::GetInstance().RemoveFile(kernelInfo->GetCopyProfilerPath());
    return ChangeNextState(faultInfo, obj);
}

void KernelLeakAppReportState::PostEvent(shared_ptr<FaultInfoBase> &faultInfo)
{
    const string &leakType = faultInfo->GetProcessName();
    FaultStateType curState = faultInfo->GetState();
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    OverBaseProcesses processInfo = kernelInfo->GetProcessForReport();
    pid_t pid = kernelInfo->GetProcessPidForReport();
    const string &processName = kernelInfo->GetProcessNameForReport();
    map<string, string>reportTypeMap = {
        {"gpu", "gpu_memory"},
        {"ion", "ion_memory"},
        {"ashmem", "ashmem_memory"},
    };
    HIVIEW_LOGD("PostAppEvent %{public}s process: %{public}s pid: %{public}d",
        reportTypeMap[leakType].c_str(), processName.c_str(), pid);

    AppEventHandler::ResourceOverLimitInfo info;
    info.pid = processInfo.pid;
    info.uid = processInfo.uid;
    info.appRunningUniqueId = kernelInfo->GetAppRunningId();
    info.bundleName = processInfo.processName;
    info.bundleVersion = kernelInfo->GetVersionForReport();
    info.resourceType = reportTypeMap[leakType];
    info.ion = leakType == "ion" ? processInfo.virtualSize : 0;
    info.gpu = leakType == "gpu" ? processInfo.virtualSize : 0;
    info.rss = leakType == "ashmem" ? processInfo.virtualSize : 0;
    info.level = (curState == PROC_APP_REPORT_KILL_STATE) ? "kill" : "warning";
    kernelInfo->GetLogPath(info.logPath);
    DfrUtil::GetInstance().GetStatm(info.pid, info.vss, info.rss);
    DfrUtil::GetInstance().GetMeminfo(info.avaliableMem, info.freeMem, info.totalMem);
    DfrUtil::GetInstance().PostAppEvent(info);
}

void KernelLeakAppReportState::FilterAppeventFile(shared_ptr<FaultInfoBase> &faultInfo)
{
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    if (kernelInfo == nullptr) {
        HIVIEW_LOGE("FilterAppeventFile kernelInfo is null.");
        return;
    }
    pair<string, pid_t> processInfo = { kernelInfo->GetProcessNameForReport(), kernelInfo->GetProcessPidForReport() };
    string srcPath = kernelInfo->GetLogPath();
    string destPath = kernelInfo->GetLogFileToApp();
    DfrLogFilter::FilterMemoryLeakFile(processInfo, srcPath, destPath);
}

ErrCode KernelLeakAppReportState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    FaultStateType nextState = kernelInfo->GetNextState();
    return detectorObj.ExeNextStateProcess(faultInfo, nextState);
}
} // namespace HiviewDFX
} // namespace OHOS
