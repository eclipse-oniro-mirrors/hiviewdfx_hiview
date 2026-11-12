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

#include "fault_info_base.h"

#include <vector>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include "suspend_manager_client.h"
#include "bundle_mgr_client.h"
#include "ffrt.h"
#include "fault_common.h"
#include "dfr_util.h"
#include "old_log_cleaner.h"
#include "hiview_logger.h"

using namespace std;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "FaultInfoBase");
FaultInfoBase::FaultInfoBase(pid_t pid, string name)
    : pid_(pid), processName_(name)
{
}

FaultStateType FaultInfoBase::GetState() const
{
    return state_;
}

void FaultInfoBase::SetState(FaultStateType state)
{
    state_ = state;
}

string FaultInfoBase::GetProcessName()
{
    return processName_;
}

void FaultInfoBase::SetPid(pid_t pid)
{
    pid_ = pid;
}

pid_t FaultInfoBase::GetPid() const
{
    return pid_;
}

void FaultInfoBase::SetLeakReason(const std::string &reason)
{
    leakReason_ = reason;
}

const std::string& FaultInfoBase::GetLeakReason() const
{
    return leakReason_;
}

bool FaultInfoBase::IsDumping() const
{
    return isdumping_.load();
}

void FaultInfoBase::SetIsDumping(bool flag)
{
    isdumping_.store(flag);
}

string FaultInfoBase::GetLeakedTime() const
{
    return leakedTime_;
}

void FaultInfoBase::SetLeakedTime()
{
    leakedTime_ = DfrUtil::GetInstance().GetRealTimeStampStr();
}

LeakGrade FaultInfoBase::GetLeakGrade() const
{
    return leakGrade_.load();
}

void FaultInfoBase::SetLeakGrade(LeakGrade leakGrade)
{
    leakGrade_.store(leakGrade);
}

time_t FaultInfoBase::GetPidStartTime() const
{
    return startTime_;
}

void FaultInfoBase::SetProcessDied(bool flag)
{
    setProcessDiedMtx_.lock();
    isDied_ = flag;
    setProcessDiedMtx_.unlock();
}

bool FaultInfoBase::IsDied()
{
    setProcessDiedMtx_.lock();
    bool flag = isDied_;
    setProcessDiedMtx_.unlock();
    return flag;
}

void FaultInfoBase::SetDumpLogType(DumpLogType type)
{
    dumpLogType_ = type;
}

DumpLogType FaultInfoBase::GetDumpLogType() const
{
    return dumpLogType_;
}

bool FaultInfoBase::CheckIfProcessAlive() const
{
    return processName_ == DfrUtil::GetInstance().GetProcessName(pid_);
}

void FaultInfoBase::SetUid(pid_t pid)
{
    uid_ = DfrUtil::GetInstance().GetProcessUid(pid);
}

void FaultInfoBase::SetUidDirect(pid_t uid)
{
    uid_ = uid;
}

bool FaultInfoBase::IsApp()
{
    pid_t uid = GetUid();
    return uid >= MIN_APP_USERID;
}

pid_t FaultInfoBase::GetUid()
{
    if (uid_ == -1) {
        uid_ = DfrUtil::GetInstance().GetProcessUid(pid_);
    }
    return uid_;
}

string FaultInfoBase::GetBundleName()
{
    if (bundleName_.empty()) {
        pid_t uid = GetUid();
        bundleName_ = DfrUtil::GetInstance().GetApplicationNameByUid(uid);
    }
    return bundleName_;
}

void FaultInfoBase::UnfreezingProcess()
{
    const string &bundleName = GetBundleName();
    SuspendManager::SuspendManagerClient::GetInstance().ThawOneApplication(
        GetUid(), bundleName, "ACTIVE_BY_DUMP_STACK");
    ffrt::this_task::sleep_for(chrono::microseconds(WAIT_TO_UNFREEZE_PROCESS * US_PER_SECOND));
}

int32_t FaultInfoBase::SendCmd(TrackCmd &trackCmd)
{
    int fd = open(BBOX_PATH, O_RDONLY);
    if (fd < 0) {
        return fd;
    }
    fdsan_exchange_owner_tag(fd, 0, FDSAN_TAG);
    UnfreezingProcess();
    int32_t ret = ioctl(fd, LOGGER_MEMCHECK_COMMAND, &trackCmd);
    fdsan_close_with_tag(fd, FDSAN_TAG);
    return ret;
}

void FaultInfoBase::EnsureBundleInfoLoaded()
{
    if (!bundleVersion_.empty() && !bundleVersionCode_.empty()) {
        return;
    }
    if (!IsApp()) {
        return;
    }
    const string &bundleName = GetBundleName();
    if (bundleName.empty()) {
        return;
    }
    AppExecFwk::BundleInfo info;
    AppExecFwk::BundleMgrClient client;
    if (!client.GetBundleInfo(bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT,
        info, AppExecFwk::Constants::ALL_USERID)) {
        return;
    }
    bundleVersion_ = info.versionName;
    bundleVersionCode_ = to_string(info.versionCode);
}

string FaultInfoBase::GetHapVersion()
{
    if (!bundleVersion_.empty()) {
        return bundleVersion_;
    }
    EnsureBundleInfoLoaded();
    return bundleVersion_;
}

string FaultInfoBase::GetHapVersionCode()
{
    if (!bundleVersionCode_.empty()) {
        return bundleVersionCode_;
    }
    EnsureBundleInfoLoaded();
    return bundleVersionCode_;
}

void FaultInfoBase::SetStartTime()
{
    startTime_ = DfrUtil::GetInstance().GetProcessStartTime(pid_);
}

void FaultInfoBase::SetTelemetryFlag(bool flag)
{
    telemetryFlag_ = flag;
}

bool FaultInfoBase::GetTelemetryFlag()
{
    return telemetryFlag_;
}

bool FaultInfoBase::IsEverSendTelemetryEvent()
{
    return isEverSendTelemetryEvent_;
}

void FaultInfoBase::SetEverSendTelemetryEvent(bool flag)
{
    isEverSendTelemetryEvent_ = flag;
}

MemoryLeakInfoBase::MemoryLeakInfoBase(pid_t pid, string name)
    : FaultInfoBase(pid, name)
{
}

bool MemoryLeakInfoBase::IsTriggerHardThreshold() const
{
    return isTriggerHardThreshold_;
}

void MemoryLeakInfoBase::SetIsTriggerHardThreshold(bool flag)
{
    isTriggerHardThreshold_ = flag;
}

void MemoryLeakInfoBase::SetEventMsg(const string &msg)
{
    eventMsg_ = msg;
}

const string& MemoryLeakInfoBase::GetEventMsg() const
{
    return eventMsg_;
}

void MemoryLeakInfoBase::SetLastGrade(LeakGrade grade)
{
    lastGrade_ = grade;
    lastGradeStamp = DfrUtil::GetInstance().GetRunningMonotonicTime();
}

size_t MemoryLeakInfoBase::GetNextIndex(size_t i)
{
    return (i + 1) % MAX_RECORD_NUM;
}

size_t MemoryLeakInfoBase::GetPervIndex(size_t i)
{
    return (MAX_RECORD_NUM + i - 1) % MAX_RECORD_NUM;
}

void MemoryLeakInfoBase::ReportMemoryLeakEvent()
{
    reportEventType_ = MEMORY_LEAK_EVENT;
}

void MemoryLeakInfoBase::ReportMemoryLeakWpEvent()
{
    reportEventType_ = MEMORY_LEAK_WP_EVENT;
}

string MemoryLeakInfoBase::GetReportEventName()
{
    return REPORT_EVENT_TYPE[reportEventType_];
}

bool MemoryLeakInfoBase::ShouldReport()
{
    return reportEventType_ != MEMORY_LEAK_WP_EVENT || DfrUtil::GetInstance().IsBetaOrDevelopMode();
}

void FaultInfoBase::SetAppRunningId(const std::string &id)
{
    appRunningId_ = id;
}

string FaultInfoBase::GetAppRunningId()
{
    if (appRunningId_ != "") {
        return appRunningId_;
    }
    appRunningId_ = DfrUtil::GetInstance().GetAppRunningUniqueIdByPid(pid_);
    return appRunningId_;
}
} // namespace HiviewDFX
} // namespace OHOS
