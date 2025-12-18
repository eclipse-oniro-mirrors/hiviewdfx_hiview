/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef FAULTLOG_BOOTSCAN_LISTENER_H
#define FAULTLOG_BOOTSCAN_LISTENER_H

#include "event.h"
#include "event_loop.h"

namespace OHOS {
namespace HiviewDFX {
class FaultLogBootScanListener : public EventListener {
public:
    explicit FaultLogBootScanListener(std::shared_ptr<EventLoop> workLoop);
    ~FaultLogBootScanListener() override = default;
    void OnUnorderedEvent(const Event& msg) override;
    std::string GetListenerName() override;
private:
    void AddBootScanEvent();

    std::shared_ptr<EventLoop> workLoop_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // FAULTLOG_BOOTSCAN_LISTENER_H
