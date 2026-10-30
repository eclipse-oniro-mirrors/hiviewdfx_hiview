/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef DFR_NATIVE_LEAK_REPORT_STATE_H
#define DFR_NATIVE_LEAK_REPORT_STATE_H

#include "fault_state_base.h"
#include "fault_info_base.h"
#include "native_leak_info.h"
#include "fault_common.h"

namespace OHOS {
namespace HiviewDFX {

class NativeLeakReportState : public FaultStateBase {
public:
    ErrCode ChangeNextState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj) override;
    ErrCode StateProcess(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &faultObj) override;
    std::string GenerateSummaryInfo(const std::string &name, const std::shared_ptr<NativeLeakInfo> &nlInfo);
    void HandleAppLeakReport(std::shared_ptr<FaultInfoBase> &faultInfo);
    void DoReportExtPssEvent(std::shared_ptr<FaultInfoBase> &faultInfo, std::string secondFrame,
                             std::string telemetryId);

private:
    std::string GetMaxPssType(const DetailSmapsInfo &detailSmapsInfo);
    struct EventFrameInfo {
        std::string eventName;
        std::string firstFrame;
        std::string secondFrame;
        std::string thirdFrame;
    };
    EventFrameInfo PrepareEventFrames(const std::shared_ptr<NativeLeakInfo> &nlInfo,
                                      FaultStateType curState, bool isTelemetry);
    std::string GenerateSummary(const std::shared_ptr<NativeLeakInfo> &nlInfo,
                                const std::string &name, bool isTelemetry);
    void WriteHiSysEvent(const EventFrameInfo &frameInfo, const std::shared_ptr<NativeLeakInfo> &nlInfo,
                         const std::shared_ptr<FaultInfoBase> &faultInfo, bool isTelemetry);
    void HandleTelemetry(const std::shared_ptr<NativeLeakInfo> &nlInfo, const EventQuota &eq);
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_NATIVE_LEAK_REPORT_STATE_H

