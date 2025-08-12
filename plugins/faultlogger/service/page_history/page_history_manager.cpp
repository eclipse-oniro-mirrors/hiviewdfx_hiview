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

#include "page_history_manager.h"

#include "hiview_logger.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "Faultlogger");
namespace {
constexpr const char* const INTERACTION_COMPLETED_LATENCY = "INTERACTION_COMPLETED_LATENCY";
constexpr const char* const ABILITY_OR_PAGE_SWITCH = "ABILITY_OR_PAGE_SWITCH";
constexpr const char* const ABILITY_ONFOREGROUND = "ABILITY_ONFOREGROUND";
constexpr const char* const ABILITY_ONBACKGROUND = "ABILITY_ONBACKGROUND";
}

PageHistoryManager& PageHistoryManager::GetInstance()
{
    static PageHistoryManager instance;
    return instance;
}

void PageHistoryManager::HandleEvent(const Event& event)
{
    if (event.eventName_ != INTERACTION_COMPLETED_LATENCY &&
        event.eventName_ != ABILITY_ONFOREGROUND &&
        event.eventName_ != ABILITY_ONBACKGROUND) {
        HIVIEW_LOGE("event(%{public}s) is not match.", event.eventName_.c_str());
        return;
    }
    Event& eventRef = const_cast<Event&>(event);
    SysEvent& sysEvent = static_cast<SysEvent&>(eventRef);

    PageTraceNode node;
    node.pid_ = sysEvent.GetPid();
    node.timestamp_ = sysEvent.happenTime_;
    if (sysEvent.eventName_ == INTERACTION_COMPLETED_LATENCY) {
        if (sysEvent.GetEventValue("SCENE_ID") != ABILITY_OR_PAGE_SWITCH) {
            return;
        }
        node.SetPageInfo(sysEvent.GetEventValue("PAGE_URL"), sysEvent.GetEventValue("PAGE_NAME"));
    } else if (sysEvent.eventName_ == ABILITY_ONFOREGROUND) {
        node.SetPageInfo("", "enters foreground");
    } else if (sysEvent.eventName_ == ABILITY_ONBACKGROUND) {
        node.SetPageInfo("", "leaves foreground");
    } else {
        return;
    }
    auto processName = sysEvent.GetEventValue("BUNDLE_NAME");

    recorder_.PutPageTrace(processName, std::move(node));
}

std::string PageHistoryManager::GetPageHistory(const std::string& processName, int64_t pid)
{
    return recorder_.GetPageTrace(processName, pid);
}
} // HiviewDFX
} // OHOS
