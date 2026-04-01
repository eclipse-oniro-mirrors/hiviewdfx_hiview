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
 
#ifndef PASSTHROUGH_MONITOR_H
#define PASSTHROUGH_MONITOR_H
 
#include "xperf_monitor.h"
 
namespace OHOS {
namespace HiviewDFX {
 
/**
 * @brief 透传事件的监控器
 * 
 * 监控需要透传的事件，按eventName进行上报
 */
class PassthroughMonitor : public XperfMonitor {
public:
    static PassthroughMonitor& GetInstance();
    PassthroughMonitor(const PassthroughMonitor&) = delete;
    void operator=(const PassthroughMonitor&) = delete;
 
    void ProcessEvent(OhosXperfEvent* event) override;
 
private:
    void ProcessLoadCompleteEvent(OhosXperfEvent* event);
 
    PassthroughMonitor() = default;
    virtual ~PassthroughMonitor() = default;
};
 
} // namespace HiviewDFX
} // namespace OHOS
 
#endif