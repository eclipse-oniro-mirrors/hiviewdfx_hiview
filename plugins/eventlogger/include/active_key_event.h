/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifndef HIVIEW_EVENT_LOG_ACTIVE_KEY_EVENT_H
#define HIVIEW_EVENT_LOG_ACTIVE_KEY_EVENT_H
#ifdef MULTIMODALINPUT_INPUT_ENABLE
#include <memory>
#include <string>
#include <list>

#include "input_manager.h"
#include "ffrt.h"

namespace OHOS {
namespace HiviewDFX {
class ActiveKeyEvent {
public:
    ActiveKeyEvent();
    ~ActiveKeyEvent();
    ActiveKeyEvent& operator=(const ActiveKeyEvent&) = delete;
    ActiveKeyEvent(const ActiveKeyEvent&) = delete;
    void Init();
    static int64_t SystemTimeMillisecond();

private:
    void InitSubscribe(std::set<int32_t> preKeys, int32_t finalKey, int32_t count, int32_t holdTime);
    void CombinationKeyHandle(std::shared_ptr<MMI::KeyEvent> keyEvent);
    void CombinationKeyCallback(std::shared_ptr<MMI::KeyEvent> keyEvent);

#ifdef HITRACE_CATCHER_ENABLE
    void HitraceCapture();
#endif // HITRACE_CATCHER_ENABLE
    void SysMemCapture(int fd);
    void DumpCapture(int fd);

    mutable ffrt::mutex mutex_;
    std::list<int32_t> subscribeIds_;
    uint64_t triggeringTime_;
    uint8_t taskOutDeps = 0;
    int reportLimit_ = 0;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // MULTIMODALINPUT_INPUT_ENABLE
#endif // HIVIEW_EVENT_LOG_ACTIVE_KEY_EVENT_H
