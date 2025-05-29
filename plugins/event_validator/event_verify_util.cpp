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
#include "event_verify_util.h"

#include "focused_event_util.h"
#include "hiview_logger.h"
#include "running_status_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("EventVerifyUtil");
constexpr char TEST_TYPE_KEY[] = "test_type_";

uint64_t GenerateHash(std::shared_ptr<SysEvent> event)
{
    constexpr size_t infoLenLimit = 256;
    size_t infoLen = event->rawData_->GetDataLength();
    size_t hashLen = (infoLen < infoLenLimit) ? infoLen : infoLenLimit;
    const uint8_t* p = event->rawData_->GetData();
    uint64_t ret { 0xCBF29CE484222325ULL }; // basis value
    size_t i = 0;
    while (i < hashLen) {
        ret ^= *(p + i);
        ret *= 0x100000001B3ULL; // prime value
        i++;
    }
    return ret;
}

void EventVerifyUtil::Init(HiviewContext* context)
{
    eventPeriodInfoUtil_.Init(context);
    paramWatcher_.Init();
}

bool EventVerifyUtil::IsValidEvent(std::shared_ptr<SysEvent> event)
{
    if (!IsValidSysEvent(event)) {
        return false;
    }

    if (FocusedEventUtil::IsFocusedEvent(event->domain_, event->eventName_)) {
        HIVIEW_LOGI("event[%{public}s|%{public}s|%{public}" PRIu64 "] is valid.",
            event->domain_.c_str(), event->eventName_.c_str(), event->happenTime_);
    }
    eventPeriodInfoUtil_.UpdatePeriodInfo(event);
    return true;
}

bool EventVerifyUtil::IsValidSysEvent(const std::shared_ptr<SysEvent> event)
{
    if (event->domain_.empty() || event->eventName_.empty()) {
        HIVIEW_LOGW("domain=%{public}s or name=%{public}s is empty.",
            event->domain_.c_str(), event->eventName_.c_str());
        return false;
    }
    auto baseInfo = EventJsonParser::GetInstance()->GetDefinedBaseInfoByDomainName(event->domain_, event->eventName_);
    if (baseInfo.keyConfig.type == INVALID_EVENT_TYPE) {
        HIVIEW_LOGW("type defined for event[%{public}s|%{public}s|%{public}" PRIu64 "] invalid, or privacy dismatch.",
            event->domain_.c_str(), event->eventName_.c_str(), event->happenTime_);
        return false;
    }
    if (event->GetEventType() != baseInfo.keyConfig.type) {
        HIVIEW_LOGW("type=%{public}d of event[%{public}s|%{public}s|%{public}" PRIu64 "] is invalid.",
            event->GetEventType(), event->domain_.c_str(), event->eventName_.c_str(), event->happenTime_);
        return false;
    }

    // deduplicate Event
    auto eventId = GenerateHash(event);
    if (IsDuplicateEvent(eventId)) {
        HIVIEW_LOGW("ignore duplicate event[%{public}s|%{public}s|%{public}" PRIu64 "].",
            event->domain_.c_str(), event->eventName_.c_str(), eventId);
        return false;
    }

    // append extra event info
    DecorateSysEvent(event, baseInfo, eventId);
    return true;
}

bool EventVerifyUtil::IsDuplicateEvent(const uint64_t eventId)
{
    for (auto iter = eventIdList_.rbegin(); iter != eventIdList_.rend(); ++iter) {
        if (*iter == eventId) {
            return true;
        }
    }
    std::list<uint64_t>::size_type maxSize { 25 }; // size of queue limit to 25
    if (eventIdList_.size() >= maxSize) {
        eventIdList_.pop_front();
    }
    eventIdList_.emplace_back(eventId);
    return false;
}

void EventVerifyUtil::DecorateSysEvent(const std::shared_ptr<SysEvent> event, const BaseInfo& baseInfo, uint64_t id)
{
    if (!baseInfo.level.empty()) {
        event->SetLevel(baseInfo.level);
    }
    if (!baseInfo.tag.empty()) {
        event->SetTag(baseInfo.tag);
    }
    event->SetPrivacy(baseInfo.keyConfig.privacy);
    std::string testType = paramWatcher_.GetTestType();
    if (!testType.empty()) {
        event->SetEventValue(TEST_TYPE_KEY, testType);
    }
    event->preserve_ = baseInfo.keyConfig.preserve;
    event->collect_ = baseInfo.keyConfig.collect;
    event->SetId(id); // add hashcode id
    event->SetInvalidParams(baseInfo.disallowParams);
}
} // namespace HiviewDFX
} // namespace OHOS
