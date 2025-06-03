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

#include "perf_utils.h"

namespace OHOS {
namespace HiviewDFX {

int64_t GetCurrentRealTimeNs()
{
    struct timespec ts = { 0, 0 };
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (ts.tv_sec * NS_TO_S + ts.tv_nsec);
}

int64_t GetCurrentSystimeMs()
{
    auto timeNow = std::chrono::system_clock::now();
    auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow.time_since_epoch());
    int64_t curSystime = tmp.count();
    return curSystime;
}

void ConvertRealtimeToSystime(int64_t realTime, int64_t& sysTime)
{
    int64_t curRealTime = GetCurrentRealTimeNs();
    if (curRealTime == 0) {
        sysTime = 0;
        return;
    }
    int64_t curSysTime = GetCurrentSystimeMs();
    sysTime = curSysTime - (curRealTime - realTime) / NS_TO_MS;
}

std::string GetSourceTypeName(PerfSourceType sourceType)
{
    std::string type = "";
    switch (sourceType) {
        case PERF_TOUCH_EVENT:
            type = "TOUCHSCREEN";
            break;
        case PERF_MOUSE_EVENT:
            type = "MOUSE";
            break;
        case PERF_TOUCH_PAD:
            type = "TOUCHPAD";
            break;
        case PERF_JOY_STICK:
            type = "JOYSTICK";
            break;
        case PERF_KEY_EVENT:
            type = "KEY_EVENT";
            break;
        default :
            type = "UNKNOWN_SOURCE";
            break;
    }
    return type;
}

}
}