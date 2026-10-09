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
#include "faultlog_file_delete.h"

#include <string>

#include "constants.h"
#include "faultlog_util.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");

FaultLogFileDelete::FaultLogFileDelete()
{
    faultType_ = FaultLogType::FILE_DELETE;
}

std::string FaultLogFileDelete::GetFaultModule(SysEvent& sysEvent) const
{
    return sysEvent.GetEventValue(FaultKey::MODULE_NAME);
}

FaultLogInfo FaultLogFileDelete::FillFaultLogInfo(SysEvent& sysEvent)
{
    // base class fills common fields (time, uid, pid, module, reason, summary, sectionMap)
    auto info = FaultLogEventPipeline::FillFaultLogInfo(sysEvent);
    // read PID and HAPPEN_TIME from event fields to locate the temp stack file;
    // prefix "stacktrace-filedelete-" belongs to the stacktrace family,
    // distinguished from FDSAN "stacktrace-" files by the second segment
    info.pid = sysEvent.GetEventIntValue(FaultKey::MODULE_PID);
    info.time = sysEvent.GetEventIntValue(FaultKey::HAPPEN_TIME);
    info.logPath = GetFileDeleteTempLogName(info);
    // the stacktrace temp file stays in the temp directory, no formal faultlog file is generated
    info.dumpLogToFaultlogger = false;
    info.reportToAppEvent = false;
    return info;
}

void FaultLogFileDelete::UpdateFaultLogInfo() {}

bool FaultLogFileDelete::ReportToAppEvent(std::shared_ptr<SysEvent> sysEvent) const
{
    (void)sysEvent;
    return false;
}
} // namespace HiviewDFX
} // namespace OHOS
