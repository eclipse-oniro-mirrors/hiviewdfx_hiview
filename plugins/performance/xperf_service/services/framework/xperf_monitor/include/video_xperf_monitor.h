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

#include <mutex>
#include <map>
#include <list>
#include "xperf_monitor.h"
#include "video_record.h"
#include "rs_event.h"
#include "network_event.h"
#include "avcodec_event.h"
#include "video_jank_record.h"
#include "surface_info.h"

namespace OHOS {
namespace HiviewDFX {

class VideoXperfMonitor : public XperfMonitor {
public:
    static VideoXperfMonitor& GetInstance();
    VideoXperfMonitor(const VideoXperfMonitor&) = delete;
    void operator=(const VideoXperfMonitor&) = delete;

    void OnSurfaceReceived(int32_t pid, const std::string& bundleName, int64_t uniqueId,
        const std::string& surfaceName);

    void ProcessEvent(OhosXperfEvent* event) override;

private:
    VideoXperfMonitor() = default;
    ~VideoXperfMonitor() = default;

private:
    mutable std::mutex mMutex;
    std::map<int64_t, VideoJankRecord> videoJankRecordMap;
    std::list<SurfaceInfo> surfaceInfoList;

    void BroadcastVideoJank(const std::string& msg);
    void OnVideoJankReceived(OhosXperfEvent* event);
    void OnNetworkJankReceived(OhosXperfEvent* event);
    void OnAvcodecJankReceived(OhosXperfEvent* event);
    void WaitForDomainReport(int64_t uniqueId);
    void FaultJudgment(int64_t uniqueId);
    SurfaceInfo GetSurfaceInfo(int64_t uniqueId);
};
} // namespace HiviewDFX
} // namespace OHOS

#endif