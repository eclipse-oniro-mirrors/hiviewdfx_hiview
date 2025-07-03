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

#include <gtest/gtest.h>
#include <map>
#include <string>

#include "file_util.h"
#include "perf_collect_config.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX::UCollectUtil;
namespace OHOS {
namespace HiviewDFX {
namespace {
const std::string COLLECT_CONFIG_TEST_PATH = "/data/test/hiview/ucollection/collector_config_test.json";
const std::string INVALID_CONFIG_PATH = "/data/test/hiview/ucollection/collector_config_test_invalid.json";
}

class PerfCollectConfigTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

/**
 * @tc.name: PerfCollectConfigTest001
 * @tc.desc: used to test GetPerfCount of PerfCollectConfig
 * @tc.type: FUNC
*/
HWTEST_F(PerfCollectConfigTest, PerfCollectConfigTest001, TestSize.Level1)
{
    std::map<PerfCaller, uint8_t> perfConcurrencyConfig = PerfCollectConfig::GetPerfCount(COLLECT_CONFIG_TEST_PATH);
    ASSERT_TRUE(perfConcurrencyConfig.find(PerfCaller::EVENTLOGGER) != perfConcurrencyConfig.end());
    ASSERT_EQ(perfConcurrencyConfig.at(PerfCaller::EVENTLOGGER), 1);
    ASSERT_TRUE(perfConcurrencyConfig.find(PerfCaller::XPOWER) != perfConcurrencyConfig.end());
    ASSERT_EQ(perfConcurrencyConfig.at(PerfCaller::XPOWER), 1);
    ASSERT_TRUE(perfConcurrencyConfig.find(PerfCaller::UNIFIED_COLLECTOR) != perfConcurrencyConfig.end());
    ASSERT_EQ(perfConcurrencyConfig.at(PerfCaller::UNIFIED_COLLECTOR), 1);
    ASSERT_TRUE(perfConcurrencyConfig.find(PerfCaller::PERFORMANCE_FACTORY) != perfConcurrencyConfig.end());
    ASSERT_EQ(perfConcurrencyConfig.at(PerfCaller::PERFORMANCE_FACTORY), 1);
    FileUtil::CreateFile(INVALID_CONFIG_PATH);
    perfConcurrencyConfig = PerfCollectConfig::GetPerfCount(INVALID_CONFIG_PATH);
    const uint8_t perfCountForNormal = 2; // default config for normal scene
    const uint8_t perfCountForFactory = 8; // default config for factory scene
    ASSERT_TRUE(perfConcurrencyConfig.find(PerfCaller::EVENTLOGGER) != perfConcurrencyConfig.end());
    ASSERT_EQ(perfConcurrencyConfig.at(PerfCaller::EVENTLOGGER), perfCountForNormal);
    ASSERT_TRUE(perfConcurrencyConfig.find(PerfCaller::XPOWER) != perfConcurrencyConfig.end());
    ASSERT_EQ(perfConcurrencyConfig.at(PerfCaller::XPOWER), perfCountForNormal);
    ASSERT_TRUE(perfConcurrencyConfig.find(PerfCaller::UNIFIED_COLLECTOR) != perfConcurrencyConfig.end());
    ASSERT_EQ(perfConcurrencyConfig.at(PerfCaller::UNIFIED_COLLECTOR), perfCountForNormal);
    ASSERT_TRUE(perfConcurrencyConfig.find(PerfCaller::PERFORMANCE_FACTORY) != perfConcurrencyConfig.end());
    ASSERT_EQ(perfConcurrencyConfig.at(PerfCaller::PERFORMANCE_FACTORY), perfCountForFactory);
}

/**
 * @tc.name: PerfCollectConfigTest002
 * @tc.desc: used to test GetAllowMemory of PerfCollectConfig
 * @tc.type: FUNC
*/
HWTEST_F(PerfCollectConfigTest, PerfCollectConfigTest002, TestSize.Level1)
{
    int64_t allowMemory = PerfCollectConfig::GetAllowMemory(COLLECT_CONFIG_TEST_PATH);
    ASSERT_EQ(allowMemory, 1000); // 1000 : memory value of test config
    allowMemory = PerfCollectConfig::GetAllowMemory(INVALID_CONFIG_PATH);
    ASSERT_EQ(allowMemory, -1);
}

/**
 * @tc.name: PerfCollectConfigTest003
 * @tc.desc: used to test GetAllowMemory of PerfCollectConfig
 * @tc.type: FUNC
*/
HWTEST_F(PerfCollectConfigTest, PerfCollectConfigTest003, TestSize.Level1)
{
    ASSERT_EQ(PerfCollectConfig::MapPerfCallerToString(PerfCaller::EVENTLOGGER), "EVENTLOGGER");
    ASSERT_EQ(PerfCollectConfig::MapPerfCallerToString(PerfCaller::XPOWER), "XPOWER");
    ASSERT_EQ(PerfCollectConfig::MapPerfCallerToString(PerfCaller::UNIFIED_COLLECTOR), "UNIFIED_COLLECTOR");
    ASSERT_EQ(PerfCollectConfig::MapPerfCallerToString(PerfCaller::PERFORMANCE_FACTORY), "PERFORMANCE_FACTORY");
    const int invalidNum = 100; // 100 : invalid number used to cast to PerfCaller
    ASSERT_EQ(PerfCollectConfig::MapPerfCallerToString(static_cast<PerfCaller>(invalidNum)), "");
}
}
}
