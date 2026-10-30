/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
 * Author: huangyu
 * Create: 2023-11-16
 */

#ifndef DFR_FAULT_INFO_BASE_H
#define DFR_FAULT_INFO_BASE_H

#include <vector>
#include <atomic>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include "fault_common.h"
#include "dfr_util.h"
#include "suspend_manager_client.h"
#include "bundle_mgr_client.h"
#include "ffrt.h"
#include "process_anomaly_info.h"
#include "resource_quota_policy_info.h"
#include <atomic>

namespace OHOS {
namespace HiviewDFX {

enum DetectorType {
    NONE_LEAK_DETECTOR = 0,
    FD_LEAK_DETECTOR,
    THREAD_LEAK_DETECTOR,
    NOTIFY_LEAK_DETECTOR,
    KERNEL_LEAK_DETECTOR,
    NATIVE_LEAK_DETECTOR,
    RSS_LEAK_DETECTOR,
    DETECTOR_SIZE,
};

enum ReportEventType {
    MEMORY_LEAK_EVENT = 0,
    MEMORY_LEAK_WP_EVENT,
};

enum QuotaAction {
    NO_ACTION = 0,
    ACTION_WARNING = 1,
    ACTION_SOFT_KILL = 1 << 1,
    ACTION_KILL = 1 << 2,
    ACTION_CLEAN = 1 << 3,
    ACTION_MAX = 1 << 4,
};

enum QuotaCondition {
    CONDITION_UNKNOWN = 0,
    CONDITION_FOREGROUND = 1,
    CONDITION_BACKGROUND = 1 << 1,
    CONDITION_STANDBY = 1 << 6,
    CONDITION_MAX = 1 << 7,
};

struct Quota {
    int value { INT_MAX };
    QuotaAction action { NO_ACTION };
    uint32_t conditions { CONDITION_MAX };
    std::string preCleanProcess;
};

enum ProcessSourceType {
    COMMON_DETECT = 0,
    SYSLOAD_DETECT,
    XPERF_EVENT_DETECT,
    MEMMGR_EVENT_DETECT,
    STANDBY_DETECT,
    SERVICE_LEAK_DETECT,
};

// use for process which not periodic detect
struct ProcessSourceInfo {
    ProcessSourceType type { COMMON_DETECT };
    std::string detectMsg;
    std::string GetLeakReason()
    {
        if (type == XPERF_EVENT_DETECT) {
            return detectMsg;
        }
        return "";
    }
};

constexpr int SEC_PER_MIN = 60;
constexpr int MIN_PER_HOUR = 60;
constexpr int HOUR_PER_DAY = 24;
constexpr int SEC_PER_HOUR = SEC_PER_MIN * MIN_PER_HOUR;
constexpr int SEC_PER_DAY = HOUR_PER_DAY * SEC_PER_HOUR;

struct DumperInfo {
    DetectorType detectorType { NONE_LEAK_DETECTOR };
    CommonDumpType dumpType { NO_ACTION_DUMP };
    std::string dumpFunctions;
    pid_t pid { -1 };
    std::string processName;
    // CommonDumpType: dumpType, string: path
    CommonDumpPathList pathMap;
};

constexpr int UNSPECIFIED_PROCESS_TYPE = 255;

class FaultInfoBase {
public:
    FaultInfoBase(pid_t pid, std::string name);
    virtual ~FaultInfoBase() {};

    FaultStateType GetState() const;
    void SetState(FaultStateType state);
    std::string GetProcessName();
    void SetPid(pid_t pid);
    pid_t GetPid() const;
    void SetLeakReason(const std::string &reason);
    const std::string& GetLeakReason() const;
    bool IsDumping() const;
    void SetIsDumping(bool flag = true);
    std::string GetLeakedTime() const;
    void SetLeakedTime();
    LeakGrade GetLeakGrade() const;
    void SetLeakGrade(LeakGrade leakGrade);
    int32_t GetStackDebugTime() const;
    time_t GetPidStartTime() const;
    void SetProcessDied(bool flag);
    bool IsDied();
    void SetDumpLogType(DumpLogType type);
    DumpLogType GetDumpLogType() const;
    bool CheckIfProcessAlive() const;
    void SetUid(pid_t pid);
    void SetUidDirect(pid_t uid);
    bool IsApp();
    pid_t GetUid();
    std::string GetBundleName();
    void UnfreezingProcess();
    int32_t SendCmd(TrackCmd &trackCmd);
    std::string GetHapVersion();
    std::string GetHapVersionCode();
    void SetTelemetryFlag(bool flag = true);
    bool GetTelemetryFlag();
    bool IsEverSendTelemetryEvent();
    void SetEverSendTelemetryEvent(bool flag = true);
    void SetAppRunningId(const std::string &id);
    virtual std::string GetAppRunningId();

protected:
    void SetStartTime();
    void EnsureBundleInfoLoaded();
    virtual std::string GetProfilerPath() const = 0;
    virtual std::string GetProfilerName() const = 0;

public:
    ffrt::condition_variable dumpCond_;
    ffrt::mutex profileCatMtx_;

protected:
    pid_t pid_;
    std::string processName_;

    FaultStateType state_ { PROC_IDLE_STATE };
    std::atomic<LeakGrade> leakGrade_ { NO_LEAK };
    DumpLogType dumpLogType_ { NO_LOG };
    std::string leakedTime_ { "" };
    std::string bundleVersion_ { "" };
    std::string bundleVersionCode_ { "" };
    std::string bundleName_ { "" };
    std::string leakReason_ { "" };
    std::string appRunningId_ { "" };
    DumperInfo dumperInfo_;
    time_t startTime_ { 0 }; // record the process start time
    pid_t uid_ { -1 };
    std::atomic<bool> isdumping_ { false };
    bool isDied_ { false };
    ffrt::mutex setProcessDiedMtx_;
    bool isEverSendTelemetryEvent_ { false };
    bool telemetryFlag_ { false };
};

class MemoryLeakInfoBase : public FaultInfoBase {
public:
    MemoryLeakInfoBase(pid_t pid, std::string name);
    virtual std::string GetSampleFilePath() = 0;
    virtual std::string GetRsMemPath() = 0;

    bool IsTriggerHardThreshold() const;
    void SetIsTriggerHardThreshold(bool flag);
    void SetEventMsg(const std::string &msg);
    const std::string& GetEventMsg() const;
    void SetLastGrade(LeakGrade grade);
    size_t GetNextIndex(size_t i);
    size_t GetPervIndex(size_t i);
    void ReportMemoryLeakEvent();
    void ReportMemoryLeakWpEvent();
    std::string GetReportEventName();
    bool ShouldReport();

protected:
    bool isTriggerHardThreshold_ { false };
    std::string eventMsg_;
    LeakGrade lastGrade_ { NO_LEAK };
    time_t lastGradeStamp { 0 };
    ReportEventType reportEventType_ { MEMORY_LEAK_WP_EVENT };
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_FAULT_INFO_BASE_H
