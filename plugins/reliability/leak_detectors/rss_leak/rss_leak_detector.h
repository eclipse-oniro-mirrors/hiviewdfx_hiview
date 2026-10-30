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

#ifndef DFR_RSS_LEAK_DETECTOR_H
#define DFR_RSS_LEAK_DETECTOR_H

#include <string>
#include <map>
#include <sys/types.h>
#include "fault_detector_base.h"
#include "detail_info_parser.h"
#include "fault_common.h"
#include "singleton.h"
#include "ffrt.h"

namespace OHOS {
namespace HiviewDFX {

class NativeLeakDetector;

class RssLeakDetector : public FaultDetectorBase, public Singleton<RssLeakDetector> {
    DECLARE_SINGLETON(RssLeakDetector);

public:
    void HandleDetector(int interval) override;
    void OnChangeState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultStateType stateType) override;
    ErrCode ExeNextStateProcess(std::shared_ptr<FaultInfoBase> faultInfo, FaultStateType stateType) override;
    FaultStateBase *GetStateObj(FaultStateType stateType) override;
    void SetAppResourceLimit(const AppResourceLimtEvent &appResourceLimt) override;
    void JudgeNotify(pid_t pid, const TelemetryLeakInfo telemetryInfo);
    void NotifyProcessCreate(pid_t pid);
    void NotifyProcessDied(pid_t pid);
    bool DoRssLeak(std::shared_ptr<FaultInfoBase> &faultInfo, TelemetrySource telemetrySource);
    bool AddTelemetryInfo(const std::string &bundleName, const TelemetryLeakInfo &telemetryInfo);
    bool RemoveTelemetryInfo(const std::string &bundleName);
    void DoSystemTelemetry(pid_t pid, const TelemetryLeakInfo &telemetryInfo);

private:
    bool CheckOverThreshold(pid_t pid, const TelemetryLeakInfo& telemetryInfo, std::string& bundleName);
    bool IsMemoryOverThreshold(SubLeakType subLeakType, int32_t threshold, const DetailSmapsInfo& detailSmapsInfo);
    void HandleSystemTelemetry(std::shared_ptr<FaultInfoBase>& faultInfo, SubLeakType subLeakType);
    void HandleAppTelemetry(pid_t pid, const std::string& bundleName, SubLeakType subLeakType);
    void HandleOverThreshold(pid_t pid, const TelemetryLeakInfo& telemetryInfo, const std::string& bundleName);
    void HandleSystemTelemetryBySubLeakType(pid_t pid, const TelemetryLeakInfo& telemetryInfo,
                                            const std::string& bundleName, std::shared_ptr<FaultInfoBase>& faultInfo);
    bool AddAppTelemetryInfoToList(std::shared_ptr<FaultInfoBase> &faultInfo);
    bool AddProcessInfo(pid_t pid, std::string bundleName);
    void RemoveProcessInfo(pid_t pid);
    size_t GetProcessInfoCount();
    void ClearTelemetryInfo();
    bool ShouldDoMainProcess(int interval) override;
    ErrCode MainProcess() override;

private:

    std::map<pid_t, TelemetryLeakInfo> allInfoList_;
    ffrt::mutex allListMtx_;

    std::map<std::string, TelemetryLeakInfo> telemetryInfoList_;
    ffrt::mutex telemetryInfoMtx_;

    int mainInterval_ { 0 };

    struct LastLeakInfo {
        pid_t pid { -1 };
        std::string bundleName;
    };
    LastLeakInfo lastNativeHeap_;
    LastLeakInfo lastArks_;
    LastLeakInfo lastJsvm_;
    LastLeakInfo lastArkweb_;
    LastLeakInfo lastAshmem_;
    LastLeakInfo lastKmp_;
    LastLeakInfo lastArksSta_;
};

} // namespace HiviewDFX
} // namespace OHOS

#endif // DFR_RSS_LEAK_DETECTOR_H