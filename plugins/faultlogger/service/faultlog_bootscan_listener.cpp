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
#include "faultlog_bootscan_listener.h"

#include "export_faultlogger_interface.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");

namespace {
constexpr time_t FORTYEIGHT_HOURS = 48 * 60 * 60;
}

void FaultLogBootScanListener::AddBootScanEvent()
{
    if (workLoop_ == nullptr) {
        HIVIEW_LOGE("workLoop_ is nullptr.");
        return;
    }
    // some crash happened before hiview start, ensure every crash event is added into eventdb
    auto task = [this] {
        auto instance = GetFaultloggerInterface(FAULTLOGGER_LIB_DELAY_RELEASE_TIME);
        if (instance == nullptr) {
            return;
        }
        instance->StartFaultLogBootScan();
    };
    workLoop_->AddTimerEvent(nullptr, nullptr, task, 60, false); // delay 60 seconds
}

FaultLogBootScanListener::FaultLogBootScanListener(std::shared_ptr<EventLoop> workLoop)
    : workLoop_(workLoop)
{
    AddListenerInfo(Event::MessageType::PLUGIN_MAINTENANCE);
    AddListenerInfo(Event::TELEMETRY_EVENT);
}

void FaultLogBootScanListener::OnUnorderedEvent(const Event& msg)
{
    if (msg.messageType_ == Event::TELEMETRY_EVENT) {
        auto instance = GetFaultloggerInterface(FAULTLOGGER_LIB_DELAY_RELEASE_TIME);
        if (instance == nullptr) {
            return;
        }
        instance->SanitizerHandleUnorderedEvent(msg);
    }
#ifndef UNITTEST
    if (msg.messageType_ != Event::MessageType::PLUGIN_MAINTENANCE ||
        msg.eventId_ != Event::EventId::PLUGIN_LOADED) {
        HIVIEW_LOGE("messageType_(%{public}u), eventId_(%{public}u).", msg.messageType_, msg.eventId_);
        return;
    }
    AddBootScanEvent();
#endif
}

std::string FaultLogBootScanListener::GetListenerName()
{
    return "Faultlogger";
}
} // namespace HiviewDFX
} // namespace OHOS
