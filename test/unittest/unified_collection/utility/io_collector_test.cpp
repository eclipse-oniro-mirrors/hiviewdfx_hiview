/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include <regex>
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

namespace {
// %-13s\t%12s\t%12s\t%12s\t%12s\t%12s\t%12s\t%20s\t%20s
const std::regex ALL_PROC_IO_STATS1("^\\d{1,}\\s{1,}[\\w/\\.:-]{1,}\\s{1,}\\d{1,}(\\s{1,}\\d{1,}\\.\\d{2}){6}$");
const std::regex ALL_PROC_IO_STATS2("^[\\d\\s]{12}[\\s\\w/\\.:-]{13,}[\\s\\d]{13}([\\s\\d\\.]{13}){6}$");
// %-13s\t%20s\t%20s\t%20s\t%20s\t%12s\t%12s\t%12s
const std::regex DISK_STATS1("^\\w{1,}(\\s{1,}\\d{1,}\\.\\d{2}){4}(\\s{1,}\\d{1,}\\.\\d{4}){2}\\s{1,}\\d{1,}$");
const std::regex DISK_STATS2("^[\\w\\s]{13,}([\\s\\d\\.]{13}){4}([\\s\\d\\.]{13}){2}[\\s\\d]{13}$");
// %-15s\t%15s\t%15s\t%15s\t%15s
const std::regex EMMC_INFO1("^[\\w\\.]{1,}\\s{1,}\\w{1,}\\s{1,}[\\w\\s]{1,}\\w{1,}\\s{1,}\\d{1,}\\.\\d{2}$");
const std::regex EMMC_INFO2("^[\\w\\.\\s]{15,}([\\w\\s]{10}){3}[\\s\\d\\.]{12}$");
// %-12.2f\t%12.2f\t%12.2f\t%12.2f\t%12.2f\t%12.2f
const std::regex SYS_IO_STATS1("^\\d{1,}\\.\\d{2}(\\s{1,}\\d{1,}\\.\\d{2}){5}$");
const std::regex SYS_IO_STATS2("^[\\d\\s\\.]{12}([\\d\\s\\.]{13}){5}$");

bool CheckFormat(const std::string &fileName, const std::regex &reg1, const std::regex &reg2)
{
    std::ifstream file;
    file.open(fileName.c_str());
    if (!file.is_open()) {
        return false;
    }
    std::string line;
    getline(file, line);
    while (getline(file, line)) {
        if (line.size() > 0 && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1, 1);
        }
        if (line.size() == 0) {
            continue;
        }
        if (!regex_match(line, reg1) || !regex_match(line, reg2)) {
            file.close();
            return false;
        }
    }
    file.close();
    return true;
}
}

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
    bool flag = CheckFormat(result.data, DISK_STATS1, DISK_STATS2);
    ASSERT_TRUE(flag);

    sleep(3);
    auto nextResult = collect->ExportDiskStats();
    std::cout << "export disk stats nextResult " << nextResult.retCode << std::endl;
    ASSERT_TRUE(nextResult.retCode == UcError::SUCCESS);
    flag = CheckFormat(nextResult.data, DISK_STATS1, DISK_STATS2);
    ASSERT_TRUE(flag);
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
    bool flag = CheckFormat(nextResult.data, DISK_STATS1, DISK_STATS2);
    ASSERT_TRUE(flag);
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
    bool flag = CheckFormat(result.data, EMMC_INFO1, EMMC_INFO2);
    ASSERT_TRUE(flag);
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
    bool flag = CheckFormat(result.data, ALL_PROC_IO_STATS1, ALL_PROC_IO_STATS2);
    ASSERT_TRUE(flag);
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
    bool flag = CheckFormat(result.data, ALL_PROC_IO_STATS1, ALL_PROC_IO_STATS2);
    ASSERT_TRUE(flag);

    sleep(3);
    auto nextResult = collect->ExportAllProcIoStats();
    std::cout << "export all proc io stats nextResult " << nextResult.retCode << std::endl;
    ASSERT_TRUE(nextResult.retCode == UcError::SUCCESS);
    flag = CheckFormat(nextResult.data, ALL_PROC_IO_STATS1, ALL_PROC_IO_STATS2);
    ASSERT_TRUE(flag);
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
    bool flag = CheckFormat(result.data, SYS_IO_STATS1, SYS_IO_STATS2);
    ASSERT_TRUE(flag);
}
