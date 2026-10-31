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

#ifndef DFR_NATIVE_LEAK_XPERF_DETECTOR_H
#define DFR_NATIVE_LEAK_XPERF_DETECTOR_H

#include "fault_detector_base.h"
#include "fault_common.h"
#include "native_leak_info.h"
#include "native_leak_detector.h"
#include "singleton.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {

class NativeLeakXperfDetector : public Singleton<NativeLeakXperfDetector> {
    DECLARE_SINGLETON(NativeLeakXperfDetector);

public:
    void DetectXperfMemoryPeak(std::shared_ptr<Event> event);
    void JudgeXperfProcessOverThreshold(
        const std::vector<ProcessMemory> &processMemoryList, const std::string &faultEventId);
    void SendPidsToXperf(const std::vector<int32_t> &sendPids);
    void SetLogTypeByLeakGrade(const std::shared_ptr<NativeLeakInfo> &nlInfo, LeakGrade grade);
    void ApplyDumpConfig(std::shared_ptr<FaultInfoBase> &faultInfo, const ProcessMemory &pm,
                         std::unordered_map<std::string, DumpConfig> &overThresholdDumpMap,
                         const std::string &faultEventId, std::vector<int32_t> &sendPids);

private:
    time_t lastReviceXperfTime_ = 0;
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_NATIVE_LEAK_XPERF_DETECTOR_H
