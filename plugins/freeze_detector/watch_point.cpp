/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "watch_point.h"

#include <iostream>

namespace OHOS {
namespace HiviewDFX {
WatchPoint::WatchPoint()
    : seq_(0), timestamp_(0), pid_(0), tid_(0), uid_(0), terminalThreadStack_(""), telemetryId_(""), traceName_(""),
    domain_(""), stringId_(""), msg_(""), hitraceIdInfo_(""), procStatm_(""), hostResourceWarning_(""),
    freezeExtFile_(""), appRunningUniqueId_(""), applicationInfo_(""), taskName_(""),
    timeoutEventId_(""), lastDispatchEventId_(""), lastProcessEventId_(""), lastMarkedEventId_(""), thermalLevel_("")
{
}

WatchPoint::WatchPoint(const WatchPoint::Builder& builder)
    : seq_(builder.seq_),
    timestamp_(builder.timestamp_),
    pid_(builder.pid_),
    tid_(builder.tid_),
    uid_(builder.uid_),
    terminalThreadStack_(builder.terminalThreadStack_),
    telemetryId_(builder.telemetryId_),
    traceName_(builder.traceName_),
    domain_(builder.domain_),
    stringId_(builder.stringId_),
    msg_(builder.msg_),
    packageName_(builder.packageName_),
    processName_(builder.processName_),
    foreGround_(builder.foreGround_),
    logPath_(builder.logPath_),
    hitraceTime_(builder.hitraceTime_),
    sysrqTime_(builder.sysrqTime_),
    hitraceIdInfo_(builder.hitraceIdInfo_),
    procStatm_(builder.procStatm_),
    hostResourceWarning_(builder.hostResourceWarning_),
    freezeExtFile_(builder.freezeExtFile_),
    appRunningUniqueId_(builder.appRunningUniqueId_),
    applicationInfo_(builder.applicationInfo_),
    taskName_(builder.taskName_),
    timeoutEventId_(builder.timeoutEventId_),
    lastDispatchEventId_(builder.lastDispatchEventId_),
    lastProcessEventId_(builder.lastProcessEventId_),
    lastMarkedEventId_(builder.lastMarkedEventId_),
    thermalLevel_(builder.thermalLevel_)
{
}

WatchPoint::Builder::Builder()
    : seq_(0), timestamp_(0), pid_(0), tid_(0), uid_(0), terminalThreadStack_(""), telemetryId_(""), traceName_(""),
    domain_(""), stringId_(""), msg_(""), hitraceIdInfo_(""), procStatm_(""), hostResourceWarning_(""),
    freezeExtFile_(""), appRunningUniqueId_(""), applicationInfo_(""), taskName_(""),
    timeoutEventId_(""), lastDispatchEventId_(""), lastProcessEventId_(""), lastMarkedEventId_(""), thermalLevel_("")
{
}

WatchPoint::Builder::~Builder() {}

WatchPoint::Builder& WatchPoint::Builder::InitSeq(long seq)
{
    seq_ = seq;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitTimestamp(unsigned long long timestamp)
{
    timestamp_ = timestamp;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitPid(long pid)
{
    pid_ = pid;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitTid(long tid)
{
    tid_ = tid;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitUid(long uid)
{
    uid_ = uid;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitTerminalThreadStack(const std::string& terminalThreadStack)
{
    terminalThreadStack_ = terminalThreadStack;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitTelemetryId(const std::string& telemetryId)
{
    telemetryId_ = telemetryId;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitTraceName(const std::string& traceName)
{
    traceName_ = traceName;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitDomain(const std::string& domain)
{
    domain_ = domain;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitStringId(const std::string& stringId)
{
    stringId_ = stringId;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitMsg(const std::string& msg)
{
    msg_ = msg;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitProcessName(const std::string& processName)
{
    processName_ = processName;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitPackageName(const std::string& packageName)
{
    packageName_ = packageName;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitForeGround(const std::string& foreGround)
{
    foreGround_ = foreGround;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitLogPath(const std::string& logPath)
{
    logPath_ = logPath;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitHitraceTime(const std::string& hitraceTime)
{
    hitraceTime_ = hitraceTime;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitSysrqTime(const std::string& sysrqTime)
{
    sysrqTime_ = sysrqTime;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitHitraceIdInfo(const std::string& hitraceIdInfo)
{
    hitraceIdInfo_ = hitraceIdInfo;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitProcStatm(const std::string& procStatm)
{
    procStatm_ = procStatm;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitHostResourceWarning(const std::string& hostResourceWarning)
{
    hostResourceWarning_ = hostResourceWarning;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitFreezeExtFile(const std::string& freezeExtFile)
{
    freezeExtFile_ = freezeExtFile;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitAppRunningUniqueId(const std::string& appRunningUniqueId)
{
    appRunningUniqueId_ = appRunningUniqueId;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitApplicationInfo(const std::string& applicationInfo)
{
    applicationInfo_ = applicationInfo;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitTaskName(const std::string& taskName)
{
    taskName_ = taskName;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitTimeoutEventId(const std::string& timeoutEventId)
{
    timeoutEventId_ = timeoutEventId;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitLastDispatchEventId(const std::string& lastDispatchEventId)
{
    lastDispatchEventId_ = lastDispatchEventId;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitLastProcessEventId(const std::string& lastProcessEventId)
{
    lastProcessEventId_ = lastProcessEventId;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitLastMarkedEventId(const std::string& lastMarkedEventId)
{
    lastMarkedEventId_ = lastMarkedEventId;
    return *this;
}

WatchPoint::Builder& WatchPoint::Builder::InitThermalLevel(const std::string& thermalLevel)
{
    thermalLevel_ = thermalLevel;
    return *this;
}

WatchPoint WatchPoint::Builder::Build() const
{
    WatchPoint watchPoint = WatchPoint(*this);
    return watchPoint;
}

long WatchPoint::GetSeq() const
{
    return seq_;
}

unsigned long long WatchPoint::GetTimestamp() const
{
    return timestamp_;
}

long WatchPoint::GetPid() const
{
    return pid_;
}

long WatchPoint::GetTid() const
{
    return tid_;
}

long WatchPoint::GetUid() const
{
    return uid_;
}

std::string WatchPoint::GetTerminalThreadStack() const
{
    return terminalThreadStack_;
}

std::string WatchPoint::GetTelemetryId() const
{
    return telemetryId_;
}

std::string WatchPoint::GetTraceName() const
{
    return traceName_;
}

std::string WatchPoint::GetDomain() const
{
    return domain_;
}

std::string WatchPoint::GetStringId() const
{
    return stringId_;
}

std::string WatchPoint::GetMsg() const
{
    return msg_;
}

std::string WatchPoint::GetPackageName() const
{
    return packageName_;
}

std::string WatchPoint::GetProcessName() const
{
    return processName_;
}

std::string WatchPoint::GetForeGround() const
{
    return foreGround_;
}

std::string WatchPoint::GetLogPath() const
{
    return logPath_;
}

std::string WatchPoint::GetHitraceTime() const
{
    return hitraceTime_;
}

std::string WatchPoint::GetSysrqTime() const
{
    return sysrqTime_;
}

std::string WatchPoint::GetHitraceIdInfo() const
{
    return hitraceIdInfo_;
}

std::string WatchPoint::GetProcStatm() const
{
    return procStatm_;
}

std::string WatchPoint::GetHostResourceWarning() const
{
    return hostResourceWarning_;
}

std::string WatchPoint::GetFreezeExtFile() const
{
    return freezeExtFile_;
}

std::string WatchPoint::GetAppRunningUniqueId() const
{
    return appRunningUniqueId_;
}

std::string WatchPoint::GetApplicationInfo() const
{
    return applicationInfo_;
}

std::string WatchPoint::GetTaskName() const
{
    return taskName_;
}

std::string WatchPoint::GetTimeoutEventId() const
{
    return timeoutEventId_;
}

std::string WatchPoint::GetLastDispatchEventId() const
{
    return lastDispatchEventId_;
}

std::string WatchPoint::GetLastProcessEventId() const
{
    return lastProcessEventId_;
}

std::string WatchPoint::GetLastMarkedEventId() const
{
    return lastMarkedEventId_;
}

std::string WatchPoint::GetThermalLevel() const
{
    return thermalLevel_;
}

void WatchPoint::SetLogPath(const std::string& logPath)
{
    logPath_ = logPath;
}

void WatchPoint::SetFreezeExtFile(const std::string& freezeExtFile)
{
    freezeExtFile_ = freezeExtFile;
}

void WatchPoint::SetTerminalThreadStack(const std::string& terminalThreadStack)
{
    terminalThreadStack_ = terminalThreadStack;
}

void WatchPoint::SetSeq(long seq)
{
    seq_ = seq;
}

void WatchPoint::SetTimeoutEventId(const std::string& timeoutEventId)
{
    timeoutEventId_ = timeoutEventId;
}

void WatchPoint::SetLastDispatchEventId(const std::string& lastDispatchEventId)
{
    lastDispatchEventId_ = lastDispatchEventId;
}

void WatchPoint::SetLastProcessEventId(const std::string& lastProcessEventId)
{
    lastProcessEventId_ = lastProcessEventId;
}

void WatchPoint::SetLastMarkedEventId(const std::string& lastMarkedEventId)
{
    lastMarkedEventId_ = lastMarkedEventId;
}

void WatchPoint::SetThermalLevel(const std::string& thermalLevel)
{
    thermalLevel_ = thermalLevel;
}

bool WatchPoint::operator<(const WatchPoint& node) const
{
    if (timestamp_ == node.timestamp_) {
        return stringId_.compare(node.GetStringId());
    }
    return timestamp_ < node.timestamp_;
}

bool WatchPoint::operator==(const WatchPoint& node) const
{
    return timestamp_ == node.GetTimestamp() && stringId_.compare(node.GetStringId());
}
} // namespace HiviewDFX
} // namespace OHOS
