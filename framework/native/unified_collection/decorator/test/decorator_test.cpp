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

#include "cpu_decorator.h"
#include "file_util.h"
#include "gpu_decorator.h"
#include "hiebpf_decorator.h"
#include "hilog_decorator.h"
#include "io_decorator.h"
#include "memory_decorator.h"
#include "network_decorator.h"
#include "trace_decorator.h"
#include "trace_manager.h"
#include "wm_decorator.h"
#ifdef HAS_HIPROFILER
#include "mem_profiler_decorator.h"
#include "native_memory_profiler_sa_client_manager.h"
#endif
#ifdef HAS_HIPERF
#include "perf_decorator.h"
#endif

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace OHOS::Developtools::NativeDaemon;

namespace {
constexpr uint32_t TEST_LINE_NUM = 100;
constexpr int TEST_DURATION = 10;
constexpr int TEST_INTERVAL = 1;
TraceManager g_traceManager;
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
    std::regex("\\w{1,}\\s\\d+\\s\\d+\\s\\d+\\s\\d+\\s\\d+\\s\\d+"),
    std::regex("Hitrace Traffic statistics:"),
    std::regex("Caller TraceFile RawSize\\(b\\) UsedSize\\(b\\) TimeSpent\\(us\\) TimeStamp\\(us\\)"),
    // eg: OTHER /data/Other_trace_2024051200504@14036-90732232.sys 176129 25151 127609 1715446244066654
    std::regex("\\w{1,}\\s.{1,}\\.(sys|zip)\\s\\d+\\s\\d+\\s\\d+\\s\\d{16}"),
    std::regex("Hitrace Traffic Compress Ratio:"),
    std::regex("^0.142800$")
};

std::unordered_set<std::string> COLLECTOR_NAMES = {
    "CpuCollector", "GpuCollector", "HiebpfCollector", "HilogCollector",
    "IoCollector", "MemoryCollector", "NetworkCollector", "TraceCollector", "WmCollector",
};

void CallCollectorFuncs()
{
    auto cpuCollector = CpuCollector::Create();
    (void)cpuCollector->CollectSysCpuUsage();
    auto gpuCollector = GpuCollector::Create();
    (void)gpuCollector->CollectSysGpuLoad();
    auto hiebpfCollector = HiebpfCollector::Create();
    (void)hiebpfCollector->StartHiebpf(5, "com.ohos.launcher", "/data/local/tmp/ebpf.txt"); // 5 : test duration
    auto hilogCollector = HilogCollector::Create();
    (void)hilogCollector->CollectLastLog(getpid(), TEST_LINE_NUM);
    auto ioCollector = IoCollector::Create();
    (void)ioCollector->CollectRawDiskStats();
#ifdef HAS_HIPROFILER
    auto memProfilerCollector = MemProfilerCollector::Create();
    memProfilerCollector->Start(NativeMemoryProfilerSaClientManager::NativeMemProfilerType::MEM_PROFILER_LIBRARY,
        0, TEST_DURATION, TEST_INTERVAL);
#endif
    auto memCollector = MemoryCollector::Create();
    (void)memCollector->CollectSysMemory();
    auto networkCollector = NetworkCollector::Create();
    (void)networkCollector->CollectRate();
#ifdef HAS_HIPERF
    auto perfCollector = PerfCollector::Create(PerfCaller::UNIFIED_COLLECTOR);
    (void)perfCollector->StartPerf("/data/local/tmp/");
#endif
    auto traceCollector = TraceCollector::Create();
    UCollect::TraceCaller caller = UCollect::TraceCaller::OTHER;
    const std::vector<std::string> tagGroups = {"scene_performance"};
    (void)g_traceManager.OpenSnapshotTrace(tagGroups);
    CollectResult<std::vector<std::string>> resultDumpTrace = traceCollector->DumpTrace(caller);
    (void)g_traceManager.CloseTrace();
    (void)traceCollector->TraceOff();
    auto wmCollector = WmCollector::Create();
    (void)wmCollector->ExportWindowsInfo();
}

void CallStatFuncs()
{
    CpuDecorator::SaveStatCommonInfo();
    GpuDecorator::SaveStatCommonInfo();
    HiebpfDecorator::SaveStatCommonInfo();
    HilogDecorator::SaveStatCommonInfo();
    IoDecorator::SaveStatCommonInfo();
    MemoryDecorator::SaveStatCommonInfo();
    NetworkDecorator::SaveStatCommonInfo();
    TraceDecorator::SaveStatCommonInfo();
#ifdef HAS_HIPROFILER
    MemProfilerDecorator::SaveStatCommonInfo();
#endif
#ifdef HAS_HIPERF
    PerfDecorator::SaveStatCommonInfo();
#endif
    WmDecorator::SaveStatCommonInfo();
    TraceDecorator::SaveStatSpecialInfo();
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
    return collectorNames.empty() ? true : false;
}
}

class DecoratorTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase()
    {
#ifdef HAS_HIPROFILER
        COLLECTOR_NAMES.insert("MemProfilerCollector");
#endif
#ifdef HAS_HIPERF
        COLLECTOR_NAMES.insert("PerfCollector");
#endif
    };
    static void TearDownTestCase() {};
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
    bool res = CheckContent(UC_STAT_LOG_PATH, REGEXS, COLLECTOR_NAMES);
    ASSERT_TRUE(res);
    if (FileUtil::FileExists(UC_STAT_LOG_PATH)) {
        FileUtil::RemoveFile(UC_STAT_LOG_PATH);
    }
}
