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
#include "faultlog_undeletable.h"

#include <memory>
#include <string>

#include "constants.h"
#include "faultlog_util.h"
#include "file_util.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");

namespace {
constexpr const char *const UNDELETEABLE_REASON_KEY = "UNDELETEABLE";
}

FaultLogUndeletable::FaultLogUndeletable()
{
    faultType_ = FaultLogType::UNDELETEABLE_FILE;
}

std::string FaultLogUndeletable::GetFaultModule(SysEvent& sysEvent) const
{
    return sysEvent.GetEventValue(FaultKey::MODULE_NAME);
}

bool FaultLogUndeletable::IsValidUndeletableEvent(SysEvent& sysEvent)
{
    const std::string reason = sysEvent.GetEventValue(FaultKey::REASON);
    if (reason.find(UNDELETEABLE_REASON_KEY) == std::string::npos) {
        HIVIEW_LOGE("Invalid undeletable event, reason mismatch");
        return false;
    }
    const int64_t uid = static_cast<int64_t>(sysEvent.GetUid());
    const int64_t eventUid = sysEvent.GetEventIntValue(FaultKey::MODULE_UID);
    if (uid != eventUid) {
        HIVIEW_LOGE("Invalid undeletable event, uid is mismatch");
        return false;
    }
    int32_t pid = sysEvent.GetEventIntValue(FaultKey::MODULE_PID);
    int64_t happenTime = sysEvent.GetEventIntValue(FaultKey::HAPPEN_TIME);
    const std::string logPath = GetUndeletableTempLogName(pid, happenTime);
    if (!FileUtil::FileExists(logPath)) {
        HIVIEW_LOGE("Invalid undeletable event, processdump log does not exist, path=%{public}s", logPath.c_str());
        return false;
    }
    return true;
}

bool FaultLogUndeletable::AddFaultLog(std::shared_ptr<Event>& event)
{
    if (event == nullptr) {
        HIVIEW_LOGE("Undeletable event is null");
        return false;
    }
    auto sysEvent = std::static_pointer_cast<SysEvent>(event);
    if (!IsValidUndeletableEvent(*sysEvent)) {
        HIVIEW_LOGE("Reject invalid undeletable event");
        return false;
    }
    return FaultLogEventPipeline::AddFaultLog(event);
}

FaultLogInfo FaultLogUndeletable::FillFaultLogInfo(SysEvent& sysEvent)
{
    auto info = FaultLogEventPipeline::FillFaultLogInfo(sysEvent);
    info.pid = sysEvent.GetEventIntValue(FaultKey::MODULE_PID);
    info.time = sysEvent.GetEventIntValue(FaultKey::HAPPEN_TIME);
    info.logPath = GetUndeletableTempLogName(info.pid, info.time);
    info.summary = "";
    // the stacktrace temp file stays in the temp directory, no formal faultlog file is generated;
    // the pipeline only enriches the event (FINGERPRINT/FIRST_FRAME/LOG_PATH etc.) for event store
    info.dumpLogToFaultlogger = false;
    info.reportToAppEvent = false;
    return info;
}

void FaultLogUndeletable::UpdateFaultLogInfo() {}

bool FaultLogUndeletable::ReportToAppEvent(std::shared_ptr<SysEvent> sysEvent) const
{
    (void)sysEvent;
    return false;
}
} // namespace HiviewDFX
} // namespace OHOS
