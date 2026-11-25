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

#include "cpucollector_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "cpu_decorator.h"
#include "fuzz_data_source.h"
#include "utility/cpu_collector.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
namespace {
constexpr size_t BINARY_MOD = 2;
}

static void CpuCollectorFuzzTest(const uint8_t* data, size_t size)
{
#if defined(UNIFIED_COLLECTOR_CPU_ENABLE)
    if (data == nullptr || size == 0) {
        return;
    }
    FuzzDataSource source(data, size);
    int32_t pid = 0;
    (void)source.GetValue(pid);
    uint8_t isRegular = 0;
    (void)source.GetValue(isRegular);
    bool isRegularBool = (isRegular % BINARY_MOD == 0);
    uint8_t isRegular2 = 0;
    (void)source.GetValue(isRegular2);
    bool isRegularBool2 = (isRegular2 % BINARY_MOD == 0);
    uint8_t isRegular3 = 0;
    (void)source.GetValue(isRegular3);
    bool isRegularBool3 = (isRegular3 % BINARY_MOD == 0);
    uint8_t isRegular4 = 0;
    (void)source.GetValue(isRegular4);
    bool isRegularBool4 = (isRegular4 % BINARY_MOD == 0);

    // CpuCollector::Create returns a collector decorated by CpuDecorator,
    // which uses ProcessStatInfoCollector to collect the cpu stat infos
    auto cpuCollector = CpuCollector::Create();
    if (cpuCollector == nullptr) {
        return;
    }
    (void)cpuCollector->CollectSysCpuLoad();
    (void)cpuCollector->CollectSysCpuUsage(isRegularBool);
    (void)cpuCollector->GetSysCpuUsage();
    (void)cpuCollector->CollectCpuFrequency();
    (void)cpuCollector->CollectProcessCpuStatInfo(pid, isRegularBool2);
    (void)cpuCollector->CollectProcessCpuStatInfos(isRegularBool3);
    auto threadCollector = cpuCollector->CreateThreadCollector(pid);
    if (threadCollector != nullptr) {
        (void)threadCollector->CollectThreadStatInfos(isRegularBool4);
    }
    CpuDecorator::SaveStatCommonInfo();
    CpuDecorator::ResetStatInfo();
#else
    (void)data;
    (void)size;
#endif
}
} // namespace UCollectUtil
} // namespace HiviewDFX
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr) {
        return 0;
    }
    OHOS::HiviewDFX::UCollectUtil::CpuCollectorFuzzTest(data, size);
    return 0;
}
