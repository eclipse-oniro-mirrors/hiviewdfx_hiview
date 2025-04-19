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

#ifdef UNIT_TEST
#include <iostream>
#include <cstring>
#endif
#include <fstream>

#include "faultlog_dump.h"
#include "faultlog_event_factory.h"
#include "faultlog_util.h"
#include "faultlogger_service_ohos.h"
#include "hiview_logger.h"
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

    const int eventCount = 5;
    std::array<std::string, eventCount> eventNames = {
        "PROCESS_EXIT",
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
    FaultLogEventFactory factory;
    auto faultLogEvent = factory.CreateFaultLogEvent(event->eventName_);
    if (faultLogEvent) {
        return faultLogEvent->ProcessFaultLogEvent(event, workLoop_, faultLogManager_);
    }
    return true;
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
    faultLogManager_ = std::make_shared<FaultLogManager>(workLoop_);
    faultLogManager_->Init();
    hasInit_ = true;
#ifndef UNITTEST
    FaultloggerServiceOhos::StartService(std::make_shared<FaultLogManagerService>(workLoop_, faultLogManager_));

    faultLogBootScan_ = std::make_shared<FaultLogBootScan>(workLoop_, faultLogManager_);
    context->RegisterUnorderedEventListener(faultLogBootScan_);
#endif
}

void Faultlogger::Dump(int fd, const std::vector<std::string>& cmds)
{
    FaultLogDump faultLogDump(fd, faultLogManager_);
    faultLogDump.DumpByCommands(cmds);
}
} // namespace HiviewDFX
} // namespace OHOS
