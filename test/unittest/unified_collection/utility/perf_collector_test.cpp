/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
#include "time_util.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace OHOS::HiviewDFX::UCollect;

class PerfCollectorTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

#ifdef HAS_HIPERF
/**
 * @tc.name: PerfCollectorTest001
 * @tc.desc: used to test PerfCollector.StartPerf
 * @tc.type: FUNC
*/
HWTEST_F(PerfCollectorTest, PerfCollectorTest001, TestSize.Level1)
{
    std::string deviceType = Parameter::GetDeviceTypeStr();
    if (deviceType != "wearable") {
        std::shared_ptr<UCollectUtil::PerfCollector> perfCollector = UCollectUtil::PerfCollector::Create();
        vector<pid_t> selectPids = {getpid()};
        std::string filedir = "/data/local/tmp/";
        std::string filename = "hiperf-";
        filename += TimeUtil::TimestampFormatToDate(TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC,
        "%Y%m%d%H%M%S");
        filename += ".data";
        perfCollector->SetOutputFilename(filename);
        perfCollector->SetSelectPids(selectPids);
        perfCollector->SetTimeStopSec(3);
        CollectResult<bool> data = perfCollector->StartPerf(filedir);
        std::cout << "collect perf data result" << data.retCode << std::endl;
        ASSERT_TRUE(data.retCode == UcError::SUCCESS);
    }
}

/**
 * @tc.name: PerfCollectorTest002
 * @tc.desc: used to test PerfCollector.StartPerf
 * @tc.type: FUNC
*/
HWTEST_F(PerfCollectorTest, PerfCollectorTest002, TestSize.Level1)
{
    std::string deviceType = Parameter::GetDeviceTypeStr();
    if (deviceType != "wearable") {
        std::shared_ptr<UCollectUtil::PerfCollector> perfCollector = UCollectUtil::PerfCollector::Create();
        vector<pid_t> selectPids = {getpid()};
        std::string filedir = "/data/local/tmp/";
        std::string filename = "hiperf-";
        filename += TimeUtil::TimestampFormatToDate(TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC,
        "%Y%m%d%H%M%S");
        filename += ".data";
        std::string filepath = filedir + filename;
        perfCollector->SetOutputFilename(filename);
        perfCollector->SetSelectPids(selectPids);
        perfCollector->SetTimeStopSec(3);
        CollectResult<bool> data = perfCollector->StartPerf(filedir);
        std::cout << "collect perf data result" << data.retCode << std::endl;
        ASSERT_EQ(FileUtil::FileExists(filepath), true);
    }
}

/**
 * @tc.name: PerfCollectorTest003
 * @tc.desc: used to test PerfCollector.SetTargetSystemWide
 * @tc.type: FUNC
*/
HWTEST_F(PerfCollectorTest, PerfCollectorTest003, TestSize.Level1)
{
    std::string deviceType = Parameter::GetDeviceTypeStr();
    if (deviceType != "wearable") {
        std::shared_ptr<UCollectUtil::PerfCollector> perfCollector = UCollectUtil::PerfCollector::Create();
        vector<pid_t> selectPids = {getpid()};
        std::string filedir = "/data/local/tmp/";
        std::string filename = "hiperf-";
        filename += TimeUtil::TimestampFormatToDate(TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC,
        "%Y%m%d%H%M%S");
        filename += ".data";
        std::string filepath = filedir + filename;
        perfCollector->SetOutputFilename(filename);
        perfCollector->SetTargetSystemWide(true);
        perfCollector->SetCallGraph("fp");
        std::vector<std::string> selectEvents = {"hw-cpu-cycles", "hw-instructions"};
        perfCollector->SetSelectEvents(selectEvents);
        CollectResult<bool> data = perfCollector->Prepare(filedir);
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
}

/**
 * @tc.name: PerfCollectorTest004
 * @tc.desc: used to test PerfCollector.SetCpuPercent
 * @tc.type: FUNC
*/
HWTEST_F(PerfCollectorTest, PerfCollectorTest004, TestSize.Level1)
{
    std::string deviceType = Parameter::GetDeviceTypeStr();
    if (deviceType != "wearable") {
        std::shared_ptr<UCollectUtil::PerfCollector> perfCollector = UCollectUtil::PerfCollector::Create();
        vector<pid_t> selectPids = {getpid()};
        std::string filedir = "/data/local/tmp/";
        std::string filename = "hiperf-";
        filename += TimeUtil::TimestampFormatToDate(TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC,
        "%Y%m%d%H%M%S");
        filename += ".data";
        std::string filepath = filedir + filename;
        perfCollector->SetOutputFilename(filename);
        perfCollector->SetSelectPids(selectPids);
        perfCollector->SetFrequency(100);
        perfCollector->SetTimeStopSec(2);
        perfCollector->SetCpuPercent(100);
        CollectResult<bool> data = perfCollector->StartPerf(filedir);
        std::cout << "collect perf data result" << data.retCode << std::endl;
        ASSERT_EQ(FileUtil::FileExists(filepath), true);
    }
}

/**
 * @tc.name: PerfCollectorTest005
 * @tc.desc: used to test PerfCollector.SetReport
 * @tc.type: FUNC
*/
HWTEST_F(PerfCollectorTest, PerfCollectorTest005, TestSize.Level1)
{
    std::string deviceType = Parameter::GetDeviceTypeStr();
    if (deviceType != "wearable") {
        std::shared_ptr<UCollectUtil::PerfCollector> perfCollector = UCollectUtil::PerfCollector::Create();
        vector<pid_t> selectPids = {getpid()};
        std::string filedir = "/data/local/tmp/";
        std::string filename = "hiperf-";
        filename += TimeUtil::TimestampFormatToDate(TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC,
        "%Y%m%d%H%M%S");
        filename += ".data";
        std::string filepath = filedir + filename;
        perfCollector->SetOutputFilename(filename);
        perfCollector->SetSelectPids(selectPids);
        perfCollector->SetTimeStopSec(3);
        perfCollector->SetReport(true);
        CollectResult<bool> data = perfCollector->StartPerf(filedir);
        std::cout << "collect perf data result" << data.retCode << std::endl;
        ASSERT_EQ(FileUtil::FileExists(filepath), true);
    }
}
#else
/**
 * @tc.name: PerfCollectorTest001
 * @tc.desc: used to test empty PerfCollector
 * @tc.type: FUNC
*/
HWTEST_F(PerfCollectorTest, PerfCollectorTest001, TestSize.Level1)
{
    auto perfCollector = UCollectUtil::PerfCollector::Create();
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

