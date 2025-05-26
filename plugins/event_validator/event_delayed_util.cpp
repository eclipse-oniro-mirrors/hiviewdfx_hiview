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
#include "event_delayed_util.h"

#include "running_status_logger.h"

namespace OHOS {
namespace HiviewDFX {
void EventDelayedUtil::CheckIfEventDelayed(std::shared_ptr<SysEvent> event)
{
    uint64_t happenTime = event->happenTime_;
    uint64_t createTime = event->createTime_ / 1000; // 1000: us->ms
    if (createTime < happenTime) { // for the time jump scene
        LogEventDelayedInfo(false, happenTime, createTime);
        return;
    }

    // event delay exceeds threshold
    constexpr uint64_t delayDetectLimit = 5 * 1000; // 5s
    if (uint64_t delayTime = createTime - happenTime; delayTime > delayDetectLimit) {
        LogEventDelayedInfo(true, happenTime, createTime);
        return;
    }

    // event not delayed
    LogEventDelayedInfo(false, happenTime, createTime);
}

void EventDelayedUtil::LogEventDelayedInfo(bool isCurEventDelayed, uint64_t happenTime, uint64_t createTime)
{
    if (isLastEventDelayed_ == isCurEventDelayed) {
        return;
    }

    // reset delay info of the last event by delay info of current event
    isLastEventDelayed_ = isCurEventDelayed;
    std::string info { "event delay " };
    if (isCurEventDelayed) {
        info.append("start;");
    } else {
        info.append("end;");
    }
    info.append("happen_time=[").append(std::to_string(happenTime)).append("]; ");
    info.append("create_time=[").append(std::to_string(createTime)).append("]");
    RunningStatusLogger::GetInstance().LogEventRunningLogInfo(info);
}
} // namespace HiviewDFX
} // namespace OHOS
