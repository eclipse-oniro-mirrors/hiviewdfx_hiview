/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <regex>
#include <unordered_set>

#ifdef UNIFIED_COLLECTOR_CPU_ENABLE
#include "cpu_decorator.h"
#endif

#include "decorator.h"
#include "file_util.h"

#ifdef UNIFIED_COLLECTOR_GPU_ENABLE
#include "gpu_decorator.h"
#endif

#ifdef UNIFIED_COLLECTOR_EBPF_ENABLE
#include "hiebpf_decorator.h"
#endif

#ifdef UNIFIED_COLLECTOR_HILOG_ENABLE
#include "hilog_decorator.h"
#endif

#ifdef UNIFIED_COLLECTOR_IO_ENABLE
#include "io_decorator.h"
#endif

#ifdef UNIFIED_COLLECTOR_MEMORY_ENABLE
#include "memory_decorator.h"
#endif

#ifdef UNIFIED_COLLECTOR_NETWORK_ENABLE
#include "network_decorator.h"
#endif

#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
#include "trace_decorator.h"
#endif

#ifdef UNIFIED_COLLECTOR_WM_ENABLE
#include "wm_decorator.h"
#endif

#ifdef HAS_HIPROFILER
#include "mem_profiler_decorator.h"
#endif

#ifdef HAS_HIPERF
#include "perf_decorator.h"
#endif

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;

#ifdef HAS_HIPROFILER
using namespace OHOS::Developtools::NativeDaemon;
#endif

namespace {
const std::vector<std::regex> REGEXS = {
    std::regex("^Date:$"),
    std::regex("^\\d{4}-\\d{2}-\\d{2}$"),
    std::regex("API statistics:"),
    std::regex("API TotalCall FailCall AvgLatency\\(us\\) MaxLatency\\(us\\) TotalTimeSpent\\(us\\)"),
    // eg: MemProfilerCollector::Start-1 1 1 144708 144708 144708
    std::regex("\\w{1,}::\\w{1,}(-\\d)?\\s\\d+\\s\\d+\\s\\d+\\s\\d+\\s\\d+"),
    std::regex("Hitrace API detail statistics:"),
    std::regex("Caller FailCall OverCall TotalCall AvgLatency\\(us\\) MaxLatency\\(us\\) TotalTimeSpent\\(us\\)"),
    // eg: OTHER 0 0 1 127609 127609 127609
    std::regex("\\w{1,}\\s\\d+\\s\\d+\\s\\d+\\s\\d+\\s\\d+\\s\\d+")
};

std::unordered_set<std::string> g_collector_names;

void CallCollectorFuncs()
{
#ifdef UNIFIED_COLLECTOR_CPU_ENABLE
    auto cpuCollector = CpuCollector::Create();
    (void)cpuCollector->CollectSysCpuUsage();
#endif

#ifdef UNIFIED_COLLECTOR_GPU_ENABLE
    auto gpuCollector = GpuCollector::Create();
    (void)gpuCollector->CollectSysGpuLoad();
#endif

#ifdef UNIFIED_COLLECTOR_EBPF_ENABLE
    auto hiebpfCollector = HiebpfCollector::Create();
    (void)hiebpfCollector->StartHiebpf(5, "com.ohos.launcher", "/data/local/tmp/ebpf.txt"); // 5 : test duration
#endif

#ifdef UNIFIED_COLLECTOR_HILOG_ENABLE
    constexpr uint32_t TEST_LINE_NUM = 100;
    auto hilogCollector = HilogCollector::Create();
    (void)hilogCollector->CollectLastLog(getpid(), TEST_LINE_NUM);
#endif

#ifdef UNIFIED_COLLECTOR_IO_ENABLE
    auto ioCollector = IoCollector::Create();
    (void)ioCollector->CollectRawDiskStats();
#endif

#ifdef HAS_HIPROFILER
    constexpr int TEST_DURATION = 10;
    constexpr int TEST_INTERVAL = 1;
    auto memProfilerCollector = MemProfilerCollector::Create();
    MemoryProfilerConfig memoryProfilerConfig = {
        .type = NativeMemoryProfilerSaClientManager::NativeMemProfilerType::MEM_PROFILER_LIBRARY,
        .pid = 0,
        .duration = TEST_DURATION,
        .sampleInterval = TEST_INTERVAL,
    };
    memProfilerCollector->Start(memoryProfilerConfig);
#endif

#ifdef UNIFIED_COLLECTOR_MEMORY_ENABLE
    auto memCollector = MemoryCollector::Create();
    (void)memCollector->CollectSysMemory();
#endif

#ifdef UNIFIED_COLLECTOR_NETWORK_ENABLE
    auto networkCollector = NetworkCollector::Create();
    (void)networkCollector->CollectRate();
#endif

#ifdef HAS_HIPERF
    auto perfCollector = PerfCollector::Create(PerfCaller::UNIFIED_COLLECTOR);
    (void)perfCollector->StartPerf("/data/local/tmp/");
#endif

#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
    TraceCollector::Create()->DumpTrace(UCollect::TraceCaller::HIVIEW);
#endif

#ifdef UNIFIED_COLLECTOR_WM_ENABLE
    auto wmCollector = WmCollector::Create();
    (void)wmCollector->ExportWindowsInfo();
#endif
}

void CallStatFuncs()
{
#ifdef UNIFIED_COLLECTOR_CPU_ENABLE
    CpuDecorator::SaveStatCommonInfo();
#endif

#ifdef UNIFIED_COLLECTOR_GPU_ENABLE
    GpuDecorator::SaveStatCommonInfo();
#endif

#ifdef UNIFIED_COLLECTOR_EBPF_ENABLE
    HiebpfDecorator::SaveStatCommonInfo();
#endif

#ifdef UNIFIED_COLLECTOR_HILOG_ENABLE
    HilogDecorator::SaveStatCommonInfo();
#endif

#ifdef UNIFIED_COLLECTOR_IO_ENABLE
    IoDecorator::SaveStatCommonInfo();
#endif

#ifdef UNIFIED_COLLECTOR_MEMORY_ENABLE
    MemoryDecorator::SaveStatCommonInfo();
#endif

#ifdef UNIFIED_COLLECTOR_NETWORK_ENABLE
    NetworkDecorator::SaveStatCommonInfo();
#endif

#ifdef HAS_HIPROFILER
    MemProfilerDecorator::SaveStatCommonInfo();
#endif

#ifdef HAS_HIPERF
    PerfDecorator::SaveStatCommonInfo();
#endif

#ifdef UNIFIED_COLLECTOR_WM_ENABLE
    WmDecorator::SaveStatCommonInfo();
#endif

#ifdef UNIFIED_COLLECTOR_TRACE_ENABLE
    TraceDecorator::SaveStatSpecialInfo();
#endif
}

bool IsMatchAnyRegex(const std::string& line, const std::vector<std::regex>& regs)
{
    return std::any_of(regs.begin(), regs.end(), [line](std::regex reg) {return regex_match(line, reg);});
}

void RemoveCollectorNameIfMatched(const std::string& line, std::unordered_set<std::string>& collectorNames)
{
    for (const auto& name : collectorNames) {
        if (strncmp(line.c_str(), name.c_str(), strlen(name.c_str())) == 0) {
            collectorNames.erase(name);
            return;
        }
    }
}

bool CheckContent(const std::string& fileName, const std::vector<std::regex>& regs,
    std::unordered_set<std::string>& collectorNames)
{
    if (g_collector_names.empty()) {
        return true;
    }

    std::ifstream file;
    file.open(fileName.c_str());
    if (!file.is_open()) {
        return false;
    }
    std::string line;
    while (getline(file, line)) {
        if (line.size() > 0 && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1, 1);
        }
        if (line.size() == 0) {
            continue;
        }
        if (!IsMatchAnyRegex(line, regs)) {
            file.close();
            std::cout << "line:" << line << " not match" << std::endl;
            return false;
        }
        RemoveCollectorNameIfMatched(line, collectorNames);
    }
    file.close();
    for (const auto& name : collectorNames) {
        std::cout << "not match name : " << name << std::endl;
    }
    return collectorNames.empty();
}
}

class DecoratorTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase()
    {
#ifdef HAS_HIPROFILER
        g_collector_names.insert("MemProfilerCollector");
#endif

#ifdef HAS_HIPERF
        g_collector_names.insert("PerfCollector");
#endif

#ifdef UNIFIED_COLLECTOR_NETWORK_ENABLE
        g_collector_names.insert("NetworkCollector");
#endif

#ifdef UNIFIED_COLLECTOR_EBPF_ENABLE
        g_collector_names.insert("HiebpfCollector");
#endif

#ifdef UNIFIED_COLLECTOR_CPU_ENABLE
        g_collector_names.insert("CpuCollector");
#endif

#ifdef UNIFIED_COLLECTOR_GPU_ENABLE
        g_collector_names.insert("GpuCollector");
#endif

#ifdef UNIFIED_COLLECTOR_HILOG_ENABLE
        g_collector_names.insert("HilogCollector");
#endif

#ifdef UNIFIED_COLLECTOR_IO_ENABLE
        g_collector_names.insert("IoCollector");
#endif

#ifdef UNIFIED_COLLECTOR_MEMORY_ENABLE
        g_collector_names.insert("MemoryCollector");
#endif

#ifdef UNIFIED_COLLECTOR_WM_ENABLE
        g_collector_names.insert("WmCollector");
#endif
        system("param set hiviewdfx.ucollection.switchon true");
    };
    static void TearDownTestCase()
    {
        system("param set hiviewdfx.ucollection.switchon false");
    };
};

/**
 * @tc.name: DecoratorTest001
 * @tc.desc: used to test ucollection stat
 * @tc.type: FUNC
*/
HWTEST_F(DecoratorTest, DecoratorTest001, TestSize.Level1)
{
    CallCollectorFuncs();
    CallStatFuncs();
    bool res = CheckContent(UC_STAT_LOG_PATH, REGEXS, g_collector_names);
    ASSERT_TRUE(res);
    if (FileUtil::FileExists(UC_STAT_LOG_PATH)) {
        FileUtil::RemoveFile(UC_STAT_LOG_PATH);
    }
}
