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
#include "faultlog_event_factory.h"

#include "constants.h"
#include "faultlog_cjerror.h"
#include "faultlog_jserror.h"
#include "faultlog_sanitizer.h"
#include "faultlog_rust_panic.h"
#include "faultlog_page_info.h"
#include "faultlog_cppcrash.h"
#include "faultlog_freeze.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");

std::unique_ptr<FaultLogEventInterface> FaultLogEventFactory::CreateFaultLogEvent(const FaultLogType type)
{
    switch (type) {
        case FaultLogType::JS_CRASH:
            return std::make_unique<FaultLogJsError>();
        case FaultLogType::CJ_ERROR:
            return std::make_unique<FaultLogCjError>();
        case FaultLogType::RUST_PANIC:
            return std::make_unique<FaultLogRustPanic>();
        case FaultLogType::ADDR_SANITIZER:
            return std::make_unique<FaultLogSanitizer>();
        case FaultLogType::CPP_CRASH:
            return std::make_unique<FaultLogCppCrash>();
        case FaultLogType::PROCESS_PAGE_INFO:
            return std::make_unique<FaultLogPageInfo>();
        case FaultLogType::APP_FREEZE:
        case FaultLogType::APPFREEZE_WARNING:
        case FaultLogType::SYS_FREEZE:
        case FaultLogType::SYS_WARNING:
            return std::make_unique<FaultLogFreeze>();
        default:
            HIVIEW_LOGW("Failed to create the FaultLogEventInterface, type is %{public}d", type);
            return nullptr;
    }
}
} // namespace HiviewDFX
} // namespace OHOS
