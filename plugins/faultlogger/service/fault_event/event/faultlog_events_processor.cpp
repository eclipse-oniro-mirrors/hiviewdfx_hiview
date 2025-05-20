/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "faultlog_events_processor.h"

#include "constants.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");

void FaultLogEventsProcessor::AddSpecificInfo(FaultLogInfo& info)
{
    if (info.faultLogType == FaultLogType::ADDR_SANITIZER && info.reason.find("FDSAN") != std::string::npos) {
        info.sectionMap["APPEND_ORIGIN_LOG"] = info.logPath;
        info.logPath = "";
        return;
    }

    if (info.faultLogType == FaultLogType::JS_CRASH || info.faultLogType == FaultLogType::CJ_ERROR) {
        FaultLogProcessorBase::GetProcMemInfo(info);
    }
    if (info.faultLogType == FaultLogType::JS_CRASH ||
        info.faultLogType == FaultLogType::RUST_PANIC ||
        info.faultLogType == FaultLogType::CJ_ERROR) {
        info.sectionMap[FaultKey::HILOG] = GetHilogByPid(info.pid);
    }
}
} // namespace HiviewDFX
} // namespace OHOS
