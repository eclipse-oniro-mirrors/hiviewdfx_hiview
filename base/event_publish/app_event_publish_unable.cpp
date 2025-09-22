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
#include "app_event_elapsed_time.h"
#include "app_event_handler.h"
#include "app_event_publisher_factory.h"
#include "event_publish.h"
#include "user_data_size_reporter.h"

namespace OHOS {
namespace HiviewDFX {
ElapsedTime::ElapsedTime()
{}

ElapsedTime::ElapsedTime(uint64_t limitCostMilliseconds, std::string printContent)
{}

ElapsedTime::~ElapsedTime()
{}

void ElapsedTime::MarkElapsedTime(const std::string& markContent)
{}

int AppEventHandler::PostEvent(const AppLaunchInfo& event)
{
    return -1;
}

int AppEventHandler::PostEvent(const ScrollJankInfo& event)
{
    return -1;
}

int AppEventHandler::PostEvent(const ResourceOverLimitInfo& event)
{
    return -1;
}

int AppEventHandler::PostEvent(const CpuUsageHighInfo& event)
{
    return -1;
}

int AppEventHandler::PostEvent(const BatteryUsageInfo& event)
{
    return -1;
}

int AppEventHandler::PostEvent(const AppKilledInfo& event)
{
    return -1;
}

int AppEventHandler::PostEvent(const AudioJankFrameInfo& event)
{
    return -1;
}

bool AppEventHandler::IsAppListenedEvent(int32_t uid, const std::string& eventName)
{
    return false;
}

bool AppEventPublisherFactory::IsPublisher(const std::string& name)
{
    return false;
}

void AppEventPublisherFactory::RegisterPublisher(const std::string& name)
{}

void AppEventPublisherFactory::UnregisterPublisher(const std::string& name)
{}

class EventPublish::Impl {};

EventPublish::EventPublish()
    : impl_(std::make_unique<Impl>())
{}

EventPublish::~EventPublish() = default;

EventPublish& EventPublish::GetInstance()
{
    static EventPublish publisher;
    return publisher;
}

void EventPublish::PushEvent(int32_t uid, const std::string& eventName,
    HiSysEvent::EventType eventType, const std::string& paramJson, uint32_t maxFileSizeBytes)
{}

bool EventPublish::IsAppListenedEvent(int32_t uid, const std::string& eventName)
{
    return false;
}

void UserDataSizeReporter::ReportUserDataSize(int32_t uid, const std::string& pathHolder, const std::string& eventName)
{}

bool UserDataSizeReporter::ShouldReport(const std::string& pathHolder)
{
    return false;
}

void UserDataSizeReporter::ClearOverTimeRecord()
{}
} // namespace HiviewDFX
} // namespace OHOS