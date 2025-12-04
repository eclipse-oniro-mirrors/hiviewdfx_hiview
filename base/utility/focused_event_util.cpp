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

#include "focused_event_util.h"

#include <list>
#include <map>

namespace OHOS {
namespace HiviewDFX {
namespace FocusedEventUtil {
namespace {
const std::map<std::string, std::list<std::string>> FOCUSED_EVENT_MAP {
    {"AAFWK", {"THREAD_BLOCK_3S", "THREAD_BLOCK_6S"}},
    {"ACE", {"UI_BLOCK_3S", "UI_BLOCK_6S"}},
    {"FRAMEWORK", {"SERVICE_BLOCK", "SERVICE_WARNING"}},
    {"HMOS_SVC_BROKER", {"CONTAINER_LIFECYCLE_EVENT"}},
};
}

bool IsFocusedEvent(const std::string& eventDomain, const std::string& eventName)
{
    auto iter = FOCUSED_EVENT_MAP.find(eventDomain);
    if (iter == FOCUSED_EVENT_MAP.end()) {
        return false;
    }
    auto findRet = std::find(iter->second.begin(), iter->second.end(), eventName);
    return findRet != iter->second.end();
}
} // namespace FocusedEventUtil
} // namespace HiviewDFX
} // namespace OHOS