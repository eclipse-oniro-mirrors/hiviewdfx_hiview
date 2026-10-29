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
 
#ifndef ACE_INTERACTION_EVENT_H
#define ACE_INTERACTION_EVENT_H
 
#include "xperf_event.h"
 
namespace OHOS {
namespace HiviewDFX {
 
struct AceInteractionCompletedLatencyEvent : public OhosXperfEvent {
    int32_t uniqueId{0};
    int32_t appPid{0};
    std::string bundleName;
    std::string processName;
    std::string abilityName;
    std::string pageUrl;
    int32_t versionCode{0};
    std::string versionName;
    std::string pageName;
    std::string sceneId;
    std::string sourceType;
    int64_t inputTime{0};
    int64_t animationStartLatency{0};
    int64_t animationEndLatency{0};
    int64_t e2eLatency{0};
    std::string note;
    std::string subhealthInfo;
    std::string subhealthReason;
    int32_t subhealthTime{0};
};
 
struct AceInteractionAppJankEvent : public OhosXperfEvent {
    int32_t uniqueId{0};
    std::string sceneId;
    std::string processName;
    std::string moduleName;
    std::string abilityName;
    std::string pageUrl;
    int32_t versionCode{0};
    std::string versionName;
    std::string pageName;
    int64_t starttime{0};
    int64_t duration{0};
    int32_t totalFrames{0};
    int32_t totalMissedFrames{0};
    int64_t maxFrametime{0};
    int64_t maxFrametimeSinceStart{0};
    int64_t maxHitchTime{0};
    int64_t maxHitchTimeSinceStart{0};
    int32_t maxSeqMissedFrames{0};
    std::string note;
    bool displayAnimator{false};
    std::string subhealthInfo;
    std::string subhealthReason;
    int32_t subhealthTime{0};
    std::string jankCount;
    std::string actionType;
    int32_t pos{0};
};
 
} // namespace HiviewDFX
} // namespace OHOS
 
#endif