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

#include "io_collector.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace OHOS::HiviewDFX::UCollect;

class IoCollectorTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
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
 * @tc.name: IoCollectorTest002
 * @tc.desc: used to test IoCollector.CollectRawDiskStats
 * @tc.type: FUNC
*/
HWTEST_F(IoCollectorTest, IoCollectorTest002, TestSize.Level1)
{
    std::shared_ptr<IoCollector> collect = IoCollector::Create();
    auto result = collect->CollectRawDiskStats();
    std::cout << "collect raw disk stats result " << result.retCode << std::endl;
    ASSERT_TRUE(result.retCode == UcError::SUCCESS);
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
 * @tc.name: IoCollectorTest004
 * @tc.desc: used to test IoCollector.ExportDiskStats
 * @tc.type: FUNC
*/
HWTEST_F(IoCollectorTest, IoCollectorTest004, TestSize.Level1)
{
    std::shared_ptr<IoCollector> collect = IoCollector::Create();
    auto result = collect->ExportDiskStats([] (const DiskStats &stats) {
        return false;
    });
    std::cout << "export disk stats result " << result.retCode << std::endl;
    ASSERT_TRUE(result.retCode == UcError::SUCCESS);

    sleep(3);
    auto nextResult = collect->ExportDiskStats();
    std::cout << "export disk stats nextResult " << nextResult.retCode << std::endl;
    ASSERT_TRUE(nextResult.retCode == UcError::SUCCESS);
}

/**
 * @tc.name: IoCollectorTest005
 * @tc.desc: used to test IoCollector.ExportDiskStats
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

    sleep(3);
    auto nextResult = collect->ExportDiskStats();
    std::cout << "export disk stats nextResult " << nextResult.retCode << std::endl;
    ASSERT_TRUE(nextResult.retCode == UcError::SUCCESS);
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
 * @tc.name: IoCollectorTest007
 * @tc.desc: used to test IoCollector.ExportEMMCInfo
 * @tc.type: FUNC
*/
HWTEST_F(IoCollectorTest, IoCollectorTest007, TestSize.Level1)
{
    std::shared_ptr<IoCollector> collect = IoCollector::Create();
    auto result = collect->ExportEMMCInfo();
    std::cout << "export emmc info result " << result.retCode << std::endl;
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
 * @tc.name: IoCollectorTest009
 * @tc.desc: used to test IoCollector.ExportAllProcIoStats
 * @tc.type: FUNC
*/
HWTEST_F(IoCollectorTest, IoCollectorTest009, TestSize.Level1)
{
    std::shared_ptr<IoCollector> collect = IoCollector::Create();
    auto result = collect->ExportAllProcIoStats();
    std::cout << "export all proc io stats result " << result.retCode << std::endl;
    ASSERT_TRUE(result.retCode == UcError::SUCCESS);
}

/**
 * @tc.name: IoCollectorTest010
 * @tc.desc: used to test IoCollector.ExportAllProcIoStats
 * @tc.type: FUNC
*/
HWTEST_F(IoCollectorTest, IoCollectorTest010, TestSize.Level1)
{
    std::shared_ptr<IoCollector> collect = IoCollector::Create();
    auto result = collect->ExportAllProcIoStats();
    std::cout << "export all proc io stats result " << result.retCode << std::endl;
    ASSERT_TRUE(result.retCode == UcError::SUCCESS);

    sleep(3);
    auto nextResult = collect->ExportAllProcIoStats();
    std::cout << "export all proc io stats nextResult " << nextResult.retCode << std::endl;
    ASSERT_TRUE(nextResult.retCode == UcError::SUCCESS);
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
 * @tc.name: IoCollectorTest012
 * @tc.desc: used to test IoCollector.ExportSysIoStats
 * @tc.type: FUNC
*/
HWTEST_F(IoCollectorTest, IoCollectorTest012, TestSize.Level1)
{
    std::shared_ptr<IoCollector> collect = IoCollector::Create();
    auto result = collect->ExportSysIoStats();
    std::cout << "export sys io stats result " << result.retCode << std::endl;
    ASSERT_TRUE(result.retCode == UcError::SUCCESS);
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

    auto result2 = collect->CollectRawDiskStats();
    ASSERT_TRUE(result2.retCode == UcError::FEATURE_CLOSED);

    auto result3 = collect->CollectDiskStats();
    ASSERT_TRUE(result3.retCode == UcError::FEATURE_CLOSED);

    auto result4 = collect->ExportDiskStats();
    ASSERT_TRUE(result4.retCode == UcError::FEATURE_CLOSED);

    auto result5 = collect->CollectEMMCInfo();
    ASSERT_TRUE(result5.retCode == UcError::FEATURE_CLOSED);

    auto result6 = collect->ExportEMMCInfo();
    ASSERT_TRUE(result6.retCode == UcError::FEATURE_CLOSED);

    auto result7 = collect->CollectAllProcIoStats();
    ASSERT_TRUE(result7.retCode == UcError::FEATURE_CLOSED);

    auto result8 = collect->ExportAllProcIoStats();
    ASSERT_TRUE(result8.retCode == UcError::FEATURE_CLOSED);

    auto result9 = collect->CollectSysIoStats();
    ASSERT_TRUE(result9.retCode == UcError::FEATURE_CLOSED);

    auto result10 = collect->ExportSysIoStats();
    ASSERT_TRUE(result10.retCode == UcError::FEATURE_CLOSED);
}
#endif
