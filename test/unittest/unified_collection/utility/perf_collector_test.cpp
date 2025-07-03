/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#include <iostream>

#include "file_util.h"
#include "parameter_ex.h"
#include "perf_collector.h"
#include "string_util.h"
#include "time_util.h"

#include <gtest/gtest.h>
#include <thread>

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace OHOS::HiviewDFX::UCollect;

constexpr char PERF_TEST_DIR[] = "/data/local/tmp/perf_test/";

class PerfCollectorTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};

    static void SetUpTestCase()
    {
        FileUtil::ForceCreateDirectory(PERF_TEST_DIR);
    };

    static void TearDownTestCase()
    {
        FileUtil::ForceRemoveDirectory(PERF_TEST_DIR);
    };
};

#ifdef HAS_HIPERF
/**
 * @tc.name: PerfCollectorTest001
 * @tc.desc: used to test PerfCollector.StartPerf
 * @tc.type: FUNC
*/
HWTEST_F(PerfCollectorTest, PerfCollectorTest001, TestSize.Level1)
{
    auto perfCollector = UCollectUtil::PerfCollector::Create(PerfCaller::EVENTLOGGER);
    vector<pid_t> selectPids = {getpid()};
    std::string filename = "hiperf-";
    filename += TimeUtil::TimestampFormatToDate(TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC,
    "%Y%m%d%H%M%S");
    filename += ".data";
    perfCollector->SetOutputFilename(filename);
    perfCollector->SetSelectPids(selectPids);
    perfCollector->SetTimeStopSec(3);
    CollectResult<bool> data = perfCollector->StartPerf(PERF_TEST_DIR);
    std::cout << "collect perf data result" << data.retCode << std::endl;
    ASSERT_TRUE(data.retCode == UcError::SUCCESS);
}

/**
 * @tc.name: PerfCollectorTest002
 * @tc.desc: used to test PerfCollector.StartPerf
 * @tc.type: FUNC
*/
HWTEST_F(PerfCollectorTest, PerfCollectorTest002, TestSize.Level1)
{
    auto perfCollector = UCollectUtil::PerfCollector::Create(PerfCaller::XPOWER);
    vector<pid_t> selectPids = {getpid()};
    std::string filename = "hiperf-";
    filename += TimeUtil::TimestampFormatToDate(TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC,
    "%Y%m%d%H%M%S");
    filename += ".data";
    std::string filepath = PERF_TEST_DIR + filename;
    perfCollector->SetOutputFilename(filename);
    perfCollector->SetSelectPids(selectPids);
    perfCollector->SetTimeStopSec(3);
    CollectResult<bool> data = perfCollector->StartPerf(PERF_TEST_DIR);
    std::cout << "collect perf data result" << data.retCode << std::endl;
    ASSERT_EQ(FileUtil::FileExists(filepath), true);
}

/**
 * @tc.name: PerfCollectorTest003
 * @tc.desc: used to test PerfCollector.SetTargetSystemWide
 * @tc.type: FUNC
*/
HWTEST_F(PerfCollectorTest, PerfCollectorTest003, TestSize.Level1)
{
    auto perfCollector = UCollectUtil::PerfCollector::Create(PerfCaller::UNIFIED_COLLECTOR);
    vector<pid_t> selectPids = {getpid()};
    std::string filename = "hiperf-";
    filename += TimeUtil::TimestampFormatToDate(TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC,
    "%Y%m%d%H%M%S");
    filename += ".data";
    std::string filepath = PERF_TEST_DIR + filename;
    perfCollector->SetOutputFilename(filename);
    perfCollector->SetTargetSystemWide(true);
    perfCollector->SetCallGraph("fp");
    std::vector<std::string> selectEvents = {"hw-cpu-cycles", "hw-instructions"};
    perfCollector->SetSelectEvents(selectEvents);
    CollectResult<bool> data = perfCollector->Prepare(PERF_TEST_DIR);
    ASSERT_TRUE(data.retCode == UcError::SUCCESS);
    TimeUtil::Sleep(1);
    data = perfCollector->StartRun();
    ASSERT_TRUE(data.retCode == UcError::SUCCESS);
    TimeUtil::Sleep(1);
    data = perfCollector->Stop();
    ASSERT_TRUE(data.retCode == UcError::SUCCESS);
    std::cout << "collect perf data result" << data.retCode << std::endl;
    ASSERT_EQ(FileUtil::FileExists(filepath), true);
}

/**
 * @tc.name: PerfCollectorTest004
 * @tc.desc: used to test PerfCollector.SetCpuPercent
 * @tc.type: FUNC
*/
HWTEST_F(PerfCollectorTest, PerfCollectorTest004, TestSize.Level1)
{
    auto perfCollector = UCollectUtil::PerfCollector::Create(PerfCaller::PERFORMANCE_FACTORY);
    vector<pid_t> selectPids = {getpid()};
    std::string filename = "hiperf-";
    filename += TimeUtil::TimestampFormatToDate(TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC,
    "%Y%m%d%H%M%S");
    filename += ".data";
    std::string filepath = PERF_TEST_DIR + filename;
    perfCollector->SetOutputFilename(filename);
    perfCollector->SetSelectPids(selectPids);
    perfCollector->SetFrequency(100);
    perfCollector->SetTimeStopSec(2);
    perfCollector->SetCpuPercent(100);
    CollectResult<bool> data = perfCollector->StartPerf(PERF_TEST_DIR);
    std::cout << "collect perf data result" << data.retCode << std::endl;
    ASSERT_EQ(FileUtil::FileExists(filepath), true);
}

/**
 * @tc.name: PerfCollectorTest005
 * @tc.desc: used to test PerfCollector.SetReport
 * @tc.type: FUNC
*/
HWTEST_F(PerfCollectorTest, PerfCollectorTest005, TestSize.Level1)
{
    auto perfCollector = UCollectUtil::PerfCollector::Create(PerfCaller::PERFORMANCE_FACTORY);
    vector<pid_t> selectPids = {getpid()};
    std::string filename = "hiperf-";
    filename += TimeUtil::TimestampFormatToDate(TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC,
    "%Y%m%d%H%M%S");
    filename += ".data";
    std::string filepath = PERF_TEST_DIR + filename;
    perfCollector->SetOutputFilename(filename);
    perfCollector->SetSelectPids(selectPids);
    perfCollector->SetTimeStopSec(3);
    perfCollector->SetReport(true);
    CollectResult<bool> data = perfCollector->StartPerf(PERF_TEST_DIR);
    std::cout << "collect perf data result" << data.retCode << std::endl;
    ASSERT_EQ(FileUtil::FileExists(filepath), true);
}

/**
 * @tc.name: PerfCollectorTest006
 * @tc.desc: used to test concurrent control for perf collection
 * @tc.type: FUNC
*/
HWTEST_F(PerfCollectorTest, PerfCollectorTest006, TestSize.Level1)
{
    vector<pid_t> selectPids = {getpid()};
    std::string fileDir = PERF_TEST_DIR;
    for (int index = 0; index < 5; ++index) { // 5 : start 5 threads to collect perf data
        std::string fileName = "concurrency-hiperf-" + std::to_string(index) + ".data";
        std::thread([fileDir, fileName, selectPids]() {
            auto perfCollector = UCollectUtil::PerfCollector::Create(PerfCaller::EVENTLOGGER);
            perfCollector->SetOutputFilename(fileName);
            perfCollector->SetSelectPids(selectPids);
            perfCollector->SetTimeStopSec(3); // 3 : perf collection will stop after this time
            CollectResult<bool> data = perfCollector->StartPerf(fileDir);
        }).detach();
    }
    sleep(5); // 5 : wati 5 seconds to ensure perf collection is completed
    std::vector<std::string> files;
    FileUtil::GetDirFiles(PERF_TEST_DIR, files, false);
    int perfDataCount = 0;
    for (const auto& file : files) {
        if (StringUtil::StartWith(FileUtil::ExtractFileName(file), "concurrency")) {
            ++perfDataCount;
        }
    }
    ASSERT_EQ(perfDataCount, 2); // 2 : max perf count for eventlogger simultaneously
}

/**
 * @tc.name: PerfCollectorTest007
 * @tc.desc: used to test invalid caller for perf collect
 * @tc.type: FUNC
*/
HWTEST_F(PerfCollectorTest, PerfCollectorTest007, TestSize.Level1)
{
    const int invalidNum = 100; // 100 : invalid number used to cast to PerfCaller
    auto perfCollector = UCollectUtil::PerfCollector::Create(static_cast<PerfCaller>(invalidNum));
    vector<pid_t> selectPids = {getpid()};
    std::string filename = "hiperf-";
    filename += TimeUtil::TimestampFormatToDate(TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC,
    "%Y%m%d%H%M%S");
    filename += ".data";
    std::string filepath = PERF_TEST_DIR + filename;
    perfCollector->SetOutputFilename(filename);
    perfCollector->SetSelectPids(selectPids);
    perfCollector->SetTimeStopSec(3);
    perfCollector->SetReport(true);
    CollectResult<bool> data = perfCollector->StartPerf(PERF_TEST_DIR);
    std::cout << "collect perf data result" << data.retCode << std::endl;
    ASSERT_EQ(data.retCode, UcError::PERF_CALLER_NOT_FIND);
    data = perfCollector->Prepare(PERF_TEST_DIR);
    ASSERT_EQ(data.retCode, UcError::PERF_CALLER_NOT_FIND);
}

#else
/**
 * @tc.name: PerfCollectorTest001
 * @tc.desc: used to test empty PerfCollector
 * @tc.type: FUNC
*/
HWTEST_F(PerfCollectorTest, PerfCollectorTest001, TestSize.Level1)
{
    auto perfCollector = UCollectUtil::PerfCollector::Create(PerfCaller::PERFORMANCE_FACTORY);
    perfCollector->SetSelectPids({});
    perfCollector->SetTargetSystemWide(true);
    perfCollector->SetTimeStopSec(0);
    perfCollector->SetFrequency(0);
    perfCollector->SetOffCPU(true);
    perfCollector->SetOutputFilename("");
    perfCollector->SetCallGraph("");
    perfCollector->SetSelectEvents({});
    perfCollector->SetCpuPercent(0);
    perfCollector->SetCpuPercent(true);

    CollectResult<bool> data = perfCollector->StartPerf("");
    ASSERT_TRUE(data.retCode == UcError::FEATURE_CLOSED);
    data = perfCollector->Prepare("");
    ASSERT_TRUE(data.retCode == UcError::FEATURE_CLOSED);
    data = perfCollector->StartRun();
    ASSERT_TRUE(data.retCode == UcError::FEATURE_CLOSED);
    data = perfCollector->Pause();
    ASSERT_TRUE(data.retCode == UcError::FEATURE_CLOSED);
    data = perfCollector->Resume();
    ASSERT_TRUE(data.retCode == UcError::FEATURE_CLOSED);
    data = perfCollector->Stop();
    ASSERT_TRUE(data.retCode == UcError::FEATURE_CLOSED);
}
#endif // HAS_HIPERF

