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

#include "hisysevent_util.h"

namespace OHOS {
namespace HiviewDFX {
HiSysEventEventType TranslateEventType(HiSysEvent::EventType type)
{
    HiSysEventEventType eventType = HISYSEVENT_FAULT;
    switch (type) {
        case HiSysEvent::EventType::STATISTIC:
            eventType = HISYSEVENT_STATISTIC;
            break;
        case HiSysEvent::EventType::SECURITY:
            eventType = HISYSEVENT_SECURITY;
            break;
        case HiSysEvent::EventType::BEHAVIOR:
            eventType = HISYSEVENT_BEHAVIOR;
            break;
        case HiSysEvent::EventType::FAULT:
        default:
            break;
    }
    return eventType;
}

void TranslateStrVector(const std::vector<std::string>& src, std::vector<char*>& dest)
{
    for (size_t i = 0; i < src.size(); ++i) {
        dest.emplace_back(PARAM_STR(src[i]));
    }
}
} // namespace HiviewDFX
} // namespace OHOS