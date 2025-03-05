/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "freeze_detector_unittest.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <unistd.h>

#include "event.h"
#include "file_util.h"
#include "time_util.h"

#define private public
#include "freeze_common.h"
#include "rule_cluster.h"
#include "resolver.h"
#include "vendor.h"
#include "freeze_detector_plugin.h"
#undef private
#include "sys_event.h"
#include "watch_point.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
void FreezeDetectorUnittest::SetUp()
{
    /**
     * @tc.setup: create work directories
     */
    printf("SetUp.\n");
}
void FreezeDetectorUnittest::SetUpTestCase()
{
    /**
     * @tc.setup: all first
     */
    printf("SetUpTestCase.\n");
}

void FreezeDetectorUnittest::TearDownTestCase()
{
    /**
     * @tc.setup: all end
     */
    printf("TearDownTestCase.\n");
}

void FreezeDetectorUnittest::TearDown()
{
    /**
     * @tc.teardown: destroy the event loop we have created
     */
    printf("TearDown.\n");
}

/**
 * @tc.name: FreezeResolver_001
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeResolver_001, TestSize.Level3)
{
    FreezeResolver freezeResolver(nullptr);
    ASSERT_EQ(freezeResolver.Init(), false);
}

/**
 * @tc.name: FreezeResolver_002
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeResolver_002, TestSize.Level3)
{
    auto freezeCommon = std::make_shared<FreezeCommon>();
    auto freezeResolver = std::make_unique<FreezeResolver>(freezeCommon);
    ASSERT_EQ(freezeResolver->Init(), false);
}

/**
 * @tc.name: FreezeResolver_003
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeResolver_003, TestSize.Level3)
{
    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon->Init();
    ASSERT_EQ(ret1, true);
    auto freezeResolver = std::make_unique<FreezeResolver>(freezeCommon);
    ASSERT_EQ(freezeResolver->Init(), true);
}

/**
 * @tc.name: FreezeResolver_004
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeResolver_004, TestSize.Level3)
{
    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon->Init();
    ASSERT_EQ(ret1, true);
    auto freezeResolver = std::make_unique<FreezeResolver>(freezeCommon);
    ASSERT_EQ(freezeResolver->Init(), true);
    auto time = freezeResolver->GetTimeZone();
    ASSERT_NE(time, "");
}

/**
 * @tc.name: FreezeResolver_005
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeResolver_005, TestSize.Level3)
{
    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon->Init();
    ASSERT_EQ(ret1, true);
    auto freezeResolver = std::make_unique<FreezeResolver>(freezeCommon);
    ASSERT_EQ(freezeResolver->Init(), true);
    WatchPoint watchPoint = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR")
        .InitStringId("SCREEN_ON")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .Build();

    ASSERT_EQ(freezeResolver->ProcessEvent(watchPoint), 0);
}

/**
 * @tc.name: FreezeResolver_006
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeResolver_006, TestSize.Level3)
{
    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon->Init();
    ASSERT_EQ(ret1, true);
    auto freezeResolver = std::make_unique<FreezeResolver>(freezeCommon);
    WatchPoint watchPoint = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR")
        .InitStringId("SCREEN_ON")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .Build();
    ASSERT_EQ(freezeResolver->ProcessEvent(watchPoint), -1);
}

/**
 * @tc.name: FreezeResolver_007
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeResolver_007, TestSize.Level3)
{
    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon->Init();
    ASSERT_EQ(ret1, true);
    auto freezeResolver = std::make_unique<FreezeResolver>(freezeCommon);
    ASSERT_EQ(freezeResolver->Init(), true);
    WatchPoint watchPoint = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("ACE")
        .InitStringId("UI_BLOCK_3S")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .Build();
    ASSERT_EQ(freezeResolver->ProcessEvent(watchPoint), -1);
}

/**
 * @tc.name: FreezeResolver_008
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeResolver_008, TestSize.Level3)
{
    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon->Init();
    ASSERT_EQ(ret1, true);
    auto freezeResolver = std::make_unique<FreezeResolver>(freezeCommon);
    ASSERT_EQ(freezeResolver->Init(), true);
    WatchPoint watchPoint = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("ACE")
        .InitStringId("UI_BLOCK_6S")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .Build();
    std::vector<WatchPoint> list;
    std::vector<FreezeResult> result;
    EXPECT_FALSE(freezeResolver->JudgmentResult(watchPoint, list,
        result));
    WatchPoint watchPoint1 = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR")
        .InitStringId("UI_BLOCK_RECOVERED")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .Build();
    list.push_back(watchPoint1);
    EXPECT_FALSE(freezeResolver->JudgmentResult(watchPoint, list,
        result));
    FreezeResult result1;
    FreezeResult result2;
    result2.SetAction("or");
    result.push_back(result1);
    result.push_back(result2);
    EXPECT_FALSE(freezeResolver->JudgmentResult(watchPoint, list,
        result));
    EXPECT_FALSE(freezeResolver->JudgmentResult(watchPoint1, list,
        result));
}

/**
 * @tc.name: FreezeVender_001
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeVender_001, TestSize.Level3)
{
    auto vendor = std::make_unique<Vendor>(nullptr);
    ASSERT_EQ(vendor->Init(), false);
    std::list<WatchPoint> list;
    FreezeResult result;
    ASSERT_EQ(vendor->ReduceRelevanceEvents(list, result), false);
}

/**
 * @tc.name: FreezeVender_002
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeVender_002, TestSize.Level3)
{
    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon->Init();
    ASSERT_EQ(ret1, true);
    auto vendor = std::make_unique<Vendor>(freezeCommon);
    ASSERT_EQ(vendor->Init(), true);
    std::list<WatchPoint> list;
    FreezeResult result;
    result.SetId(3);
    ASSERT_EQ(vendor->ReduceRelevanceEvents(list, result), false);
}

/**
 * @tc.name: FreezeVender_003
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeVender_003, TestSize.Level3)
{
    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon->Init();
    ASSERT_EQ(ret1, true);
    auto vendor = std::make_unique<Vendor>(freezeCommon);
    ASSERT_EQ(vendor->Init(), true);
    std::list<WatchPoint> list;
    FreezeResult result;
    result.SetId(1);
    WatchPoint watchPoint1 = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR")
        .InitStringId("SCREEN_ON")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .Build();
    list.push_back(watchPoint1);
    WatchPoint watchPoint2 = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR3")
        .InitStringId("SCREEN_ON223")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .Build();
    list.push_back(watchPoint2);
    ASSERT_EQ(vendor->ReduceRelevanceEvents(list, result), true);
}

/**
 * @tc.name: FreezeVender_004
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeVender_004, TestSize.Level3)
{
    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon->Init();
    ASSERT_EQ(ret1, true);
    auto vendor = std::make_unique<Vendor>(freezeCommon);
    ASSERT_EQ(vendor->Init(), true);
    std::list<WatchPoint> list;
    FreezeResult result;
    result.SetId(0);
    WatchPoint watchPoint1 = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR")
        .InitStringId("SCREEN_ON")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .Build();
    list.push_back(watchPoint1);
    WatchPoint watchPoint2 = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("AAFWK")
        .InitStringId("THREAD_BLOCK_6S")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .Build();
    list.push_back(watchPoint2);
    ASSERT_EQ(vendor->ReduceRelevanceEvents(list, result), true);
}

/**
 * @tc.name: FreezeVender_005
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeVender_005, TestSize.Level3)
{
    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon->Init();
    ASSERT_EQ(ret1, true);
    auto vendor = std::make_unique<Vendor>(freezeCommon);
    ASSERT_EQ(vendor->Init(), true);
    ASSERT_EQ(vendor->GetTimeString(1687836954734), "20230627113554");
}

/**
 * @tc.name: FreezeVender_006
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeVender_006, TestSize.Level3)
{
    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon->Init();
    ASSERT_EQ(ret1, true);
    auto vendor = std::make_unique<Vendor>(freezeCommon);
    ASSERT_EQ(vendor->Init(), true);

    std::ostringstream oss;
    std::string header = "header";
    WatchPoint watchPoint = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR")
        .InitStringId("SCREEN_ON")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .InitPid(1000)
        .InitUid(1000)
        .InitProcessName("processName")
        .InitPackageName("com.package.name")
        .InitMsg("msg")
        .Build();
    vendor->DumpEventInfo(oss, header, watchPoint);
}

/**
 * @tc.name: FreezeVender_007
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeVender_007, TestSize.Level3)
{
    WatchPoint watchPoint = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR")
        .InitStringId("SCREEN_ON")
        .InitTimestamp(1687859103947)
        .InitPid(1000)
        .InitUid(1000)
        .InitProcessName("processName")
        .InitPackageName("com.package.name")
        .InitHitraceTime("20230627")
        .InitSysrqTime("20230627")
        .Build();

    std::vector<WatchPoint> list;
    WatchPoint watchPoint1 = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR")
        .InitStringId("SCREEN_ON")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .InitLogPath("nolog")
        .InitMsg("msg")
        .Build();
    list.push_back(watchPoint1);

    WatchPoint watchPoint2 = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("AAFWK")
        .InitStringId("THREAD_BLOCK_6S")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .InitLogPath("nolog")
        .InitMsg("msg")
        .Build();
    list.push_back(watchPoint2);

    std::vector<FreezeResult> result;

    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon->Init();
    ASSERT_EQ(ret1, true);
    auto vendor = std::make_unique<Vendor>(freezeCommon);
    ASSERT_EQ(vendor->Init(), true);

    ASSERT_TRUE(!vendor->MergeEventLog(watchPoint, list, result).empty());
}

/**
 * @tc.name: FreezeVender_008
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeVender_008, TestSize.Level3)
{
    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon->Init();
    ASSERT_EQ(ret1, true);
    auto vendor = std::make_unique<Vendor>(freezeCommon);
    ASSERT_EQ(vendor->Init(), true);
    WatchPoint watchPoint = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR")
        .InitStringId("SCREEN_ON")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .InitProcessName("processName")
        .InitPackageName("com.package.name")
        .InitMsg("msg")
        .Build();
    std::vector<WatchPoint> list;
    list.push_back(watchPoint);
    vendor->MergeFreezeJsonFile(watchPoint, list);

    auto vendor1 = std::make_unique<Vendor>(nullptr);
    std::vector<FreezeResult> result;
    std::string ret = vendor->MergeEventLog(watchPoint, list, result);
    printf("MergeEventLog ret = %s\n.", ret.c_str());
}

/**
 * @tc.name: FreezeVender_009
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeVender_009, TestSize.Level3)
{
    auto vendor1 = std::make_unique<Vendor>(nullptr);
    EXPECT_EQ(vendor1->Init(), false);
    WatchPoint watchPoint = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR")
        .InitStringId("SCREEN_ON")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .InitProcessName("processName")
        .InitPackageName("com.package.name")
        .InitMsg("msg")
        .Build();
    vendor1->SendFaultLog(watchPoint, "test", "sysfreeze", "processName", "No");

    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon->Init();
    EXPECT_EQ(ret1, true);
    auto vendor = std::make_unique<Vendor>(freezeCommon);
    EXPECT_EQ(vendor->Init(), true);
    std::string ret = vendor->GetPowerStateString(OHOS::PowerMgr::PowerState::FREEZE);
    EXPECT_EQ(ret, "FREEZE");
    ret = vendor->GetPowerStateString(OHOS::PowerMgr::PowerState::INACTIVE);
    EXPECT_EQ(ret, "INACTIVE");
    ret = vendor->GetPowerStateString(OHOS::PowerMgr::PowerState::STAND_BY);
    EXPECT_EQ(ret, "STAND_BY");
    ret = vendor->GetPowerStateString(OHOS::PowerMgr::PowerState::DOZE);
    EXPECT_EQ(ret, "DOZE");
    ret = vendor->GetPowerStateString(OHOS::PowerMgr::PowerState::SLEEP);
    EXPECT_EQ(ret, "SLEEP");
    ret = vendor->GetPowerStateString(OHOS::PowerMgr::PowerState::HIBERNATE);
    EXPECT_EQ(ret, "HIBERNATE");
    ret = vendor->GetPowerStateString(OHOS::PowerMgr::PowerState::SHUTDOWN);
    EXPECT_EQ(ret, "SHUTDOWN");
    ret = vendor->GetPowerStateString(OHOS::PowerMgr::PowerState::UNKNOWN);
    EXPECT_EQ(ret, "UNKNOWN");
}

/**
 * @tc.name: FreezeRuleCluster_001
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeRuleCluster_001, TestSize.Level3)
{
    auto freezeRuleCluster = std::make_unique<FreezeRuleCluster>();
    ASSERT_EQ(freezeRuleCluster->Init(), true);
}

/**
 * @tc.name: FreezeRuleCluster_002
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeRuleCluster_002, TestSize.Level3)
{
    auto freezeRuleCluster = std::make_unique<FreezeRuleCluster>();
    ASSERT_EQ(freezeRuleCluster->Init(), true);
    ASSERT_EQ(freezeRuleCluster->CheckFileSize("path"), false);
    ASSERT_EQ(freezeRuleCluster->CheckFileSize("/system/etc/hiview/freeze_rules.xml"), true);
}

/**
 * @tc.name: FreezeRuleCluster_003
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeRuleCluster_003, TestSize.Level3)
{
    auto freezeRuleCluster = std::make_unique<FreezeRuleCluster>();
    ASSERT_EQ(freezeRuleCluster->Init(), true);
    ASSERT_EQ(freezeRuleCluster->ParseRuleFile("path"), false);
    ASSERT_EQ(freezeRuleCluster->ParseRuleFile("/system/etc/hiview/freeze_rules.xml"), true);
}

/**
 * @tc.name: FreezeRuleCluster_004
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeRuleCluster_004, TestSize.Level3)
{
    auto freezeRuleCluster = std::make_unique<FreezeRuleCluster>();
    ASSERT_EQ(freezeRuleCluster->Init(), true);

    WatchPoint watchPoint = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR")
        .InitStringId("SCREEN_ON")
        .Build();

    std::vector<FreezeResult> list;
    FreezeResult result1;
    result1.SetId(1);
    FreezeResult result2;
    result2.SetId(2);
    list.push_back(result1);
    list.push_back(result2);

    ASSERT_EQ(freezeRuleCluster->GetResult(watchPoint, list), true);
}

/**
 * @tc.name: FreezeRule_001
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeRule_001, TestSize.Level3)
{
    auto freezeRule = std::make_unique<FreezeRule>();
    FreezeResult result;
    result.SetId(1);

    freezeRule->AddResult("KERNEL_VENDOR", "SCREEN_ON", result);
    ASSERT_EQ(freezeRule->GetResult("KERNEL_VENDOR", "SCREEN_ON", result), true);
}

/**
 * @tc.name: FreezeRule_002
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeRule_002, TestSize.Level3)
{
    auto freezeRule = std::make_unique<FreezeRule>();
    FreezeResult result;
    result.SetId(1);

    ASSERT_EQ(freezeRule->GetResult("KERNEL_VENDOR", "SCREEN_ON", result), false);
}

/**
 * @tc.name: FreezeDetectorPlugin_001
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeDetectorPlugin_001, TestSize.Level3)
{
    auto freezeDetectorPlugin = std::make_unique<FreezeDetectorPlugin>();
    ASSERT_EQ(freezeDetectorPlugin->ReadyToLoad(), true);
    freezeDetectorPlugin->OnLoad();
}

/**
 * @tc.name: FreezeDetectorPlugin_002
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeDetectorPlugin_002, TestSize.Level3)
{
    auto freezeDetectorPlugin = std::make_unique<FreezeDetectorPlugin>();
    auto event = std::make_shared<Event>("sender", "event");
    ASSERT_EQ(freezeDetectorPlugin->OnEvent(event), false);
}

/**
 * @tc.name: FreezeDetectorPlugin_003
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeDetectorPlugin_003, TestSize.Level3)
{
    auto freezeDetectorPlugin = std::make_unique<FreezeDetectorPlugin>();
    freezeDetectorPlugin->OnLoad();
    ASSERT_NE(freezeDetectorPlugin, nullptr);
}

/**
 * @tc.name: FreezeDetectorPlugin_004
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeDetectorPlugin_004, TestSize.Level3)
{
    auto freezeDetectorPlugin = std::make_unique<FreezeDetectorPlugin>();
    freezeDetectorPlugin->OnUnload();
    ASSERT_NE(freezeDetectorPlugin, nullptr);
}

/**
 * @tc.name: FreezeDetectorPlugin_005
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeDetectorPlugin_005, TestSize.Level3)
{
    auto freezeDetectorPlugin = std::make_unique<FreezeDetectorPlugin>();
    auto event = std::make_shared<Event>("sender", "event");
    ASSERT_EQ(freezeDetectorPlugin->CanProcessEvent(event), false);
}

/**
 * @tc.name: FreezeDetectorPlugin006
 * @tc.desc: FreezeDetectorPlugin send LIFECYCLE_TIMEOUT
 * @tc.type: FUNC
 * @tc.require: AR000H3T5D
 */
HWTEST_F(FreezeDetectorUnittest, FreezeDetectorPluginTest006, TestSize.Level3)
{
    auto freezeDetectorPlugin = std::make_unique<FreezeDetectorPlugin>();
    auto event = std::make_shared<Event>("sender", "event");
    freezeDetectorPlugin->OnEventListeningCallback(*(event.get()));
    ASSERT_TRUE(freezeDetectorPlugin != nullptr);
}

/**
 * @tc.name: FreezeDetectorPlugin007
 * @tc.desc: add testcase coverage
 * @tc.type: FUNC
 */
HWTEST_F(FreezeDetectorUnittest, FreezeDetectorPluginTest007, TestSize.Level3)
{
    auto freezeDetectorPlugin = std::make_unique<FreezeDetectorPlugin>();
    freezeDetectorPlugin->RemoveRedundantNewline("test1\\ntest2\\ntest3");
    WatchPoint watchPoint = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR")
        .InitStringId("SCREEN_ON")
        .Build();
    freezeDetectorPlugin->ProcessEvent(watchPoint);
}

/**
 * @tc.name: FreezeCommon_001
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeCommon_001, TestSize.Level3)
{
    auto freezeCommon = std::make_unique<FreezeCommon>();
    ASSERT_EQ(freezeCommon->Init(), true);
    ASSERT_EQ(freezeCommon->IsBetaVersion(), true);
}

/**
 * @tc.name: FreezeCommon_002
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeCommon_002, TestSize.Level3)
{
    auto freezeCommon = std::make_unique<FreezeCommon>();
    ASSERT_EQ(freezeCommon->IsFreezeEvent("KERNEL_VENDOR", "SCREEN_ON"), false);
    freezeCommon->Init();
    ASSERT_EQ(freezeCommon->IsFreezeEvent("KERNEL_VENDOR", "SCREEN_ON"), true);
}

/**
 * @tc.name: FreezeCommon_003
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeCommon_003, TestSize.Level3)
{
    auto freezeCommon = std::make_unique<FreezeCommon>();
    ASSERT_EQ(freezeCommon->IsApplicationEvent("KERNEL_VENDOR", "SCREEN_ON"), false);
    freezeCommon->Init();
    ASSERT_EQ(freezeCommon->IsApplicationEvent("KERNEL_VENDOR", "SCREEN_ON"), false);
}

/**
 * @tc.name: FreezeCommon_004
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeCommon_004, TestSize.Level3)
{
    auto freezeCommon = std::make_unique<FreezeCommon>();
    ASSERT_EQ(freezeCommon->IsSystemEvent("KERNEL_VENDOR", "SCREEN_ON"), false);
    freezeCommon->Init();
    ASSERT_EQ(freezeCommon->IsSystemEvent("KERNEL_VENDOR", "SCREEN_ON"), true);
}

/**
 * @tc.name: FreezeCommon_005
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeCommon_005, TestSize.Level3)
{
    auto freezeCommon = std::make_unique<FreezeCommon>();
    freezeCommon->Init();
    FreezeResult result;
    result.SetId(1);
    ASSERT_EQ(freezeCommon->IsSystemResult(result), true);
    result.SetId(0);
    ASSERT_EQ(freezeCommon->IsSystemResult(result), false);
}

/**
 * @tc.name: FreezeCommon_006
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeCommon_006, TestSize.Level3)
{
    auto freezeCommon = std::make_unique<FreezeCommon>();
    freezeCommon->Init();
    FreezeResult result;
    result.SetId(0);
    ASSERT_EQ(freezeCommon->IsApplicationResult(result), true);
    result.SetId(1);
    ASSERT_EQ(freezeCommon->IsApplicationResult(result), false);
}

/**
 * @tc.name: FreezeCommon_007
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeCommon_007, TestSize.Level3)
{
    auto freezeCommon = std::make_unique<FreezeCommon>();
    freezeCommon->GetPrincipalStringIds();
    freezeCommon->Init();
    freezeCommon->GetPrincipalStringIds();
}

/**
 * @tc.name: FreezeCommon_008
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeCommon_008, TestSize.Level3)
{
    auto freezeCommon = std::make_unique<FreezeCommon>();
    freezeCommon->WriteStartInfoToFd(0, "FreezeCommon_008 test");
    freezeCommon->WriteEndInfoToFd(0, "FreezeCommon_008 test");
    ASSERT_TRUE(freezeCommon != nullptr);
}

/**
 * @tc.name: FreezeWatchPoint_001
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeWatchPoint_001, TestSize.Level3)
{
    WatchPoint point = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR")
        .InitStringId("SCREEN_ON")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .Build();
    auto wp1 = std::make_unique<WatchPoint>(point);
    std::string logPath = "/data/test/test_data/LOG001.log";
    wp1->SetLogPath(logPath);
    ASSERT_EQ(wp1->GetLogPath(), logPath);
}

/**
 * @tc.name: FreezeWatchPoint_002
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeWatchPoint_002, TestSize.Level3)
{
    WatchPoint point = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR")
        .InitStringId("SCREEN_ON")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .Build();
    auto wp1 = std::make_unique<WatchPoint>(point);
    long seq = 1000L;
    wp1->SetSeq(seq);
    ASSERT_EQ(wp1->GetSeq(), seq);
}

/**
 * @tc.name: FreezeWatchPoint_003
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeWatchPoint_003, TestSize.Level3)
{
    WatchPoint watchPoint = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR")
        .InitStringId("SCREEN_ON")
        .InitMsg("Test")
        .InitTimestamp(1687859103947)
        .Build();
    auto wp = std::make_unique<WatchPoint>(watchPoint);

    WatchPoint watchPoint1 = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR")
        .InitStringId("SCREEN_ON")
        .InitTimestamp(1687859103950)
        .Build();
    auto wp1 = std::make_unique<WatchPoint>(watchPoint1);
    bool ret = wp < wp1;
    printf("wp < wp1: %s\n", ret ? "true" : "false");
    ret = wp == wp1;
    printf("wp = wp1: %s\n", ret ? "true" : "false");
    std::string result = wp->GetMsg();
    EXPECT_TRUE(!wp->GetMsg().empty());
    EXPECT_TRUE(wp1->GetMsg().empty());
}

/**
 * @tc.name: FreezeWatchPoint_004
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeWatchPoint_004, TestSize.Level3)
{
    auto wp1 = std::make_unique<WatchPoint>();
    ASSERT_EQ(wp1->GetSeq(), 0);
}

/**
 * @tc.name: FreezeWatchPoint_005
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeWatchPoint_005, TestSize.Level3)
{
    WatchPoint watchPoint = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR")
        .InitStringId("SCREEN_ON")
        .InitTimestamp(1687859103947)
        .InitPid(1000)
        .InitTid(1000)
        .InitUid(1000)
        .InitProcessName("processName")
        .InitPackageName("com.package.name")
        .InitHitraceTime("20230627")
        .InitSysrqTime("20230627")
        .Build();
    auto wp1 = std::make_unique<WatchPoint>(watchPoint);
    ASSERT_EQ(wp1->GetTid(), 1000);
}

/**
 * @tc.name: FreezeDBHelper_001
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeDBHelper_001, TestSize.Level3)
{
    auto freezeCommon = nullptr;
    auto db = std::make_unique<DBHelper>(freezeCommon);
    ASSERT_NE(db, nullptr);
    std::string watchPackage = "com.package.name";
    std::vector<WatchPoint> list;
    WatchPoint watchPoint = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("ACE")
        .InitStringId("UI_BLOCK_3S")
        .InitPackageName(watchPackage)
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .Build();
    list.push_back(watchPoint);
    WatchPoint watchPoint1 = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("ACE")
        .InitStringId("UI_BLOCK_RECOVERED")
        .InitPackageName(watchPackage)
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .Build();
    list.push_back(watchPoint1);
    unsigned long long start = TimeUtil::GetMilliseconds() - 5L;
    unsigned long long end = TimeUtil::GetMilliseconds();
    auto result = FreezeResult(5, "ACE", "UI_BLOCK_3S");
    DBHelper::WatchParams params = {watchPoint.GetPid(), watchPoint.GetPackageName()};
    db->SelectEventFromDB(start, end, list, params, result);
}

/**
 * @tc.name: FreezeDBHelper_002
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeDBHelper_002, TestSize.Level3)
{
    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon->Init();
    ASSERT_EQ(ret1, true);
    auto db = std::make_unique<DBHelper>(freezeCommon);
    ASSERT_NE(db, nullptr);
    std::string watchPackage = "com.package.name";
    std::vector <WatchPoint> list;
    WatchPoint watchPoint = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("ACE")
        .InitStringId("UI_BLOCK_3S")
        .InitPackageName(watchPackage)
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .Build();
    list.push_back(watchPoint);
    WatchPoint watchPoint1 = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("ACE")
        .InitStringId("UI_BLOCK_RECOVERED")
        .InitPackageName(watchPackage)
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .Build();
    list.push_back(watchPoint1);
    unsigned long long start = TimeUtil::GetMilliseconds() + 1000L;
    unsigned long long end = TimeUtil::GetMilliseconds();
    auto result = FreezeResult(5, "ACE", "UI_BLOCK_3S");
    DBHelper::WatchParams params = {watchPoint.GetPid(), watchPoint.GetPackageName()};
    db->SelectEventFromDB(start, end, list, params, result);
}
}
}
