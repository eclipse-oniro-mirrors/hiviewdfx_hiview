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

#ifndef DFR_KERNEL_SAMPLE_STATE_H
#define DFR_KERNEL_SAMPLE_STATE_H

#include <vector>
#include "fault_state_base.h"
#include "fault_info_base.h"
#include "fault_detector_base.h"
#include "fault_common.h"
#include "kernel_leak_info.h"
#include "ffrt.h"

namespace OHOS {
namespace HiviewDFX {

class KernelLeakSampleState : public FaultStateBase {
public:
    void CollectBaseInfo(std::shared_ptr<KernelLeakInfo> &kernelInfo) const;
    ErrCode ChangeNextState(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj) override;
    ErrCode StateProcess(std::shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj) override;
    bool SampleReady();
    void SampleOver();

private:
    int GetLdkSlabMem() const;
    int GetLdkDriverMem() const;
    int GetMemSize(const std::string &infoName, const std::string &procName) const;

private:
    std::unordered_map<std::string, std::string> memInfoMap_;
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_KERNEL_SAMPLE_STATE_H
