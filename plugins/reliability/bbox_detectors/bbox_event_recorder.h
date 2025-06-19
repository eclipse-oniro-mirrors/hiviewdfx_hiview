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

#ifndef BBOX_EVENT_RECORDER_H
#define BBOX_EVENT_RECORDER_H

#include <map>
#include <mutex>
#include <string>

namespace OHOS {
namespace HiviewDFX {
class BboxEventRecorder {
public:
    bool AddEventToMaps(const std::string& event, uint64_t happentTime, const std::string& logPath);
    bool IsExistEvent(const std::string& event, uint64_t happentTime, const std::string& logPath) const;

private:
    mutable std::mutex mutex_;
    std::map<std::string, std::string> maps_;
};
} // HiviewDFX
} // OHOS
#endif // BBOX_EVENT_RECORDER_H
