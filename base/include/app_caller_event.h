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

#ifndef HIVIEW_BASE_APP_CALLER_EVENT_H
#define HIVIEW_BASE_APP_CALLER_EVENT_H

#include <string>
#include "event.h"
namespace OHOS {
namespace HiviewDFX {
class AppCallerEvent : public Event {
public:
    explicit AppCallerEvent(const std::string &sender) : Event(sender), uid_(0), pid_(0), beginTime_(0), endTime_(0),
        resultCode_(0), taskBeginTime_(0), taskEndTime_(0), foreground_(0), isBusinessJank_(false) {}

    ~AppCallerEvent() = default;

public:
    std::string bundleName_;    // app bundle name
    std::string bundleVersion_; // app bundle version
    int32_t uid_;               // app user id
    int32_t pid_;               // app process id
    int64_t beginTime_;         // message handle begin time, millisecond unit
    int64_t endTime_;           // message handle end time, millisecond unit
    int32_t resultCode_;        // handle event error code
    int64_t taskBeginTime_;     // task start time
    int64_t taskEndTime_;       // task finish time
    int32_t foreground_;        // app foreground
    bool isBusinessJank_;        // is business jank or not, control output file name
    std::string threadName_;    // app thread name
    std::string externalLog_;   // trace file
};
} // HiviewDFX
} // OHOS
#endif // HIVIEW_BASE_APP_CALLER_EVENT_H