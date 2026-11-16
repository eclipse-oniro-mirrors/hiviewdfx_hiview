/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "rs_fd_warning_event_handler.h"

#include <ctime>

#include "dfr_util.h"
#include "fd_leak_info.h"
#include "fd_leak_state_context.h"
#include "fault_common.h"
#include "fault_detector_manager.h"
#include "hisysevent.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {

using namespace std;
DEFINE_LOG_LABEL(0xD002DA0, "RsFdWarningEventHandler");

namespace {
constexpr const char* const FD_RS_LEAK_FIRST_FRAME = "FD_RS_LEAK";
constexpr const char* const FD_RS_LEAK_SECOND_FRAME = "ERROR";
}

ffrt::mutex RsFdWarningInfo::reportedMtx_;
map<string, time_t> RsFdWarningInfo::reportedMap_;

RsFdWarningInfo::RsFdWarningInfo(const string &eventName, const RsFdWarningParams &params)
    : LeakEventInfoBase(eventName), params_(params)
{
    SetLeakedTime();
    fdLeakInfo_ = make_shared<FdLeakInfo>(params.pid, params.processName);
    FdEventInfo eventInfo;
    eventInfo.pid = params.pid;
    eventInfo.name = params.processName;
    eventInfo.fdNum = static_cast<uint32_t>(params.fdNum);
    eventInfo.happenTime = time(nullptr);
    fdLeakInfo_->SetFdEventInfo(eventInfo);
    fdLeakInfo_->SetFdNums(static_cast<uint32_t>(params.fdNum));
    if (!params.appRunningId.empty()) {
        fdLeakInfo_->SetAppRunningId(params.appRunningId);
    }
}

string RsFdWarningInfo::GetFilePath()
{
    return string(FILE_DESCRIPTOR_LEAK_PATH);
}

string RsFdWarningInfo::GetFileName()
{
    return fdLeakInfo_->GetFdLogName();
}

pid_t RsFdWarningInfo::GetPid()
{
    return params_.pid;
}

string RsFdWarningInfo::GetProcessName()
{
    return params_.processName;
}

int32_t RsFdWarningInfo::GetFdThreshold()
{
    return params_.fdThreshold;
}

int32_t RsFdWarningInfo::GetFdNum()
{
    return params_.fdNum;
}

string RsFdWarningInfo::GetAppRunningId()
{
    return fdLeakInfo_->GetAppRunningId();
}

bool RsFdWarningInfo::IsEverReported()
{
    // rate limit by process, so that the same process is reported once within 24h
    if (params_.processName.empty()) {
        // process name is unavailable, do not rate limit this event
        return false;
    }
    lock_guard<ffrt::mutex> lock(reportedMtx_);
    auto it = reportedMap_.find(params_.processName);
    if (it == reportedMap_.end()) {
        return false;
    }
    time_t now = time(nullptr);
    if (now <= it->second + static_cast<time_t>(SEC_PER_DAY)) {
        HIVIEW_LOGW("same process already reported within 24h, rate limited, process:%{public}s, "
            "lastReportTime:%{public}s, now:%{public}s", params_.processName.c_str(),
            DfrUtil::GetInstance().TimeStampToStr(it->second).c_str(),
            DfrUtil::GetInstance().TimeStampToStr(now).c_str());
        return true;
    }
    return false;
}

void RsFdWarningInfo::RecordReport()
{
    if (params_.processName.empty()) {
        return;
    }
    lock_guard<ffrt::mutex> lock(reportedMtx_);
    time_t now = time(nullptr);
    // remove expired entries so that reportedMap_ does not grow unbounded
    for (auto it = reportedMap_.begin(); it != reportedMap_.end();) {
        if (now > it->second + static_cast<time_t>(SEC_PER_DAY)) {
            it = reportedMap_.erase(it);
        } else {
            ++it;
        }
    }
    reportedMap_[params_.processName] = now;
}

/******************** RsFdWarningEventHandler *********************** */

shared_ptr<LeakEventInfoBase> RsFdWarningEventHandler::HandleMessage(SysEvent sysEvent)
{
    int callingUid = sysEvent.GetEventIntValue("uid_");
    if (callingUid != RENDER_SERVICE_UID) {
        HIVIEW_LOGE("Calling UID is not RENDER_SERVICE_UID, do not handle %{public}s",
            sysEvent.eventName_.c_str());
        return nullptr;
    }

    std::string type = sysEvent.GetEventValue("TYPE");
    if (type != TYPE_FD_WARNING) {
        HIVIEW_LOGE("TYPE is not FD_WARNING, skip");
        return nullptr;
    }

    pid_t pid = sysEvent.GetEventIntValue("PID");
    int32_t fdThreshold = sysEvent.GetEventIntValue("FD_THRESHOLD");
    if (fdThreshold <= 0 || fdThreshold > FD_WARNING_THRESHOLD_MAX) {
        fdThreshold = FD_WARNING_THRESHOLD_DEFAULT;
    }
    int32_t fdNum = sysEvent.GetEventIntValue("FD_NUM");
    if (fdNum < 0 || fdNum > FD_WARNING_THRESHOLD_MAX) {
        HIVIEW_LOGW("invalid fd num %{public}d, reset to 0, pid:%{public}d", fdNum, pid);
        fdNum = 0;
    }

    RsFdWarningParams params;
    params.pid = pid;
    params.processName = sysEvent.GetEventValue("BUNDLE_NAME");
    if (params.processName.empty()) {
        params.processName = DfrUtil::GetInstance().GetProcessName(pid);
    }
    params.fdThreshold = fdThreshold;
    params.fdNum = fdNum;
    params.appRunningId = sysEvent.GetEventValue("APP_RUNNING_ID");

    auto eventInfo = make_shared<RsFdWarningInfo>("FD_WARNING", params);
    auto fdWarningInfo = static_pointer_cast<RsFdWarningInfo>(eventInfo);
    if (fdWarningInfo->IsEverReported()) {
        return nullptr;
    }
    return eventInfo;
}

ErrCode RsFdWarningEventHandler::DumpMessage(shared_ptr<LeakEventInfoBase> eventInfo)
{
    auto fdWarningInfo = static_pointer_cast<RsFdWarningInfo>(eventInfo);
    auto fdInfo = fdWarningInfo->GetFdLeakInfo();

    // Hand over to FdLeakDumpState so that the warning path shares the dump implementation
    // with the fd leak path; fdInfo already carries the fd num from the event.
    FdLeakStateContext::GetInstance().GetDumpState()->DumpWarningInfo(fdInfo);
    return SUCCESSED;
}

ErrCode RsFdWarningEventHandler::ReportMessage(shared_ptr<LeakEventInfoBase> eventInfo)
{
    auto fdWarningInfo = static_pointer_cast<RsFdWarningInfo>(eventInfo);
    std::string summaryInfo = "rs fd warning, PID: " + to_string(fdWarningInfo->GetPid()) +
        ", BUNDLE_NAME: " + fdWarningInfo->GetProcessName() +
        ", FD_THRESHOLD: " + to_string(fdWarningInfo->GetFdThreshold()) +
        ", FD_NUM: " + to_string(fdWarningInfo->GetFdNum());

    std::string logPath = fdWarningInfo->GetFdLeakInfo()->GetFdLogPath();

    int32_t ret = HiSysEventWrite(HiSysEvent::Domain::RELIABILITY, "FD_LEAK", HiSysEvent::EventType::FAULT,
        "PID", fdWarningInfo->GetPid(),
        "MODULE", fdWarningInfo->GetProcessName().empty() ? "/" : fdWarningInfo->GetProcessName(),
        "HAPPEN_TIME", fdWarningInfo->GetLeakedTime(),
        "FIRST_FRAME", FD_RS_LEAK_FIRST_FRAME,
        "SECOND_FRAME", FD_RS_LEAK_SECOND_FRAME,
        "FINGERPRINT", DfrUtil::GetInstance().CalcFingerPrint(
            fdWarningInfo->GetProcessName() + FD_RS_LEAK_FIRST_FRAME + FD_RS_LEAK_SECOND_FRAME, 0),
        "LOG_PATH", logPath,
        "FOREGROUND", FaultDetectorManager::CheckIfAppForeGround(fdWarningInfo->GetProcessName()) ? "YES" : "NO",
        "APP_RUNNING_ID", fdWarningInfo->GetAppRunningId(),
        "SUMMARY", summaryInfo);
    if (ret < 0) {
        HIVIEW_LOGW("HiSysEventWrite failed, ret:%{public}d, pid:%{public}d", ret, fdWarningInfo->GetPid());
        return FAILURE;
    }
    fdWarningInfo->RecordReport();
    HIVIEW_LOGD("report FD_LEAK (FIRST_FRAME=FD_RS_LEAK) successfully, pid:%{public}d", fdWarningInfo->GetPid());
    return SUCCESSED;
}

} // HiviewDFX
} // OHOS
