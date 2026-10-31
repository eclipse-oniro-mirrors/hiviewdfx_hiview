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
#include "notify_leak_report_state.h"
#include "notify_leak_detector.h"
#include "telemetry_manager.h"
#include "dfr_util.h"
#include "hiview_logger.h"
#include "fault_common.h"
#include "hisysevent.h"
#include "fault_detector_manager.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "NotifyLeakReportState");

using namespace std;

void NotifyLeakReportState::HandleParam(shared_ptr<FaultInfoBase> &faultInfo, string secondFrame)
{
    auto notifyInfo = static_pointer_cast<NotifyLeakInfo>(faultInfo);
    MetaInfoParams params = {
        .eventName = "memleak",
        .processName = faultInfo->GetProcessName(),
        .pid = faultInfo->GetPid(),
        .firstFrame = NOTIFY_TYPE_STRING[notifyInfo->GetNotifyType()],
        .secondFrame = secondFrame,
        .thirdFrame = "0",
        .timeStamp = faultInfo->GetLeakedTime(),
        .basePath = MEMORY_LEAK_PATH,
        .versionName = faultInfo->GetHapVersion(),
        .versionCode = faultInfo->GetHapVersionCode(),
        .foreground = FaultDetectorManager::CheckIfAppForeGround(faultInfo->GetProcessName()) ? "1" : "0",
        .appRunningId = faultInfo->GetAppRunningId()
    };
    DfrUtil::GetInstance().CreateMetaInfoFile(params);
}

ErrCode NotifyLeakReportState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    HIVIEW_LOGI("start");
    auto notifyInfo = static_pointer_cast<NotifyLeakInfo>(faultInfo);
    if (notifyInfo == nullptr) {
        return FAILURE;
    }
    FaultStateType curState = faultInfo->GetState();
    string secondFrame = LEAK_GRADE_STRING[notifyInfo->GetLeakGrade()];
    if (notifyInfo->GetCommpressStatus() == NEED_COMPRESS_AND_COMPRESSED) {
        secondFrame = LEAK_GRADE_STRING[LEAK_INFO];
    } else if (curState == PROC_REPORT_OVERLIMIT_STATE) {
        return ChangeNextState(faultInfo, detectorObj);
    }
    EventQuota eq = notifyInfo->GetEventQuota();
    HandleParam(faultInfo, secondFrame);

    int ret = HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, "MEMORY_LEAK", HiSysEvent::EventType::FAULT,
        "PID", faultInfo->GetPid(),
        "TELEMETRY_ID", eq.telemetryId,
        "TID", notifyInfo->GetTid(),
        "HAPPEN_TIME", faultInfo->GetLeakedTime(),
        "MEMTOTAL", "0",
        "MODULE", faultInfo->GetProcessName().empty() ? "/" : faultInfo->GetProcessName(),
        "VERSION", faultInfo->GetHapVersion().empty() ? "/" : faultInfo->GetHapVersion(),
        "FIRST_FRAME", NOTIFY_TYPE_STRING[notifyInfo->GetNotifyType()],
        "SECOND_FRAME", secondFrame,
        // hask calculate by hap name and version
        "FINGERPRINT", DfrUtil::GetInstance().CalcFingerPrint(faultInfo->GetProcessName() +
            NOTIFY_TYPE_STRING[notifyInfo->GetNotifyType()] + faultInfo->GetHapVersion() +
            LEAK_GRADE_STRING[notifyInfo->GetLeakGrade()] + to_string(notifyInfo->GetCommpressStatus()), 0),
        "LOG_PATH", notifyInfo->GetLogPathForSysEvent(),
        "FOCUS_MODE", DfrUtil::GetInstance().GetFocusMode(),
        "APP_RUNNING_ID", faultInfo->GetAppRunningId(),
        "TELEMETRY_TASK", DfrUtil::GetInstance().GetLeakTypeName(eq.faultType, eq.subLeakType),
        "SUMMARY", notifyInfo->GetLeakStack()
    );
    if (ret < 0) {
        HIVIEW_LOGE("HiSysEventWrite failed, ret=%{public}d", ret);
    }
    return ChangeNextState(faultInfo, detectorObj);
}

ErrCode NotifyLeakReportState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    FaultStateType curState = faultInfo->GetState();
    FaultStateType nextState;
    auto notifyInfo = static_pointer_cast<NotifyLeakInfo>(faultInfo);
    LeakGrade leakGrade = notifyInfo->GetLeakGrade();
    if (curState == PROC_REPORT_OVERLIMIT_STATE) {
        nextState = PROC_FINISHED_STATE;
    } else if (faultInfo->IsApp() && notifyInfo->GetNotifyType() == JS_LEAK && leakGrade == LEAK_CONTROL) {
        nextState = PROC_APP_REPORT_STATE;
    } else {
        nextState = PROC_FINISHED_STATE;
    }
    return detectorObj.ExeNextStateProcess(faultInfo, nextState);
}

} // namespace HiviewDFX
} // namespace OHOS
