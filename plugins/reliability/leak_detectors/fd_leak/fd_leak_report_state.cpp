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
#include <fcntl.h>
#include <unistd.h>
#include "fd_leak_report_state.h"
#include "fd_leak_info.h"
#include "fd_leak_dump_client.h"
#include "fd_leak_detector.h"
#include "fault_common.h"
#include "hiview_logger.h"
#include "hisysevent.h"
#include "res_sched_client.h"
#include "res_sched_kill_reason.h"
#include "fault_detector_manager.h"
#include "dfr_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "FdLeakReportState");

using namespace std;

ErrCode FdLeakReportState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    auto fdInfo = static_pointer_cast<FdLeakInfo>(faultInfo);
    bool alreadySetResourceLimit = detectorObj.IsAlreadySetResourceLimit(fdInfo->GetPid(), fdInfo->GetProcessName());
    if (alreadySetResourceLimit) {
        return detectorObj.ExeNextStateProcess(faultInfo, PROC_APP_REPORT_STATE);
    }
    FaultStateType nextState = fdInfo->GetNextState();
    return detectorObj.ExeNextStateProcess(faultInfo, nextState);
}

void FdLeakReportState::CreateMetaInfo(const shared_ptr<FdLeakInfo> &fdInfo, const FdEventInfo &eventInfo,
    const std::string &secondFrame)
{
    MetaInfoParams params = {
        .eventName = "fdleak",
        .processName = eventInfo.name,
        .pid = eventInfo.pid,
        .firstFrame = "0",
        .secondFrame = secondFrame,
        .thirdFrame = "0",
        .timeStamp = DfrUtil::GetInstance().TimeStampToSimplelyStr(eventInfo.happenTime),
        .basePath = FILE_DESCRIPTOR_LEAK_PATH,
        .versionName = eventInfo.version,
        .versionCode = eventInfo.versionCode,
        .foreground = FaultDetectorManager::CheckIfAppForeGround(eventInfo.name) ? "1" : "0",
        .appRunningId = fdInfo->GetAppRunningId()
    };
    DfrUtil::GetInstance().CreateMetaInfoFile(params);
}

void FdLeakReportState::WriteHiSysEvent(const shared_ptr<FdLeakInfo> &fdInfo, const FdEventInfo &eventInfo,
    const std::string &firstFrame, const std::string &secondFrame, const std::string &summaryInfo)
{
    EventQuota eq = fdInfo->GetEventQuota();
    HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, "FD_LEAK", HiSysEvent::EventType::FAULT,
        "TELEMETRY_ID", eq.telemetryId,
        "PID", eventInfo.pid,
        "FD_NUM", eventInfo.fdNum,
        "FINGERPRINT", DfrUtil::GetInstance().CalcFingerPrint(
            eventInfo.name + eventInfo.version + firstFrame + LEAK_GRADE_STRING[fdInfo->GetLeakGrade()], 0),
        "HAPPEN_TIME", DfrUtil::GetInstance().TimeStampToSimplelyStr(eventInfo.happenTime),
        "FIRST_FRAME", firstFrame,
        "SECOND_FRAME", secondFrame,
        "MODULE", eventInfo.name.empty() ? "/" : eventInfo.name,
        "VERSION", eventInfo.version.empty() ? "/" : eventInfo.version,
        "LOG_PATH", fdInfo->GetFdLogPath(),
        "FOCUS_MODE", DfrUtil::GetInstance().GetFocusMode(),
        "FOREGROUND", (FaultDetectorManager::CheckIfAppForeGround(eventInfo.name) ? "YES" : "NO"),
        "APP_RUNNING_ID", fdInfo->GetAppRunningId(),
        "SUMMARY", summaryInfo);
}

ErrCode FdLeakReportState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    HIVIEW_LOGD("begin FdLeakDetector SendBoardCast");
    auto fdInfo = static_pointer_cast<FdLeakInfo>(faultInfo);
    FdEventInfo eventInfo = fdInfo->GetFdEventInfo();
    string summaryInfo = eventInfo.summary;
    if (fdInfo->ShouldBeKilled() == SHOULD_KILL) {
        summaryInfo += "\n has killed:" + eventInfo.name +
            (FaultDetectorManager::CheckIfAppForeGround(eventInfo.name) ? "(foreground)" : "(background)");
    }
    if (!fdInfo->IsEverSendTelemetryEvent() && fdInfo->GetTelemetryFlag()) {
        fdInfo->ConsumeQuota();
        fdInfo->SetEverSendTelemetryEvent();
    }

    FaultStateType curState = faultInfo->GetState();
    const string firstFrame = fdInfo->GetTopFdTypeList().empty() ? "open" : fdInfo->GetTopFdTypeList()[0];
    string secondFrame =
        curState==PROC_REPORT_OVERLIMIT_STATE ? "OVER_LIMIT" : LEAK_GRADE_STRING[faultInfo->GetLeakGrade()];

    CreateMetaInfo(fdInfo, eventInfo, secondFrame);
    WriteHiSysEvent(fdInfo, eventInfo, firstFrame, secondFrame, summaryInfo);
    fdInfo->RecordReport();

    if (DfrUtil::GetInstance().IsApp(faultInfo->GetPid())) {
        DfrUtil::GetInstance().NotifyWarningToApp(eventInfo.pid, AppExecFwk::LeakType::FD,
            static_cast<unsigned long>(eventInfo.fdNum));
    }

    return ChangeNextState(faultInfo, detectorObj);
}
} // namespace HiviewDFX
} // namespace OHOS
