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

#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/wait.h>
#include "kernel_leak_info.h"
#include "kernel_leak_dump_state.h"
#include "kernel_leak_sample_state.h"
#include "kernel_leak_judge_state.h"
#include "kernel_leak_report_state.h"
#include "kernel_leak_removal_state.h"
#include "hiview_logger.h"
#include "hisysevent.h"
#include "hitrace_meter.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "KernelLeakSampleState");

using namespace std;
namespace {
    constexpr int LDK_SLAB_COUNT = 2;
    constexpr int INDEX_CLAIM_0 = 0;
    constexpr int INDEX_CLAIM_1 = 1;
}

ErrCode KernelLeakSampleState::StateProcess(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    string traceStr(__func__);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    auto kernelInfo = static_pointer_cast<KernelLeakInfo>(faultInfo);
    CollectBaseInfo(kernelInfo);
    return SUCCESSED;
}

ErrCode KernelLeakSampleState::ChangeNextState(shared_ptr<FaultInfoBase> &faultInfo, FaultDetectorBase &detectorObj)
{
    return SUCCESSED;
}

void KernelLeakSampleState::CollectBaseInfo(shared_ptr<KernelLeakInfo> &kernelInfo) const
{
    string infoName = kernelInfo->GetMeminfoName();
    string procName = kernelInfo->GetProcessName();
    string traceStr(__func__);
    traceStr.append(":").append(infoName);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    int memsize = GetMemSize(infoName, procName);
    if (memsize == 0) {
        return;
    }
    kernelInfo->AddMemory(memsize);
}

int KernelLeakSampleState::GetMemSize(const string &infoName, const string &procName) const
{
    int memsize = 0;
    if (infoName == "LDKSlab" && procName == "ldk-slab") {
        return GetLdkSlabMem();
    }
    if (procName == "ldk_buddy") {
        return GetLdkDriverMem();
    }

    auto it = memInfoMap_.find(infoName);
    if (it == memInfoMap_.end()) {
        HIVIEW_LOGE("cannot find infoName name: %{public}s", infoName.c_str());
        return 0;
    }
    if (!DfrUtil::GetInstance().SafeStoi(it->second, memsize)) {
        HIVIEW_LOGE("safestoi failed. infoName name: %{public}s", infoName.c_str());
        return 0;
    }
    if (procName == "slab") {
        memsize += DfrUtil::GetInstance().GetSlabNormal();
    }
    return memsize;
}

int KernelLeakSampleState::GetLdkSlabMem() const
{
    string slabInfo = DfrUtil::GetInstance().SendLeakCmdInfo(0, CMD_GET_MEM_TYPE_VALUE, MTYPE_LDK_SLAB);
    vector<string> splitSlabInfo = DfrUtil::GetInstance().Split(slabInfo, ',');
    vector<int> numbers;
    for (auto &info : splitSlabInfo) {
        int num = 0;
        if (!DfrUtil::GetInstance().SafeStoi(info, num)) {
            HIVIEW_LOGE("safastoi failed. info: %{public}s", info.c_str());
            return 0;
        }
        numbers.push_back(num);
    }
    if (numbers.size() != LDK_SLAB_COUNT) {
        HIVIEW_LOGE("ldkslab ioctl info error, need 2 value, but not get 2 value, slabInfo: %{public}s",
            slabInfo.c_str());
        return 0;
    }
    HIVIEW_LOGD("LDKSlab Reclaimable: %{public}d, LDKSlab Unreclaimable: %{public}d",
        numbers[INDEX_CLAIM_0], numbers[INDEX_CLAIM_1]);
    return numbers[INDEX_CLAIM_1];
}

int KernelLeakSampleState::GetLdkDriverMem() const
{
    string driverInfo = DfrUtil::GetInstance().SendLeakCmdInfo(0, CMD_GET_MEM_TYPE_VALUE, MTYPE_LDK_BUDDY);
    int allocPageSize = 0;
    if (!DfrUtil::GetInstance().SafeStoi(driverInfo, allocPageSize)) {
        HIVIEW_LOGE("SafeStoi failed. driverInfo: %{public}s", driverInfo.c_str());
        return 0;
    }
    HIVIEW_LOGI("get ldk_buddy size: %{public}d KB", allocPageSize);
    return allocPageSize;
}

bool KernelLeakSampleState::SampleReady()
{
    string traceStr(__func__);
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);

    string buffer = DfrUtil::GetInstance().GetCacheProcInfo(MEM_INFO_PATH);
    if (buffer.empty()) {
        HIVIEW_LOGE("failed to GetCacheProcInfo meminfo");
        return false;
    }
    string line;
    istringstream stream(buffer);
    while (getline(stream, line)) {
        istringstream lineStream(line);
        string key;
        string value;
        if (getline(lineStream, key, ':') && getline(lineStream, value)) {
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            value = value.substr(0, value.find_first_of(" kB"));
            memInfoMap_[key] = value;
        }
    }
    return true;
}

void KernelLeakSampleState::SampleOver()
{
    memInfoMap_.clear();
}

} // namespace HiviewDFX
} // namespace OHOS
