/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "native_leak_appreport_state.h"

#include <fstream>
#include "native_leak_info.h"
#include "native_leak_detector.h"
#include "fault_common.h"
#include "hiview_logger.h"
#include "parameters.h"
#include "dfr_log_filter.h"

using namespace std;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "NativeLeakAppReportState");

ErrCode NativeLeakAppReportState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &obj)
{
    const string &name = faultInfo->GetProcessName();
    const pid_t pid = faultInfo->GetPid();
    HIVIEW_LOGI("StateProcess process: %{public}s pid: %{public}d", name.c_str(), pid);

    if (!faultInfo->IsApp()) {
        return ChangeNextState(faultInfo, obj);
    }

    FilterAppeventFile(faultInfo);
    ffrt::this_task::sleep_for(chrono::microseconds(WAIT_HISYSEVENT_LOGPACK_FINISHED * US_PER_SECOND));
    PostEvent(faultInfo);
    return ChangeNextState(faultInfo, obj);
}

ErrCode NativeLeakAppReportState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &obj)
{
    auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
    FaultStateType nextState = nlInfo->GetNextState();
    return obj.ExeNextStateProcess(faultInfo, nextState);
}

void NativeLeakAppReportState::PostEvent(shared_ptr<FaultInfoBase> &faultInfo)
{
    const string &name = faultInfo->GetProcessName();
    const pid_t pid = faultInfo->GetPid();
    FaultStateType curState = faultInfo->GetState();
    HIVIEW_LOGD("PostAppEvent pss_memory process: %{public}s pid: %{public}d", name.c_str(), pid);

    auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
    const string copyProfilerPath = nlInfo->GetCopyProfilerPath();
    AppEventHandler::ResourceOverLimitInfo info;
    info.pid = nlInfo->GetPid();
    info.uid = nlInfo->GetUid();
    info.appRunningUniqueId = nlInfo->GetAppRunningId();
    info.bundleName = nlInfo->GetProcessName();
    info.bundleVersion = nlInfo->GetHapVersion();
    info.resourceType = "pss_memory";
    info.pss = static_cast<uint64_t>(nlInfo->GetLastSampleRecord().totalPss);
    info.ion = static_cast<uint64_t>(nlInfo->GetLastSampleRecord().ion);
    info.gpu = static_cast<uint64_t>(nlInfo->GetLastSampleRecord().gpu);
    info.level = (curState == PROC_APP_REPORT_KILL_STATE) ? "kill" : "warning";
    info.extPssInfo = nlInfo->GetExtPssInfo();
    nlInfo->GetLogPath(info.logPath);
    DfrUtil::GetInstance().GetStatm(info.pid, info.vss, info.rss);
    DfrUtil::GetInstance().GetMeminfo(info.avaliableMem, info.freeMem, info.totalMem);
    DfrUtil::GetInstance().PostAppEvent(info);
    ffrt::this_task::sleep_for(chrono::microseconds(WAIT_HIAPPEVENT_LOGPACK_FINISHED * US_PER_SECOND));
    nlInfo->ClearHiappLog();
}

void NativeLeakAppReportState::FilterAppeventFile(shared_ptr<FaultInfoBase> &faultInfo)
{
    auto nlInfo = static_pointer_cast<NativeLeakInfo>(faultInfo);
    pair<string, pid_t> processInfo = { nlInfo->GetProcessName(), nlInfo->GetPid() };
    uint32_t allDumpType = nlInfo->GetAllDumpLogType();
    if (allDumpType == NO_LOG) {
        return;
    }
    if ((allDumpType & ION_LOG) || (allDumpType & GPU_LOG)) {
        string srcPath = nlInfo->GetKernelPath();
        string destPath = nlInfo->GetKernelFileToApp();
        DfrLogFilter::FilterMemoryLeakFile(processInfo, srcPath, destPath);
    }
    if (allDumpType & PSS_LOG) {
        string srcPath = nlInfo->GetSmapsPath();
        string destPath = nlInfo->GetSmapsPathToApp();
        DfrLogFilter::FilterMemoryLeakFile(processInfo, srcPath, destPath);
    }
}

} // namespace HiviewDFX
} // namespace OHOS
