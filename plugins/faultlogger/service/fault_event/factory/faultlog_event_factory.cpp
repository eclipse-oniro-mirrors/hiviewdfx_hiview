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
#include "faultlog_event_base.h"
#include "faultlog_jserror.h"
#include "faultlog_sanitizer.h"
#include "faultlog_rust_panic.h"
#include "faultlog_page_info.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");

FaultLogEventFactory::FaultLogEventFactory()
{
    InitializeFaultLogEvents();
}

void FaultLogEventFactory::InitializeFaultLogEvents()
{
    faultLogEvents["JS_ERROR"] = []() {
        return std::make_unique<FaultLogJsError>();
    };
    faultLogEvents["CJ_ERROR"] = []() {
        return std::make_unique<FaultLogCjError>();
    };
    faultLogEvents["RUST_PANIC"] = []() {
        return std::make_unique<FaultLogRustPanic>();
    };
    faultLogEvents["ADDR_SANITIZER"] = []() {
        return std::make_unique<FaultLogSanitizer>();
    };
    faultLogEvents["PROCESS_PAGE_INFO"] = []() {
        return std::make_unique<FaultLogPageInfo>();
    };
}

std::unique_ptr<FaultLogEventInterface> FaultLogEventFactory::CreateFaultLogEvent(const std::string& eventName)
{
    auto it = faultLogEvents.find(eventName);
    if (it != faultLogEvents.end()) {
        return it->second();
    }
    return nullptr;
}
} // namespace HiviewDFX
} // namespace OHOS
