/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
#include "faultlogger.h"
#include <cstdint>
#include <memory>

#ifdef UNIT_TEST
#include <iostream>
#include <cstring>
#endif
#include <fstream>

#include "export_faultlogger_interface.h"
#include "faultlogger_service_ohos.h"
#include "hiview_logger.h"
#include "page_history_manager.h"
#include "plugin_factory.h"

namespace OHOS {
namespace HiviewDFX {
REGISTER(Faultlogger);
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");

bool Faultlogger::IsInterestedPipelineEvent(std::shared_ptr<Event> event)
{
    if (!hasInit_ || event == nullptr) {
        return false;
    }

    const int eventCount = 4;
    std::array<const char* const, eventCount> eventNames = {
        "JS_ERROR",
        "CJ_ERROR",
        "RUST_PANIC",
        "ADDR_SANITIZER"
    };

    return std::find(eventNames.begin(), eventNames.end(), event->eventName_) != eventNames.end();
}

bool Faultlogger::OnEvent(std::shared_ptr<Event>& event)
{
    if (!hasInit_ || event == nullptr) {
        return false;
    }
    if (event->rawData_ == nullptr) {
        return false;
    }
    auto instance = GetFaultloggerInterface(FAULTLOGGER_LIB_DELAY_RELEASE_TIME);
    if (instance == nullptr) {
        HIVIEW_LOGE("run func failed.");
        return true;
    }
    instance->ProcessFaultLogEvent(event->eventName_, event);
    return true;
}

void Faultlogger::OnEventListeningCallback(const Event& event)
{
    if (!hasInit_ || event.rawData_ == nullptr) {
        return;
    }
    PageHistoryManager::GetInstance().HandleEvent(event);
}

bool Faultlogger::CanProcessEvent(std::shared_ptr<Event> event)
{
    return true;
}

bool Faultlogger::ReadyToLoad()
{
    return true;
}

void Faultlogger::OnLoad()
{
    auto context = GetHiviewContext();
    if (context == nullptr) {
        HIVIEW_LOGE("GetHiviewContext failed.");
        return;
    }
    workLoop_ = context->GetSharedWorkLoop();
    hasInit_ = true;
#ifndef UNITTEST
    FaultloggerServiceOhos::StartService();

    faultLogBootScan_ = std::make_shared<FaultLogBootScanListener>(workLoop_);
    context->RegisterUnorderedEventListener(faultLogBootScan_);
#endif
}

void Faultlogger::Dump(int fd, const std::vector<std::string>& cmds)
{
    auto instance = GetFaultloggerInterface(FAULTLOGGER_LIB_DELAY_RELEASE_TIME);
    if (instance == nullptr) {
        dprintf(fd, "Service is not ready.\n");
        return;
    }
    instance->FaultLogDumpByCommands(fd, cmds);
}
} // namespace HiviewDFX
} // namespace OHOS
