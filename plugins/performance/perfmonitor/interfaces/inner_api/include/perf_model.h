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

#ifndef XPERF_MODEL_H
#define XPERF_MODEL_H

#include <map>
#include <string>
#include "perf_constants.h"
#include "iremote_broker.h"

namespace OHOS {
namespace HiviewDFX {
enum PerfActionType {
    UNKNOWN_ACTION = -1,
    LAST_DOWN = 0,
    LAST_UP = 1,
    FIRST_MOVE = 2
};

enum PerfSourceType {
    UNKNOWN_SOURCE = -1,
    PERF_TOUCH_EVENT = 0,
    PERF_MOUSE_EVENT = 1,
    PERF_TOUCH_PAD = 2,
    PERF_JOY_STICK = 3,
    PERF_KEY_EVENT = 4
};

enum PerfEventType {
    UNKNOWN_EVENT = -1,
    EVENT_RESPONSE = 0,
    EVENT_COMPLETE = 1,
    EVENT_JANK_FRAME = 2
};

struct AceAppInfo {
    int32_t pid {-1};
    int32_t versionCode {0};
    std::string versionName {""};
    std::string bundleName {""};
    std::string processName {""};
    std::string abilityName {""};
};

struct SubHealthInfo {
    std::string info {""};
    std::string subHealthReason {""};
    int32_t subHealthTime {0};
};

struct BaseInfo {
    int32_t pid {-1};
    int32_t versionCode {0};
    std::string versionName {""};
    std::string bundleName {""};
    std::string processName {""};
    std::string abilityName {""};
    std::string pageUrl {""};
    std::string pageName {""};
    std::string note {""};
    SubHealthInfo subHealthInfo;
};

struct DataBase {
    std::string sceneId {""};
    int32_t maxSuccessiveFrames {0};
    int32_t totalMissed {0};
    int32_t totalFrames {0};
    int64_t inputTime {0};
    int64_t beginVsyncTime {0};
    int64_t endVsyncTime {0};
    int64_t maxFrameTime {0};
    int64_t maxFrameTimeSinceStart {0};
    int64_t maxHitchTime {0};
    int64_t maxHitchTimeSinceStart {0};
    bool needReportRs {false};
    bool isDisplayAnimator {false};
    PerfSourceType sourceType {UNKNOWN_SOURCE};
    PerfActionType actionType {UNKNOWN_ACTION};
    PerfEventType eventType {UNKNOWN_EVENT};
    BaseInfo baseInfo;
};

struct JankInfo {
    int64_t skippedFrameTime {0};
    std::string windowName {""};
    std::string sceneId {""};
    uint64_t sceneTag {0};
    int64_t realSkippedFrameTime {0};
    BaseInfo baseInfo;
};

struct ImageLoadInfo {
    int64_t id {0}; //无障碍id
    std::string imageType;
    int width {0};
    int height {0};
    uint64_t loadStartTime{0};
    uint64_t loadEndTime{0};
    int loadState {0}; //图片load状态 0-失败/1-成功
};
 
struct ImageLoadStat {
    uint64_t startTime{0};
    uint64_t endTime{0};
    int totalNum {0};
    int failedNum {0};
    int64_t totalSize {0};
    int64_t failedSize {0};
    std::string typeDetails;
};

class AnimatorRecord {
public:
    void InitRecord(const std::string& sId, PerfActionType aType, PerfSourceType sType, const std::string& nt,
        int64_t time);
    void RecordFrame(int64_t vsyncTime, int64_t duration, int32_t skippedFrames);
    void Report(const std::string& sceneId, int64_t vsyncTime, bool isRsRender);
    bool IsTimeOut(int64_t nowTime);
    bool IsFirstFrame();
    bool IsDisplayAnimator(const std::string& sceneId);
    void Reset();
public:
    int64_t inputTime {0};
    int64_t beginVsyncTime {0};
    int64_t endVsyncTime {0};
    int64_t maxFrameTime {0};
    int64_t maxFrameTimeSinceStart {0};
    int64_t maxHitchTime {0};
    int64_t maxHitchTimeSinceStart {0};
    int32_t maxSuccessiveFrames {0};
    int32_t totalMissed {0};
    int32_t totalFrames {0};
    int32_t seqMissFrames {0};
    bool isSuccessive {false};
    bool isFirstFrame {false};
    bool needReportRs {false};
    bool isDisplayAnimator {false};
    std::string sceneId {""};
    PerfActionType actionType {UNKNOWN_ACTION};
    PerfSourceType sourceType {UNKNOWN_SOURCE};
    std::string note {""};
};

enum SceneType {
    APP_FOREGROUND = 0,
    NON_EXPERIENCE_ANIMATOR,
    NON_EXPERIENCE_WINDOW,
    APP_START,
    APP_RESPONSE,
    APP_BACKGROUND,
    PAGE_LOADING,
    POWER_OFF,
    ACCESSIBLE_FEATURE,
    VIDEO_PLAYING
};

class SceneRecord {
public:
    virtual void StartRecord(const SceneType& sType);
    virtual void StopRecord(const SceneType& sType);
    virtual void StartRecord(const SceneType& sType, const std::string& sId);
    virtual void StopRecord(const SceneType& sType, const std::string& sId);
public:
    SceneType type{APP_FOREGROUND};
    std::string sceneId;
    bool status{false};
    int64_t startTime{0};
    int64_t duration{0};
};

class NonExperienceAnimator : public SceneRecord {
public:
    void StartRecord(const SceneType& sType, const std::string& sId) override;
    void StopRecord(const SceneType& sType, const std::string& sId) override;
public:
    bool IsNonExperienceWhiteList(const std::string& sceneId);
};

class NonExperienceWindow : public NonExperienceAnimator {
public:
    void StartRecord(const SceneType& sType, const std::string& sId);
    void StopRecord(const SceneType& sType, const std::string& sId);
    bool IsNonExperienceWhiteList(const std::string& windowName);
};

class NonExperienceAppStart : public SceneRecord {
public:
    void StartRecord(const SceneType& sType) override;
    void StopRecord(const SceneType& sType) override;
    bool IsInStartAppStatus();
};

class NonExperiencePageLoading : public SceneRecord {
public:
    void StartRecord(const SceneType& sType, const std::string& sId) override;
    void StopRecord(const SceneType& sType) override;
    bool IsNonExperienceWhiteList(const std::string& sceneId);
};

class NonExperienceResponse : public SceneRecord {
public:
    void StartRecord(const SceneType& sType, const std::string& sId) override;
    bool IsNonExperienceWhiteList(const std::string& sceneId);
};

class IFrameCallback : public IRemoteBroker {
public:
    virtual void OnVsyncEvent(int64_t vsyncTime, int64_t duration, double jank, const std::string& windowName) = 0;
    virtual sptr<IRemoteObject> AsObject() { return nullptr;}
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.HiviewDFX.IFrameCallback");
};

class IAnimatorCallback : public IRemoteBroker {
public:
    virtual void OnAnimatorStart(const std::string& sceneId, PerfActionType type, const std::string& note) = 0;
    virtual void OnAnimatorStop(const std::string& sceneId, bool isRsRender) = 0;
    virtual sptr<IRemoteObject> AsObject() { return nullptr;}
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.HiviewDFX.IAnimatorCallback");
};

class ISceneCallback : public IRemoteBroker {
public:
    virtual void OnSceneEvent(const SceneType& type, const bool status) = 0;
    virtual void OnSceneEvent(const SceneType& type, const bool status, const std::string& sceneId) = 0;
    virtual sptr<IRemoteObject> AsObject() { return nullptr; }
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.HiviewDFX.ISceneCallback");
};
} // namespace OHOS
} // namespace HiviewDFX
#endif // XPERF_MODEL_H
