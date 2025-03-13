/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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
#include "sys_usage_event_factory.h"

#include <cinttypes>

#include "hiview_logger.h"
#include "sys_usage_event.h"
#include "time_util.h"
#include "usage_event_common.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("SysUsageEventFactory");
namespace {
constexpr uint64_t TIME_ERROR = 10;
}
using namespace SysUsageEventSpace;

std::unique_ptr<LoggerEvent> SysUsageEventFactory::Create()
{
    std::unique_ptr<LoggerEvent> event =
        std::make_unique<SysUsageEvent>(EVENT_NAME, HiSysEvent::STATISTIC);
    event->Update(KEY_OF_START, DEFAULT_UINT64);
    event->Update(KEY_OF_END, TimeUtil::GetMilliseconds());

    // correct the time error caused by interface call
    auto monotonicTime = TimeUtil::GetMonotonicTimeMs();
    auto bootTime = TimeUtil::GetBootTimeMs();
    if (bootTime < (monotonicTime + TIME_ERROR)) {
        bootTime = monotonicTime;
    }
    HIVIEW_LOGI("get monotonicTime=%{public}" PRIu64 ", bootTime=%{public}" PRIu64, monotonicTime, bootTime);
    event->Update(KEY_OF_POWER, bootTime);
    event->Update(KEY_OF_RUNNING, monotonicTime);
    return event;
}
} // namespace HiviewDFX
} // namespace OHOS
