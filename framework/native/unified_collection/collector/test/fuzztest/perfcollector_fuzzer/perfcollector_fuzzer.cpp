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

#include "perfcollector_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "fuzz_data_source.h"
#include "utility/perf_collector.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
namespace {
constexpr size_t BINARY_MOD = 2;
constexpr int MAX_CPU_PERCENT = 100; // 100: max cpu percent
constexpr char PERF_LOG_DIR[] = "/data/local/tmp";
}

static void PerfCollectorFuzzTest(const uint8_t* data, size_t size)
{
#if defined(HAS_HIPERF)
    if (data == nullptr || size == 0) {
        return;
    }
    FuzzDataSource source(data, size);
    auto perfCollector = PerfCollector::Create(PerfCaller::UNIFIED_COLLECTOR);
    if (perfCollector == nullptr) {
        return;
    }
    pid_t selectPid = 0;
    (void)source.GetValue(selectPid);
    perfCollector->SetSelectPids({selectPid});
    uint8_t targetSystemWide = 0;
    (void)source.GetValue(targetSystemWide);
    perfCollector->SetTargetSystemWide(targetSystemWide % BINARY_MOD == 0);
    int timeStopSec = 0;
    (void)source.GetValue(timeStopSec);
    perfCollector->SetTimeStopSec(timeStopSec + 1);
    int frequency = 0;
    (void)source.GetValue(frequency);
    perfCollector->SetFrequency(frequency + 1);
    uint8_t offCPU = 0;
    (void)source.GetValue(offCPU);
    perfCollector->SetOffCPU(offCPU % BINARY_MOD == 0);
    uint8_t report = 0;
    (void)source.GetValue(report);
    perfCollector->SetReport(report % BINARY_MOD == 0);
    int cpuPercent = 0;
    (void)source.GetValue(cpuPercent);
    perfCollector->SetCpuPercent(cpuPercent % MAX_CPU_PERCENT);
    std::string strData;
    constexpr size_t MAX_STR_LEN = 100;
    (void)source.GetString(strData, MAX_STR_LEN);
    perfCollector->SetOutputFilename(strData);
    perfCollector->SetCallGraph(strData);
    perfCollector->SetSelectEvents({strData});
    (void)perfCollector->Prepare(PERF_LOG_DIR);
    (void)perfCollector->StartRun();
    (void)perfCollector->Pause();
    (void)perfCollector->Resume();
    (void)perfCollector->Stop();
    (void)perfCollector->StartPerf(PERF_LOG_DIR);
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
    OHOS::HiviewDFX::UCollectUtil::PerfCollectorFuzzTest(data, size);
    return 0;
}
