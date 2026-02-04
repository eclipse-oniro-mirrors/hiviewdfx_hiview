/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "ffrt_util.h"

#include "ffrt.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace FfrtUtil {
namespace {
constexpr uint32_t CHECK_CYCLE_MILLISECONDS = 60 * 1000; // 60 seconds
}

void Sleep(uint32_t taskCycleSec)
{
    uint64_t taskCycleMs = static_cast<uint64_t>(taskCycleSec * TimeUtil::SEC_TO_MILLISEC);
    uint64_t currentTimeMs = TimeUtil::GetBootTimeMs();
    uint64_t finishTimeMs = currentTimeMs + taskCycleMs;
    while (currentTimeMs < finishTimeMs) {
        // if remaining time less than 60 seconds, only need to wait for remaining time
        uint64_t remainingTimeMs = finishTimeMs - currentTimeMs;
        ffrt::this_task::sleep_for(std::chrono::milliseconds(
            remainingTimeMs > CHECK_CYCLE_MILLISECONDS ? CHECK_CYCLE_MILLISECONDS : remainingTimeMs));
        currentTimeMs = TimeUtil::GetBootTimeMs();
    }
}
}
}
}