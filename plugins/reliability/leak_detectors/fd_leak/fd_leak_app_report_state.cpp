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

#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include "fd_leak_app_report_state.h"
#include "fd_leak_info.h"
#include "fd_leak_dump_client.h"
#include "fd_leak_detector.h"
#include "fault_common.h"
#include "hiview_logger.h"
#include "hisysevent.h"
#include "res_sched_client.h"
#include "res_sched_kill_reason.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "FdLeakAppReportState");

using namespace std;

ErrCode FdLeakAppReportState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &obj)
{
    HIVIEW_LOGI("FdLeakAppReportState::StateProcess");

    auto fdInfo = static_pointer_cast<FdLeakInfo>(faultInfo);
    if (fdInfo == nullptr) {
        HIVIEW_LOGE("StateProcess fdInfo is null.");
        return FAILURE;
    }
    if (!fdInfo->IsApp()) {
        return ChangeNextState(faultInfo, obj);
    }

    ffrt::this_task::sleep_for(chrono::microseconds(WAIT_HISYSEVENT_LOGPACK_FINISHED * US_PER_SECOND));
    PostEvent(faultInfo);
    return ChangeNextState(faultInfo, obj);
}

ErrCode FdLeakAppReportState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &obj)
{
    auto fdInfo = static_pointer_cast<FdLeakInfo>(faultInfo);
    FaultStateType nextState = fdInfo->GetNextState();
    return obj.ExeNextStateProcess(faultInfo, nextState);
}

void FdLeakAppReportState::PostEvent(shared_ptr<FaultInfoBase> &faultInfo)
{
    HIVIEW_LOGI("FdLeakAppReportState::PostEvent");

    FaultStateType curState = faultInfo->GetState();
    auto fdInfo = static_pointer_cast<FdLeakInfo>(faultInfo);
    const string copyProfilerPath = fdInfo->GetCopyProfilerPath();

    AppEventHandler::ResourceOverLimitInfo info;
    info.pid = fdInfo->GetPid();
    info.uid = fdInfo->GetUid();
    info.appRunningUniqueId = fdInfo->GetAppRunningId();
    info.bundleName = fdInfo->GetProcessName();
    info.bundleVersion = fdInfo->GetHapVersion();
    info.resourceType = "fd";
    info.fdNum = fdInfo->GetFdNums();
    info.topFdType = fdInfo->GetTopFdTypeList().empty() ? "open" : fdInfo->GetTopFdTypeList()[0];
    info.topFdNum = fdInfo->GetTopFdNum();
    info.level = (curState == PROC_APP_REPORT_KILL_STATE) ? "kill" : "warning";
    info.logPath.push_back("\"" + fdInfo->GetFdLogPath() + "\"");
    info.logPath.push_back("\"" + copyProfilerPath + "\"");

    HIVIEW_LOGD("fd postAppEvent start");
    DfrUtil::GetInstance().PostAppEvent(info);
    HIVIEW_LOGI("fd postAppEvent finished");
    ffrt::this_task::sleep_for(chrono::microseconds(WAIT_HIAPPEVENT_LOGPACK_FINISHED * US_PER_SECOND));
    DfrUtil::GetInstance().RemoveFile(copyProfilerPath);
}

} // namespace HiviewDFX
} // namespace OHOS
