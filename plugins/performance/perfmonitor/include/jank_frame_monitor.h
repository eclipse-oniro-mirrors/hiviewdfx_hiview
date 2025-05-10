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

#ifndef JANK_FRAME_MONITOR_H
#define JANK_FRAME_MONITOR_H

#include <map>
#include <mutex>
#include "perf_constants.h"
#include "perf_model.h"

namespace OHOS {
namespace HiviewDFX {

class JankFrameMonitor {
public:
    static JankFrameMonitor& GetInstance();
    void ProcessJank(double jank, const std::string& windowName);
    bool IsExceptResponseTime(int64_t time, const std::string& sceneId);
    int32_t GetFilterType() const;
    void ClearJankFrameRecord();
    void JankFrameStatsRecord(double jank);
    bool IsExclusionFrame();
    void SetVsyncLazyMode();
    void CheckInStartAppStatus();
    void CheckExclusionWindow(const std::string& windowName);
    void CheckResponseStatus();

    void SetJankFrameRecordBeginTime(int64_t val);
    int64_t GetJankFrameRecordBeginTime();
    int32_t GetJankFrameTotalCount();
    const std::vector<uint16_t>& GetJankFrameRecord();
    bool JankFrameRecordIsEmpty();
    void InitJankFrameRecord();

    void SetIsBackgroundApp(bool val);
    bool GetIsBackgroundApp();
    void SetIsStartAppFrame(bool val);
    void SetStartAppTime(int64_t val);
    void SetIsExceptAnimator(bool val);
    void SetIsResponseExclusion(bool val);

private:
    uint32_t GetJankLimit(double jank);
private:
    bool isResponseExclusion {false};
    bool isStartAppFrame {false};
    bool isBackgroundApp {false};
    bool isExclusionWindow {false};
    bool isExceptAnimator {false};
    int64_t startAppTime {0};
    std::vector<uint16_t> jankFrameRecord;
    int64_t jankFrameRecordBeginTime;
    int32_t jankFrameTotalCount {0};
};

}
}

#endif // JANK_FRAME_MONITOR_H