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

#include <string>

#include "constants.h"
#include "faultlog_util.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");

namespace {
constexpr const char *const INODE_ID_KEY = "INODE_ID";
}

FaultLogUndeletable::FaultLogUndeletable()
{
    faultType_ = FaultLogType::UNDELETEABLE_FILE;
}

std::string FaultLogUndeletable::GetFaultModule(SysEvent& sysEvent) const
{
    return sysEvent.GetEventValue(FaultKey::MODULE_NAME);
}

FaultLogInfo FaultLogUndeletable::FillFaultLogInfo(SysEvent& sysEvent)
{
    // base class fills common fields (time, uid, pid, module, reason, summary, sectionMap)
    auto info = FaultLogEventPipeline::FillFaultLogInfo(sysEvent);
    // read PID and HAPPEN_TIME from event fields to locate the temp stack file,
    // independent prefix "undeletable-" distinguishes it from the FDSAN "stacktrace-" files
    info.pid = sysEvent.GetEventIntValue(FaultKey::MODULE_PID);
    info.time = sysEvent.GetEventIntValue(FaultKey::HAPPEN_TIME);
    info.logPath = GetUndeletableTempLogName(info);
    info.summary = "";
    info.sanitizerType = "UNDELETEABLE";
    // extract INODE_ID reported by processdump
    uint32_t inodeId = static_cast<uint32_t>(sysEvent.GetEventIntValue(INODE_ID_KEY));
    info.sectionMap[INODE_ID_KEY] = std::to_string(inodeId);
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
