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

class JankFrameMonitor : public IFrameCallback {
public:
    static JankFrameMonitor& GetInstance();
    JankFrameMonitor();
    ~JankFrameMonitor();

    // outer interface for app frame observer
    void RegisterFrameCallback(IFrameCallback* cb);
    void UnregisterFrameCallback(IFrameCallback* cb);
    void OnFrameEnd(int64_t vsyncTime, int64_t duration, double jank, const std::string& windowName);
    void OnVsyncEvent(int64_t vsyncTime, int64_t duration, double jank, const std::string& windowName);

    // inner interface for app frame observer
    void ProcessJank(int64_t vsyncTime, double jank, const std::string& windowName);
    void JankFrameStatsRecord(double jank);

    // stats app jank frame
    void InitJankFrameRecord();
    void ClearJankFrameRecord();
    void SetJankFrameRecordBeginTime(int64_t val);
    int64_t GetJankFrameRecordBeginTime();
    int32_t GetJankFrameTotalCount();
    const std::vector<uint16_t>& GetJankFrameRecord();
    bool JankFrameRecordIsEmpty();

private:
    uint32_t GetJankLimit(double jank);
private:
    mutable std::mutex mMutex;
    std::vector<IFrameCallback*> frameCallbacks;
    std::vector<uint16_t> jankFrameRecord;
    int64_t jankFrameRecordBeginTime;
    int32_t jankFrameTotalCount {0};
};

}
}

#endif // JANK_FRAME_MONITOR_H