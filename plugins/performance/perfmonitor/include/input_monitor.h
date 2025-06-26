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

#ifndef INPUT_MONITOR_H
#define INPUT_MONITOR_H

#include <map>
#include <mutex>
#include "perf_constants.h"
#include "perf_model.h"

namespace OHOS {
namespace HiviewDFX {

class InputMonitor {
public:
    static InputMonitor& GetInstance();
    void RecordInputEvent(PerfActionType type, PerfSourceType sourceType, int64_t time);
    int64_t GetInputTime(const std::string& sceneId, PerfActionType type, const std::string& note);
    PerfSourceType GetSourceType();
    int64_t GetVsyncTime();
    void SetVsyncTime(int64_t val);
private:
    mutable std::mutex mMutex;
    std::map<PerfActionType, int64_t> mInputTime;
    int64_t mVsyncTime {0};
    PerfSourceType mSourceType {UNKNOWN_SOURCE};
};

}
}

#endif // INPUT_MONITOR_H