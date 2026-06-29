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
 
#ifndef PASSTHROUGH_MONITOR_H
#define PASSTHROUGH_MONITOR_H

#include <list>
#include <mutex>

#include "xperf_constant.h"
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

    void OnSurfaceReceived(const std::string& bundleName, int64_t uniqueId);
 
private:
    void OnLoadCompleteEvent(OhosXperfEvent* event);
    void OnVideoFirstFrame(OhosXperfEvent* event);
    void OnVideoSecondFrame(OhosXperfEvent* event);
    void OnAppForeground(OhosXperfEvent* event);
    void OnTouchAction(OhosXperfEvent* event);
    void OnAudioStart(OhosXperfEvent* event);
    std::string GetBundleName(int64_t uniqueId);
 
    PassthroughMonitor() = default;
    virtual ~PassthroughMonitor() = default;

    int64_t lastReportSurfaceTime_{0};

    std::mutex sourceMutex_;
    std::list<std::pair<std::string, int64_t>> lruList_;

    //----点击操作----
    std::mutex touchActionMutex_;
    std::string bundleName_;
    int64_t appForegroundTime_ = 0;
};
 
} // namespace HiviewDFX
} // namespace OHOS
 
#endif