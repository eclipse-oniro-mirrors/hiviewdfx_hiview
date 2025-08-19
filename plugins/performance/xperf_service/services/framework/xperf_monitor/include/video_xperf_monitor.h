/*
 * Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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

#ifndef VIDEO_XPERF_MONITOR_H
#define VIDEO_XPERF_MONITOR_H

#include "xperf_monitor.h"
#include "video_record.h"
#include "rs_event.h"
#include "network_event.h"
#include "avcodec_event.h"
#include "video_jank_record.h"
#include <mutex>
#include <map>

namespace OHOS {
namespace HiviewDFX {

class VideoXperfMonitor : public XperfMonitor {
public:
    VideoXperfMonitor(VideoRecord* record);
    ~VideoXperfMonitor() override;
    bool ProcessEvent(OhosXperfEvent* event) override;

private:
    mutable std::mutex mMutex;
    VideoRecord* record{nullptr};
    RsJankEvent rsVideoJankEvent;
    NetworkJankEvent networkXperfEvent;
    AvcodecJankEvent avcodecVideoJankEvent;
    std::map<int64_t, VideoJankRecord> videoJankRecordMap;

    void BroadcastVideoJank(const std::string& msg);
    void OnVideoJankReceived(OhosXperfEvent* event);
    void OnNetworkJankReceived(OhosXperfEvent* event);
    void OnAvcodecJankReceived(OhosXperfEvent* event);
    void WaitForDomainReport(int64_t uniqueId);
    void FaultJudgment(int64_t uniqueId);
};
} // namespace HiviewDFX
} // namespace OHOS

#endif