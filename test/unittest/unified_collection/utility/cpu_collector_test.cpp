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
#include <climits>
#include <iostream>
#include <unistd.h>

#include "cpu_collector.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace OHOS::HiviewDFX::UCollect;

class CpuCollectorTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

#ifdef UNIFIED_COLLECTOR_CPU_ENABLE
/**
 * @tc.name: CpuCollectorTest001
 * @tc.desc: used to test CpuCollector.CollectSysCpuLoad
 * @tc.type: FUNC
*/
HWTEST_F(CpuCollectorTest, CpuCollectorTest001, TestSize.Level1)
{
    std::shared_ptr<CpuCollector> collector = CpuCollector::Create();
    CollectResult<SysCpuLoad> result = collector->CollectSysCpuLoad();
    std::cout << "collect system cpu load result=" << result.retCode << std::endl;
    ASSERT_TRUE(result.retCode == UcError::SUCCESS);

    const SysCpuLoad& sysCpuLoad = result.data;
    std::cout << "collect system cpu load, avgLoad1=" << sysCpuLoad.avgLoad1 << std::endl;
    std::cout << "collect system cpu load, avgLoad5=" << sysCpuLoad.avgLoad5 << std::endl;
    std::cout << "collect system cpu load, avgLoad15=" << sysCpuLoad.avgLoad15 << std::endl;
    ASSERT_TRUE(sysCpuLoad.avgLoad1 > 0);
    ASSERT_TRUE(sysCpuLoad.avgLoad5 > 0);
    ASSERT_TRUE(sysCpuLoad.avgLoad15 > 0);
}

/**
 * @tc.name: CpuCollectorTest002
 * @tc.desc: used to test CpuCollector.CollectSysCpuUsage with updating
 * @tc.type: FUNC
*/
HWTEST_F(CpuCollectorTest, CpuCollectorTest002, TestSize.Level1)
{
    std::shared_ptr<CpuCollector> collector = CpuCollector::Create();

    // first collection
    sleep(1); // 1s
    CollectResult<SysCpuUsage> result = collector->CollectSysCpuUsage(true);
    std::cout << "collect1 system cpu usage result=" << result.retCode << std::endl;
    ASSERT_TRUE(result.retCode == UcError::SUCCESS);

    const SysCpuUsage& sysCpuUsage = result.data;
    ASSERT_GT(sysCpuUsage.startTime, 0);
    ASSERT_GT(sysCpuUsage.endTime, sysCpuUsage.startTime);
    ASSERT_GT(sysCpuUsage.cpuInfos.size(), 0);
    for (const auto& cpuInfo : sysCpuUsage.cpuInfos) {
        std::cout << cpuInfo.cpuId << ", userUsage=" << cpuInfo.userUsage
            << ", niceUsage=" << cpuInfo.niceUsage
            << ", systemUsage=" << cpuInfo.systemUsage
            << ", idleUsage=" << cpuInfo.idleUsage
            << ", ioWaitUsage=" << cpuInfo.ioWaitUsage
            << ", irqUsage=" << cpuInfo.irqUsage
            << ", softIrqUsage=" << cpuInfo.softIrqUsage
            << std::endl;
        ASSERT_FALSE(cpuInfo.cpuId.empty());
    }

    // second collection
    sleep(1); // 1s
    CollectResult<SysCpuUsage> nextResult = collector->CollectSysCpuUsage(true);
    std::cout << "collect2 system cpu usage result=" << nextResult.retCode << std::endl;
    ASSERT_TRUE(nextResult.retCode == UcError::SUCCESS);
    ASSERT_EQ(nextResult.data.startTime, sysCpuUsage.endTime);
    ASSERT_GT(nextResult.data.endTime, nextResult.data.startTime);
    ASSERT_EQ(nextResult.data.cpuInfos.size(), sysCpuUsage.cpuInfos.size());
}

/**
 * @tc.name: CpuCollectorTest003
 * @tc.desc: used to test CpuCollector.CollectSysCpuUsage without updating
 * @tc.type: FUNC
*/
HWTEST_F(CpuCollectorTest, CpuCollectorTest003, TestSize.Level1)
{
    std::shared_ptr<CpuCollector> collector = CpuCollector::Create();

    // first collection
    sleep(1); // 1s
    CollectResult<SysCpuUsage> result = collector->CollectSysCpuUsage(false);
    std::cout << "collect1 system cpu usage result=" << result.retCode << std::endl;
    ASSERT_TRUE(result.retCode == UcError::SUCCESS);

    // second collection
    sleep(1); // 1s
    CollectResult<SysCpuUsage> nextResult = collector->CollectSysCpuUsage(false);
    std::cout << "collect2 system cpu usage result=" << nextResult.retCode << std::endl;
    ASSERT_TRUE(nextResult.retCode == UcError::SUCCESS);
    ASSERT_EQ(nextResult.data.startTime, result.data.startTime);
    ASSERT_GT(nextResult.data.endTime, result.data.endTime);
    ASSERT_EQ(nextResult.data.cpuInfos.size(), result.data.cpuInfos.size());
}

/**
 * @tc.name: CpuCollectorTest004
 * @tc.desc: used to test CpuCollector.CollectCpuFrequency
 * @tc.type: FUNC
*/
HWTEST_F(CpuCollectorTest, CpuCollectorTest004, TestSize.Level1)
{
    std::shared_ptr<CpuCollector> collector = CpuCollector::Create();
    CollectResult<std::vector<CpuFreq>> result = collector->CollectCpuFrequency();
    std::cout << "collect system cpu frequency result=" << result.retCode << std::endl;
    ASSERT_TRUE(result.retCode == UcError::SUCCESS);

    const std::vector<CpuFreq>& cpuFreqs = result.data;
    std::cout << "collect system cpu frequency, size=" << cpuFreqs.size() << std::endl;
    ASSERT_GT(cpuFreqs.size(), 0);
    for (size_t i = 0; i < cpuFreqs.size(); ++i) {
        std::cout << "cpu" << cpuFreqs[i].cpuId << ", curFreq=" << cpuFreqs[i].curFreq << ", minFreq="
            << cpuFreqs[i].minFreq << ", maxFreq=" << cpuFreqs[i].maxFreq << std::endl;
        ASSERT_EQ(cpuFreqs[i].cpuId, i);
        ASSERT_GT(cpuFreqs[i].curFreq, 0);
        ASSERT_GT(cpuFreqs[i].minFreq, 0);
        ASSERT_GT(cpuFreqs[i].maxFreq, 0);
    }
}

/**
 * @tc.name: CpuCollectorTest009
 * @tc.desc: used to test CpuCollector.CollectProcessCpuStatInfo
 * @tc.type: FUNC
*/
HWTEST_F(CpuCollectorTest, CpuCollectorTest009, TestSize.Level1)
{
    std::shared_ptr<CpuCollector> collector = CpuCollector::Create();
    constexpr int initPid = 1;
    auto collectResult = collector->CollectProcessCpuStatInfo(initPid);
    ASSERT_TRUE(collectResult.retCode == UcError::SUCCESS);

    std::cout << "proc.procName=" << collectResult.data.procName << std::endl;
    std::cout << "proc.startTime=" << collectResult.data.startTime << std::endl;
    std::cout << "proc.endTime=" << collectResult.data.endTime << std::endl;
    std::cout << "proc.pid=" << collectResult.data.pid << std::endl;
    std::cout << "proc.minFlt=" << collectResult.data.minFlt << std::endl;
    std::cout << "proc.majFlt=" << collectResult.data.majFlt << std::endl;
    std::cout << "proc.cpuLoad=" << collectResult.data.cpuLoad << std::endl;
    std::cout << "proc.uCpuUsage=" << collectResult.data.uCpuUsage << std::endl;
    std::cout << "proc.sCpuUsage=" << collectResult.data.sCpuUsage << std::endl;
    std::cout << "proc.cpuUsage=" << collectResult.data.cpuUsage << std::endl;
    ASSERT_GT(collectResult.data.startTime, 0);
    ASSERT_GT(collectResult.data.endTime, 0);
    ASSERT_EQ(collectResult.data.pid, initPid);
    ASSERT_GE(collectResult.data.minFlt, 0);
    ASSERT_GE(collectResult.data.majFlt, 0);
    ASSERT_GE(collectResult.data.cpuLoad, 0);
    ASSERT_GE(collectResult.data.uCpuUsage, 0);
    ASSERT_GE(collectResult.data.sCpuUsage, 0);
    ASSERT_GE(collectResult.data.cpuUsage, 0);
    ASSERT_FALSE(collectResult.data.procName.empty());

    sleep(1); // 1s
    auto nextCollectResult = collector->CollectProcessCpuStatInfo(initPid);
    ASSERT_TRUE(nextCollectResult.retCode == UcError::SUCCESS);
    ASSERT_EQ(nextCollectResult.data.startTime, collectResult.data.startTime);
    ASSERT_GT(nextCollectResult.data.endTime, collectResult.data.endTime);
}

/**
 * @tc.name: CpuCollectorTest010
 * @tc.desc: used to test CpuCollector.CollectProcessCpuStatInfo
 * @tc.type: FUNC
*/
HWTEST_F(CpuCollectorTest, CpuCollectorTest010, TestSize.Level1)
{
    std::shared_ptr<CpuCollector> collector = CpuCollector::Create();
    constexpr int notExistPid = INT_MAX;
    auto collectResult = collector->CollectProcessCpuStatInfo(notExistPid);
    ASSERT_TRUE(collectResult.retCode != UcError::SUCCESS);
}

/**
 * @tc.name: CpuCollectorTest011
 * @tc.desc: used to test CpuCollector.CollectProcessCpuStatInfo
 * @tc.type: FUNC
*/
HWTEST_F(CpuCollectorTest, CpuCollectorTest011, TestSize.Level1)
{
    std::shared_ptr<CpuCollector> collector = CpuCollector::Create();
    constexpr int invalidPid = -1;
    auto collectResult = collector->CollectProcessCpuStatInfo(invalidPid);
    ASSERT_TRUE(collectResult.retCode != UcError::SUCCESS);
}

/**
 * @tc.name: CpuCollectorTest012
 * @tc.desc: used to test CpuCollector.CollectProcessCpuStatInfos
 * @tc.type: FUNC
*/
HWTEST_F(CpuCollectorTest, CpuCollectorTest012, TestSize.Level1)
{
    std::shared_ptr<CpuCollector> collector = CpuCollector::Create();
    auto collectResult = collector->CollectProcessCpuStatInfos(true);
    ASSERT_TRUE(collectResult.retCode == UcError::SUCCESS);
    ASSERT_FALSE(collectResult.data.empty());

    sleep(1); // 1s
    auto nextCollectResult = collector->CollectProcessCpuStatInfos();
    ASSERT_TRUE(nextCollectResult.retCode == UcError::SUCCESS);
    ASSERT_FALSE(nextCollectResult.data.empty());

    std::cout << "next collection startTime=" << nextCollectResult.data[0].startTime << std::endl;
    std::cout << "next collection endTime=" << nextCollectResult.data[0].endTime << std::endl;
    ASSERT_EQ(nextCollectResult.data[0].startTime, collectResult.data[0].endTime);
    ASSERT_GT(nextCollectResult.data[0].endTime, nextCollectResult.data[0].startTime);
}

/**
 * @tc.name: CpuCollectorTest013
 * @tc.desc: used to test CpuCollector.CollectProcessCpuStatInfos
 * @tc.type: FUNC
*/
HWTEST_F(CpuCollectorTest, CpuCollectorTest013, TestSize.Level1)
{
    std::shared_ptr<CpuCollector> collector = CpuCollector::Create();
    auto collectResult = collector->CollectProcessCpuStatInfos(true);
    ASSERT_TRUE(collectResult.retCode == UcError::SUCCESS);
    ASSERT_FALSE(collectResult.data.empty());

    sleep(1); // 1s
    auto nextCollectResult = collector->CollectProcessCpuStatInfos(true);
    ASSERT_TRUE(nextCollectResult.retCode == UcError::SUCCESS);
    ASSERT_FALSE(nextCollectResult.data.empty());

    ASSERT_GT(nextCollectResult.data[0].startTime, collectResult.data[0].startTime);
    ASSERT_GT(nextCollectResult.data[0].endTime, collectResult.data[0].endTime);
}

/**
 * @tc.name: CpuCollectorTest014
 * @tc.desc: used to test CpuCollector.CollectProcessCpuStatInfos
 * @tc.type: FUNC
*/
HWTEST_F(CpuCollectorTest, CpuCollectorTest014, TestSize.Level1)
{
    std::shared_ptr<CpuCollector> collector = CpuCollector::Create();
    auto firstCollectResult = collector->CollectProcessCpuStatInfos(false);
    ASSERT_TRUE(firstCollectResult.retCode == UcError::SUCCESS);
    ASSERT_FALSE(firstCollectResult.data.empty());

    sleep(1); // 1s
    auto secondCollectResult = collector->CollectProcessCpuStatInfos(false);
    ASSERT_TRUE(secondCollectResult.retCode == UcError::SUCCESS);
    ASSERT_FALSE(secondCollectResult.data.empty());

    ASSERT_EQ(firstCollectResult.data[0].startTime, secondCollectResult.data[0].startTime);
    ASSERT_LT(firstCollectResult.data[0].endTime, secondCollectResult.data[0].endTime);
}

/**
 * @tc.name: CpuCollectorTest015
 * @tc.desc: used to test the update function of CpuCollector.CollectProcessCpuStatInfo
 * @tc.type: FUNC
*/
HWTEST_F(CpuCollectorTest, CpuCollectorTest015, TestSize.Level1)
{
    std::shared_ptr<CpuCollector> collector = CpuCollector::Create();
    constexpr int initPid = 1;
    auto collectResult = collector->CollectProcessCpuStatInfo(initPid, true);
    ASSERT_TRUE(collectResult.retCode == UcError::SUCCESS);

    sleep(1); // 1s
    auto nextCollectResult = collector->CollectProcessCpuStatInfo(initPid, true);
    ASSERT_TRUE(nextCollectResult.retCode == UcError::SUCCESS);

    std::cout << "first collection startTime=" << collectResult.data.startTime << std::endl;
    std::cout << "first collection endTime=" << collectResult.data.endTime << std::endl;
    std::cout << "next collection startTime=" << nextCollectResult.data.startTime << std::endl;
    std::cout << "next collection endTime=" << nextCollectResult.data.endTime << std::endl;
    ASSERT_EQ(nextCollectResult.data.startTime, collectResult.data.endTime);
    ASSERT_GT(nextCollectResult.data.endTime, collectResult.data.endTime);
}

/**
 * @tc.name: CpuCollectorTest016
 * @tc.desc: used to test the function of CpuCollector.GetSysCpuUsage;
 * @tc.type: FUNC
*/
HWTEST_F(CpuCollectorTest, CpuCollectorTest016, TestSize.Level1)
{
    std::shared_ptr<CpuCollector> collector = CpuCollector::Create();
    auto collectResult = collector->GetSysCpuUsage();
    ASSERT_TRUE(collectResult.retCode == UcError::SUCCESS);
    ASSERT_TRUE(collectResult.data >= 0 && collectResult.data <= 1);
}

/**
 * @tc.name: CpuCollectorTest017
 * @tc.desc: used to test the function of CreateThreadStatInfoCollector with self pid;
 * @tc.type: FUNC
*/
HWTEST_F(CpuCollectorTest, CpuCollectorTest017, TestSize.Level1)
{
    std::shared_ptr<CpuCollector> collector = CpuCollector::Create();
    auto threadCollector = collector->CreateThreadCollector(getpid());
    sleep(1);
    auto collectResult1 = threadCollector->CollectThreadStatInfos();
    ASSERT_TRUE(collectResult1.retCode == UcError::SUCCESS);
    sleep(1);
    auto collectResult2 = threadCollector->CollectThreadStatInfos(true);
    ASSERT_TRUE(collectResult2.retCode == UcError::SUCCESS);
    ASSERT_TRUE(collectResult2.data.size() >= 1);
    ASSERT_TRUE(collectResult2.data[0].cpuUsage >= 0);
    ASSERT_TRUE(collectResult1.data[0].startTime == collectResult2.data[0].startTime);
    sleep(1);
    auto collectResult3 = threadCollector->CollectThreadStatInfos();
    ASSERT_TRUE(collectResult3.retCode == UcError::SUCCESS);
    ASSERT_TRUE(collectResult3.data.size() >= 1);
    ASSERT_TRUE(collectResult3.data[0].cpuUsage >= 0);
    ASSERT_TRUE(collectResult2.data[0].endTime == collectResult3.data[0].startTime);
}

/**
 * @tc.name: CpuCollectorTest018
 * @tc.desc: used to test the function of CreateThreadStatInfoCollector with other pid;
 * @tc.type: FUNC
*/
HWTEST_F(CpuCollectorTest, CpuCollectorTest018, TestSize.Level1)
{
    std::shared_ptr<CpuCollector> collector = CpuCollector::Create();
    auto threadCollector = collector->CreateThreadCollector(1);
    sleep(1);
    ASSERT_EQ(threadCollector->GetCollectPid(), 1);
    auto collectResult1 = threadCollector->CollectThreadStatInfos(true);
    ASSERT_TRUE(collectResult1.retCode == UcError::SUCCESS);
    sleep(1);
    auto collectResult2 = threadCollector->CollectThreadStatInfos(false);
    ASSERT_TRUE(collectResult2.retCode == UcError::SUCCESS);
    ASSERT_TRUE(collectResult2.data.size() >= 1);
    ASSERT_TRUE(collectResult2.data[0].cpuUsage >= 0);
    ASSERT_TRUE(collectResult1.data[0].endTime == collectResult2.data[0].startTime);
    sleep(1);
    auto collectResult3 = threadCollector->CollectThreadStatInfos();
    ASSERT_TRUE(collectResult3.retCode == UcError::SUCCESS);
    ASSERT_TRUE(collectResult3.data.size() >= 1);
    ASSERT_TRUE(collectResult3.data[0].cpuUsage >= 0);
    ASSERT_TRUE(collectResult2.data[0].startTime == collectResult3.data[0].startTime);
}

/**
 * @tc.name: CpuCollectorTest019
 * @tc.desc: used to test the function of CpuCollector.Create;
 * @tc.type: FUNC
*/
HWTEST_F(CpuCollectorTest, CpuCollectorTest019, TestSize.Level1)
{
    std::shared_ptr<CpuCollector> collector1 = CpuCollector::Create();
    ASSERT_NE(collector1, nullptr);
    std::shared_ptr<CpuCollector> collector2 = CpuCollector::Create();
    ASSERT_NE(collector2, nullptr);
    ASSERT_EQ(collector1, collector2);

    std::shared_ptr<CpuCollector> collector3 = CpuCollector::Create(false);
    ASSERT_NE(collector3, nullptr);
    std::shared_ptr<CpuCollector> collector4 = CpuCollector::Create(false);
    ASSERT_NE(collector4, nullptr);
    ASSERT_NE(collector3, collector4);
}
#else
/**
 * @tc.name: CpuCollectorTest001
 * @tc.desc: used to test empty CpuCollector
 * @tc.type: FUNC
*/
HWTEST_F(CpuCollectorTest, CpuCollectorTest001, TestSize.Level1)
{
    std::shared_ptr<CpuCollector> collector = CpuCollector::Create();
    auto result1 = collector->CollectSysCpuLoad();
    ASSERT_TRUE(result1.retCode == UcError::FEATURE_CLOSED);

    auto result2 = collector->CollectSysCpuUsage();
    ASSERT_TRUE(result2.retCode == UcError::FEATURE_CLOSED);

    auto result3 = collector->GetSysCpuUsage();
    ASSERT_TRUE(result3.retCode == UcError::FEATURE_CLOSED);

    auto result4 = collector->CollectProcessCpuStatInfo(0);
    ASSERT_TRUE(result4.retCode == UcError::FEATURE_CLOSED);

    auto result5 = collector->CollectCpuFrequency();
    ASSERT_TRUE(result5.retCode == UcError::FEATURE_CLOSED);

    auto result6 = collector->CollectProcessCpuStatInfos();
    ASSERT_TRUE(result6.retCode == UcError::FEATURE_CLOSED);

    auto result7 = collector->CreateThreadCollector(0);
    ASSERT_TRUE(result7 == nullptr);
}
#endif
