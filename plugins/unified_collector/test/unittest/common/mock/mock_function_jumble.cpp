/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "event_loop.h"
#include "event_publish.h"
#include "sys_event.h"
#include "audit.h"
#include "event.h"
#include "hiview_event_report.h"
#include "base/raw_data_base_def.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventRaw {
bool RawDataBuilder::IsBaseInfo(const std::string& key)
{
    return true;
}

std::shared_ptr<EncodedParam> RawDataBuilder::GetValue(const std::string& key)
{
    return nullptr;
}

int ParseTimeZone(const std::string& tzStr)
{
    return 0;
}
} // namespace EventRaw

void HiviewEventReport::UpdatePluginStats(const std::string &name, const std::string &procName, uint32_t procTime)
{
}

bool Audit::IsEnabled()
{
    return true;
}

Audit::~Audit()
{
    enabled_ = false;
}

bool Audit::WriteAuditEvent(StatsEvent eventType, uint64_t eventId, const std::string& digest)
{
    return true;
}

uint64_t EventLoop::AddTimerEvent(std::shared_ptr<EventHandler> handler, std::shared_ptr<Event> event,
    const Task &task, uint64_t interval, bool repeat)
{
    return 0;
}

void Event::ResetTimestamp()
{
}

int64_t SysEvent::GetEventIntValue(const std::string& key)
{
    return 0;
}

int32_t SysEvent::GetUid() const
{
    return 0;
}

int32_t SysEvent::GetPid() const
{
    return 0;
}

std::string SysEvent::GetEventValue(const std::string& key)
{
    return nullptr;
}

void EventPublish::PushEvent(int32_t uid, const std::string& eventName, HiSysEvent::EventType eventType,
    const std::string& paramJson)
{
}

bool EventLoop::RemoveEvent(uint64_t seq)
{
    return true;
}
} // namespace HiviewDFX
} // namespace OHOS