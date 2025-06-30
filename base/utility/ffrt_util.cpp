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

#include "ffrt_util.h"

#include "ffrt.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace FfrtUtil {
namespace {
constexpr int64_t CHECK_CYCLE_SECONDS = 60; // 60 seconds

inline int64_t GetCurrentSecond()
{
    return static_cast<int64_t>(TimeUtil::GetSteadyClockTimeMs()) / TimeUtil::SEC_TO_MILLISEC;
}
}


void Sleep(int64_t taskCycleSec)
{
    int64_t lastTimeSec = GetCurrentSecond();
    while (std::abs(GetCurrentSecond() - lastTimeSec) < taskCycleSec) {
        // avoid too long time to sleep, check whether the time difference
        // is exceed the task cycle is neccessary after each 60 seconds' sleep
        ffrt::this_task::sleep_for(std::chrono::seconds(CHECK_CYCLE_SECONDS));
    }
}
}
}
}