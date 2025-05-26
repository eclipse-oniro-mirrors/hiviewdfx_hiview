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

#ifndef HIVIEW_PLUGINS_EVENT_VERIFY_UTIL_H
#define HIVIEW_PLUGINS_EVENT_VERIFY_UTIL_H

#include <list>

#include "event_json_parser.h"
#include "event_param_watcher.h"
#include "event_period_info_util.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
class EventVerifyUtil {
public:
    void Init(HiviewContext* context);
    bool IsValidEvent(std::shared_ptr<SysEvent> event);

private:
    bool IsValidSysEvent(const std::shared_ptr<SysEvent> event);
    bool IsDuplicateEvent(const uint64_t eventId);
    void DecorateSysEvent(const std::shared_ptr<SysEvent> event, const BaseInfo& baseInfo, uint64_t id);

private:
    std::list<uint64_t> eventIdList_;
    EventPeriodInfoUtil eventPeriodInfoUtil_;
    EventParamWatcher paramWatcher_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGINS_EVENT_VERIFY_UTIL_H
