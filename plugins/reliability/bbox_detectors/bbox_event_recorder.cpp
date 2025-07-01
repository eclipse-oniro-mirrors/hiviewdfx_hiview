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

#include "bbox_event_recorder.h"

namespace OHOS {
namespace HiviewDFX {
bool BboxEventRecorder::AddEventToMaps(const std::string& event, uint64_t happentTime, const std::string& logPath)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto [it, success] = maps_.insert({event + std::to_string(happentTime), logPath});
    return success;
}

bool BboxEventRecorder::IsExistEvent(const std::string& event, uint64_t happentTime, const std::string& logPath) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = maps_.find(event + std::to_string(happentTime));
    if (it == maps_.end() || it->second != logPath) {
        return false;
    }
    return true;
}
} // HiviewDFX
} // OHOS
