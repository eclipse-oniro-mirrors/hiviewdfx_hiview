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
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

#include "io_collector.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace OHOS::HiviewDFX::UCollect;

class IoCollectorTest : public testing::Test {
public:
    void SetUp() override {}
    void TearDown() override {}
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
};

#ifdef UNIFIED_COLLECTOR_IO_ENABLE
/**
 * @tc.name: IoCollectorTest001
 * @tc.desc: used to test IoCollector.CollectProcessIo
 * @tc.type: FUNC
 */
HWTEST_F(IoCollectorTest, IoCollectorTest001, TestSize.Level1)
{
    std::shared_ptr<IoCollector> collector = IoCollector::Create();
    CollectResult<ProcessIo> data = collector->CollectProcessIo(1000);
    std::cout << "collect process io result" << data.retCode << std::endl;
    ASSERT_TRUE(data.retCode == UcError::SUCCESS);
}

/**
 * @tc.name: IoCollectorTest003
 * @tc.desc: used to test IoCollector.CollectDiskStats
 * @tc.type: FUNC
 */
HWTEST_F(IoCollectorTest, IoCollectorTest003, TestSize.Level1)
{
    std::shared_ptr<IoCollector> collect = IoCollector::Create();
    auto result = collect->CollectDiskStats([] (const DiskStats &stats) {
        return false;
    });
    std::cout << "collect disk stats result " << result.retCode << std::endl;
    ASSERT_TRUE(result.retCode == UcError::SUCCESS);
}

/**
 * @tc.name: IoCollectorTest005
 * @tc.desc: used to test IoCollector.CollectDiskStats
 * @tc.type: FUNC
 */
HWTEST_F(IoCollectorTest, IoCollectorTest005, TestSize.Level1)
{
    std::shared_ptr<IoCollector> collect = IoCollector::Create();
    auto result = collect->CollectDiskStats([] (const DiskStats &stats) {
        return false;
    }, true);
    std::cout << "export disk stats result " << result.retCode << std::endl;
    ASSERT_TRUE(result.retCode == UcError::SUCCESS);
}

/**
 * @tc.name: IoCollectorTest006
 * @tc.desc: used to test IoCollector.CollectEMMCInfo
 * @tc.type: FUNC
 */
HWTEST_F(IoCollectorTest, IoCollectorTest006, TestSize.Level1)
{
    std::shared_ptr<IoCollector> collect = IoCollector::Create();
    auto result = collect->CollectEMMCInfo();
    std::cout << "collect emmc info result " << result.retCode << std::endl;
    ASSERT_TRUE(result.retCode == UcError::SUCCESS);
}

/**
 * @tc.name: IoCollectorTest008
 * @tc.desc: used to test IoCollector.CollectAllProcIoStats
 * @tc.type: FUNC
 */
HWTEST_F(IoCollectorTest, IoCollectorTest008, TestSize.Level1)
{
    std::shared_ptr<IoCollector> collect = IoCollector::Create();
    auto result = collect->CollectAllProcIoStats();
    std::cout << "collect all proc io stats result " << result.retCode << std::endl;
    ASSERT_TRUE(result.retCode == UcError::SUCCESS);
}

/**
 * @tc.name: IoCollectorTest011
 * @tc.desc: used to test IoCollector.CollectSysIoStats
 * @tc.type: FUNC
 */
HWTEST_F(IoCollectorTest, IoCollectorTest011, TestSize.Level1)
{
    std::shared_ptr<IoCollector> collect = IoCollector::Create();
    auto result = collect->CollectSysIoStats();
    std::cout << "collect sys io stats result " << result.retCode << std::endl;
    ASSERT_TRUE(result.retCode == UcError::SUCCESS);
}

/**
 * @tc.name: IoCollectorTest013
 * @tc.desc: used to test file clean
 * @tc.type: FUNC
 */
HWTEST_F(IoCollectorTest, IoCollectorTest013, TestSize.Level3)
{
    DiskStats stats1 {
        .operReadRate = 0.1,
        .operWriteRate = 0.2,
    };
    ASSERT_FALSE(IoCollector::DefaultDiskStatsFilter(stats1));

    DiskStats stats2 {
        .operReadRate = 0.0,
        .operWriteRate = 0.2,
    };
    ASSERT_FALSE(IoCollector::DefaultDiskStatsFilter(stats2));

    DiskStats stats3 {
        .operReadRate = 0.2,
        .operWriteRate = 0.0,
    };
    ASSERT_FALSE(IoCollector::DefaultDiskStatsFilter(stats3));

    DiskStats stats4 {
        .operReadRate = 0,
        .operWriteRate = 0,
    };
    ASSERT_TRUE(IoCollector::DefaultDiskStatsFilter(stats4));
}
#else
/**
 * @tc.name: IoCollectorTest001
 * @tc.desc: used to test empty IoCollector
 * @tc.type: FUNC
 */
HWTEST_F(IoCollectorTest, IoCollectorTest001, TestSize.Level1)
{
    std::shared_ptr<IoCollector> collect = IoCollector::Create();
    auto result1 = collect->CollectProcessIo(0);
    ASSERT_TRUE(result1.retCode == UcError::FEATURE_CLOSED);

    auto result3 = collect->CollectDiskStats();
    ASSERT_TRUE(result3.retCode == UcError::FEATURE_CLOSED);

    auto result5 = collect->CollectEMMCInfo();
    ASSERT_TRUE(result5.retCode == UcError::FEATURE_CLOSED);

    auto result7 = collect->CollectAllProcIoStats();
    ASSERT_TRUE(result7.retCode == UcError::FEATURE_CLOSED);

    auto result9 = collect->CollectSysIoStats();
    ASSERT_TRUE(result9.retCode == UcError::FEATURE_CLOSED);
}
#endif
