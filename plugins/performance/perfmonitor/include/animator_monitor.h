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
#include "perf_constants.h"
#include "perf_model.h"

namespace OHOS {
namespace HiviewDFX {

class AnimatorMonitor {
public:
    static AnimatorMonitor& GetInstance();
    void Start(const std::string& sceneId, PerfActionType type, const std::string& note);
    void End(const std::string& sceneId, bool isRsRender);
    void StartCommercial(const std::string& sceneId, PerfActionType type, const std::string& note);
    void EndCommercial(const std::string& sceneId, bool isRsRender);
    void SetFrameTime(int64_t vsyncTime, int64_t duration, double jank, const std::string& windowName);

    bool RecordsIsEmpty();
private:
    SceneRecord* GetRecord(const std::string& sceneId);
    void RemoveRecord(const std::string& sceneId);

private:
    mutable std::mutex mMutex;
    std::map<std::string, SceneRecord*> mRecords;
};

}
}

#endif // ANIMATOR_MONITOR_H