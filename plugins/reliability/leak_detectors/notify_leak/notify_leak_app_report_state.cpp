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
#include "notify_leak_app_report_state.h"
#include "notify_leak_detector.h"
#include "dfr_util.h"
#include "hiview_logger.h"
#include "fault_common.h"
#include "hisysevent.h"
#include "old_log_cleaner.h"

using namespace std;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "NotifyLeakAppReportState");

ErrCode NotifyLeakAppReportState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    HIVIEW_LOGI("start");
    auto notifyInfo = static_pointer_cast<NotifyLeakInfo>(faultInfo);
    if (!notifyInfo->IsApp()) {
        return ChangeNextState(faultInfo, detectorObj);
    }

    ffrt::this_task::sleep_for(chrono::microseconds(WAIT_HISYSEVENT_LOGPACK_FINISHED * US_PER_SECOND));
    if (notifyInfo->ShouldPostEvent()) {
        PostEvent(faultInfo);
        ffrt::this_task::sleep_for(chrono::microseconds(WAIT_FILE_COPY_FINISHED * US_PER_SECOND));
    }
    return ChangeNextState(faultInfo, detectorObj);
}

ErrCode NotifyLeakAppReportState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    // remove rawheap after sent appevent
    auto notifyInfo = static_pointer_cast<NotifyLeakInfo>(faultInfo);
    if (notifyInfo == nullptr) {
        return FAILURE;
    }
    if (!DfrUtil::GetInstance().IsBetaOrDevelopMode() && !notifyInfo->CouldTelemetry()) {
        OldLogCleaner::GetInstance().DeleteLogFile(notifyInfo->GetJsLeakFilePath());
    }
    return detectorObj.ExeNextStateProcess(faultInfo, PROC_REPORT_OVERLIMIT_STATE);
}

void NotifyLeakAppReportState::PostEvent(shared_ptr<FaultInfoBase> &faultInfo)
{
    auto notifyInfo = static_pointer_cast<NotifyLeakInfo>(faultInfo);

    AppEventHandler::ResourceOverLimitInfo info;
    info.pid = notifyInfo->GetPid();
    info.uid = notifyInfo->GetUid();
    info.appRunningUniqueId = notifyInfo->GetAppRunningId();
    info.bundleName = notifyInfo->GetProcessName();
    info.bundleVersion = notifyInfo->GetHapVersion();
    info.resourceType = "js_heap";
    info.limitSize = static_cast<uint64_t>(notifyInfo->GetLimitSize());
    info.liveobjectSize = static_cast<uint64_t>(notifyInfo->GetLiveObjectSize());
    info.logPath.push_back("\"" + notifyInfo->GetJsLeakFilePath() + "\"");
    if (notifyInfo->HasStaticJsHeap()) {
        info.logPath.push_back("\"" + notifyInfo->GetStaticJsLeakFilePath() + "\"");
    }
    if (notifyInfo->GetHeapdumpType() == "OOMDump") {
        info.level = "kill";
    }
    DfrUtil::GetInstance().PostAppEvent(info);
    HIVIEW_LOGI("js_heap postFaultEvent end");
}
} // namespace HiviewDFX
} // namespace OHOS
