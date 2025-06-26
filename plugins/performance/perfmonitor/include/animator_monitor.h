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

#ifndef ANIMATOR_MONITOR_H
#define ANIMATOR_MONITOR_H

#include <map>
#include <mutex>
#include "animator_monitor.h"
#include "perf_constants.h"
#include "perf_model.h"
#include "scene_monitor.h"

namespace OHOS {
namespace HiviewDFX {

class AnimatorMonitor : public IAnimatorCallback, public IFrameCallback {
public:
    static AnimatorMonitor& GetInstance();
    AnimatorMonitor();
    ~AnimatorMonitor();
    // outer interface for animator
    void RegisterAnimatorCallback(IAnimatorCallback* cb);
    void UnregisterAnimatorCallback(IAnimatorCallback* cb);
    void Start(const std::string& sceneId, PerfActionType type, const std::string& note);
    void End(const std::string& sceneId, bool isRsRender);
    void SetSubHealthInfo(const SubHealthInfo& info);
    bool IsSubHealthScene();

    // inner interface for animator
    void OnAnimatorStart(const std::string& sceneId, PerfActionType type, const std::string& note) override;
    void OnAnimatorStop(const std::string& sceneId, bool isRsRender) override;
    void OnVsyncEvent(int64_t vsyncTime, int64_t duration, double jank, const std::string& windowName) override;
    bool RecordsIsEmpty();

private:
    void FlushDataBase(AnimatorRecord* record, DataBase& data);
    void ReportAnimateStart(const std::string& sceneId, AnimatorRecord* record);
    void ReportAnimateEnd(const std::string& sceneId, AnimatorRecord* record);
    AnimatorRecord* GetRecord(const std::string& sceneId);
    void RemoveRecord(const std::string& sceneId);

    mutable std::mutex mMutex;
    int64_t subHealthRecordTime = 0;
    std::vector<IAnimatorCallback*> animatorCallbacks;
    std::map<std::string, AnimatorRecord*> mRecords;
};

}
}

#endif // ANIMATOR_MONITOR_H