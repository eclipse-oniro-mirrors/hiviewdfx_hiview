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
#include "ffrt.h"
#include "freeze_common.h"
#include "rule_cluster.h"
#include "resolver.h"
#include "vendor.h"
#include "freeze_detector_plugin.h"
#undef private
#include "sys_event.h"
#include "sys_event_dao.h"
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
HWTEST_F(FreezeDetectorUnittest, FreezeResolver_001, TestSize.Level0)
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
 * @tc.name: FreezeResolver_009
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeResolver_009, TestSize.Level3)
{
    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon->Init();
    ASSERT_EQ(ret1, true);
    auto freezeResolver = std::make_unique<FreezeResolver>(freezeCommon);
    ASSERT_EQ(freezeResolver->Init(), true);
    WatchPoint watchPoint = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("FRAMEWORK")
        .InitStringId("SERVICE_WARNING")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .Build();
    std::vector<WatchPoint> list;
    std::vector<FreezeResult> result;
    list.push_back(watchPoint);
    FreezeResult result1;
    FreezeResult result2;
    result.push_back(result1);
    result.push_back(result2);
    EXPECT_TRUE(freezeResolver->JudgmentResult(watchPoint, list,
        result));
}

/**
 * @tc.name: FreezeVender_001
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeVender_001, TestSize.Level0)
{
    auto vendor = std::make_unique<Vendor>(nullptr);
    ASSERT_EQ(vendor->Init(), false);
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
    ASSERT_EQ(vendor->GetTimeString(1687836954734), "20230627113554");
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
 * @tc.name: FreezeVender_005
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeVender_005, TestSize.Level3)
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
        .InitHitraceIdInfo("hitraceId: 123")
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
    vendor->MergeEventLog(watchPoint, list, result);
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
 * @tc.name: FreezeVender_007
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeVender_007, TestSize.Level3)
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
        .InitApplicationInfo("msg:123;1234:1234")
        .InitProcStatm("test 1234")
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
 * @tc.name: FreezeVender_008
 * @tc.desc: FreezeDetector
 */
 HWTEST_F(FreezeDetectorUnittest, FreezeVender_008, TestSize.Level3)
 {
    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret = freezeCommon->Init();
    EXPECT_EQ(ret, true);
    auto vendor = std::make_unique<Vendor>(freezeCommon);
    EXPECT_EQ(vendor->Init(), true);

    std::string processName = "render_service";
    std::string isScbPro;
    vendor->CheckProcessName(processName, isScbPro);
    printf("%s %s\n", processName.c_str(), isScbPro.c_str());
    EXPECT_EQ(processName, "render_service");
    EXPECT_EQ(isScbPro, "No");
    processName = "com.ohos.sceneboard";
    vendor->CheckProcessName(processName, isScbPro);
    printf("%s %s\n", processName.c_str(), isScbPro.c_str());
    EXPECT_EQ(processName, "com.ohos.sceneboard");
    EXPECT_EQ(isScbPro, "No");
    processName = "com.ohos.sceneboard:Enability:2025";
    vendor->CheckProcessName(processName, isScbPro);
    printf("%s %s\n", processName.c_str(), isScbPro.c_str());
    EXPECT_EQ(processName, "Enability");
    EXPECT_EQ(isScbPro, "Yes");
    processName = "com.ohos.sceneboard:   ?!sys/comn/test:123 321";
    vendor->CheckProcessName(processName, isScbPro);
    printf("%s %s\n", processName.c_str(), isScbPro.c_str());
    EXPECT_EQ(processName, "sys_comn_test");
    EXPECT_EQ(isScbPro, "Yes");
}

/**
 * @tc.name: FreezeVender_009
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeVender_009, TestSize.Level3)
{
    WatchPoint watchPoint = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("AAFWK")
        .InitStringId("THREAD_BLOCK_3S")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .InitProcessName("foundation")
        .InitForeGround("No")
        .Build();
    std::vector<WatchPoint> list;
    list.push_back(watchPoint);

    std::vector<FreezeResult> result;
    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon->Init();
    ASSERT_EQ(ret1, true);
    auto vendor = std::make_unique<Vendor>(freezeCommon);
    ASSERT_EQ(vendor->Init(), true);
    vendor->MergeEventLog(watchPoint, list, result).empty();

    WatchPoint watchPoint1 = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("FRAMEWORK")
        .InitStringId("SERVICE_WARNING")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .InitProcessName("foundation")
        .InitForeGround("No")
        .Build();
    list.push_back(watchPoint1);
    ASSERT_TRUE(vendor->MergeEventLog(watchPoint1, list, result).empty());

    WatchPoint watchPoint2 = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("FRAMEWORK")
        .InitStringId("SERVICE_WARNING")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .InitProcessName("FreezeVender_012")
        .InitForeGround("Yes")
        .Build();
    list.push_back(watchPoint2);
    ASSERT_TRUE(vendor->MergeEventLog(watchPoint2, list, result).empty());
}

/**
 * @tc.name: FreezeVender_010
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeVender_010, TestSize.Level3)
{
    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon->Init();
    ASSERT_EQ(ret1, true);
    auto vendor = std::make_unique<Vendor>(freezeCommon);
    ASSERT_EQ(vendor->Init(), true);
    WatchPoint watchPoint1 = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR")
        .InitStringId("SCREEN_ON")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .InitProcessName("processName")
        .InitPackageName("com.package.name")
        .InitMsg("msg")
        .Build();
    std::string ret = vendor->MergeFreezeExtFile(watchPoint1);
    EXPECT_EQ(ret, "");

    std::string freezeExtFile = "/data/test";
    std::string testName = "FreezeDetectorUnittest";
    WatchPoint watchPoint2 = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("AAFWK")
        .InitStringId("THREAD_BLOCK_6S")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .InitProcessName(testName)
        .InitPackageName(testName)
        .InitFreezeExtFile(freezeExtFile)
        .Build();
    vendor->MergeFreezeExtFile(watchPoint2);
}

/**
 * @tc.name: FreezeVender_011
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeVender_011, TestSize.Level3)
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
        .InitEnabelMainThreadSample(true)
        .Build();
    vendor1->SendFaultLog(watchPoint, "test", "sysfreeze", "processName", "No");
}

/**
 * @tc.name: FreezeVender_012
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeVender_012, TestSize.Level3)
{
    auto vendor1 = std::make_unique<Vendor>(nullptr);
    EXPECT_EQ(vendor1->Init(), false);
    WatchPoint watchPoint = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("AAFWK")
        .InitStringId("THREAD_BLOCK_6S")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .InitProcessName("processName")
        .InitPackageName("processName")
        .InitMsg("msg")
        .InitEnabelMainThreadSample(true)
        .InitForeGround("No")
        .Build();
    std::string type;
    std::string pubLogPathName;
    std::string processName;
    std::string isScbPro;
    vendor1->InitLogInfo(watchPoint, type, pubLogPathName, processName, isScbPro);

    type = "";
    WatchPoint watchPoint1 = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("AAFWK")
        .InitStringId("THREAD_BLOCK_6S")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .InitProcessName("processName")
        .InitPackageName("processName")
        .InitForeGround("Yes")
        .InitMsg("msg")
        .InitEnabelMainThreadSample(true)
        .Build();
    vendor1->InitLogInfo(watchPoint1, type, pubLogPathName, processName, isScbPro);
    EXPECT_EQ(type, "");
}

/**
 * @tc.name: FreezeVender_013
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeVender_013, TestSize.Level3)
{
    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon->Init();
    ASSERT_EQ(ret1, true);
    auto vendor = std::make_unique<Vendor>(freezeCommon);
    ASSERT_EQ(vendor->Init(), true);

    WatchPoint watchPoint1 = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("AAFWK")
        .InitStringId("THREAD_BLOCK_6S")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .InitProcessName("processName")
        .InitPackageName("processName")
        .InitForeGround("Yes")
        .InitMsg("msg")
        .InitEnabelMainThreadSample(true)
        .Build();
    std::string type;
    std::string pubLogPathName;
    std::string processName;
    std::string isScbPro;
    vendor->InitLogInfo(watchPoint1, type, pubLogPathName, processName, isScbPro);
    EXPECT_EQ(type, "appfreeze");
}

/**
 * @tc.name: FreezeRuleCluster_001
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeRuleCluster_001, TestSize.Level0)
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
HWTEST_F(FreezeDetectorUnittest, FreezeRule_001, TestSize.Level0)
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
HWTEST_F(FreezeDetectorUnittest, FreezeDetectorPlugin_001, TestSize.Level0)
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
    freezeDetectorPlugin->OnEventListeningCallback(*(event.get()));
    freezeDetectorPlugin->RemoveRedundantNewline("test1\\ntest2\\ntest3");
    WatchPoint watchPoint = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR")
        .InitStringId("SCREEN_ON")
        .InitApplicationInfo("msg:123;1234:1234")
        .Build();
    freezeDetectorPlugin->ProcessEvent(watchPoint);
    ASSERT_EQ(freezeDetectorPlugin->CanProcessEvent(event), false);
}

/**
 * @tc.name: FreezeDetectorPlugin_007
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeDetectorPlugin_007, TestSize.Level3)
{
    auto jsonStr = "{\"domain_\":\"FORM_MANAGER\"}";
    std::string testName = "FreezeDetectorPlugin_007";
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    ASSERT_TRUE(sysEvent != nullptr);
    sysEvent->eventName_ = testName;
    sysEvent->SetSeq(1234567890);
    sysEvent->SetEventValue(FreezeCommon::EVENT_PID, getpid());
    sysEvent->SetEventValue(FreezeCommon::EVENT_TID, getpid());
    sysEvent->SetEventValue(FreezeCommon::EVENT_UID, getuid());
    sysEvent->SetEventValue(FreezeCommon::EVENT_PACKAGE_NAME, testName);
    sysEvent->SetEventValue(FreezeCommon::EVENT_PROCESS_NAME, testName);
    sysEvent->SetEventValue(FreezeCommon::HITRACE_TIME, "12453");
    sysEvent->SetEventValue(FreezeCommon::SYSRQ_TIME, "12453");
    sysEvent->SetEventValue(FreezeCommon::TERMINAL_THREAD_STACK, testName);
    sysEvent->SetEventValue(EventStore::EventCol::INFO, testName);
    auto freezeDetectorPlugin = std::make_unique<FreezeDetectorPlugin>();
    freezeDetectorPlugin->MakeWatchPoint(*(sysEvent.get()));
    sysEvent->SetEventValue(FreezeCommon::EVENT_TRACE_ID, "12345;123456");
    freezeDetectorPlugin->MakeWatchPoint(*(sysEvent.get()));
    ASSERT_TRUE(freezeDetectorPlugin != nullptr);
}

/**
 * @tc.name: FreezeDetectorPlugin_008
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeDetectorPlugin_008, TestSize.Level3)
{
    auto jsonStr = "{\"domain_\":\"FORM_MANAGER\"}";
    std::string testName = "FreezeDetectorPlugin_008";
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    ASSERT_TRUE(sysEvent != nullptr);
    sysEvent->eventName_ = testName;
    sysEvent->SetSeq(1234567890);
    sysEvent->SetEventValue(FreezeCommon::EVENT_PID, getpid());
    sysEvent->SetEventValue(FreezeCommon::EVENT_TID, getpid());
    sysEvent->SetEventValue(FreezeCommon::EVENT_UID, getuid());
    sysEvent->SetEventValue(FreezeCommon::EVENT_PACKAGE_NAME, testName);
    sysEvent->SetEventValue(FreezeCommon::EVENT_PROCESS_NAME, testName);
    sysEvent->SetEventValue(FreezeCommon::SYSRQ_TIME, "12453");
    sysEvent->SetEventValue(EventStore::EventCol::INFO, testName);
    sysEvent->SetEventValue(FreezeCommon::FOREGROUND, true);
    auto freezeDetectorPlugin = std::make_unique<FreezeDetectorPlugin>();
    freezeDetectorPlugin->MakeWatchPoint(*(sysEvent.get()));
    ASSERT_TRUE(freezeDetectorPlugin != nullptr);
}

/**
 * @tc.name: FreezeCommon_001
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeCommon_001, TestSize.Level0)
{
    auto freezeCommon = std::make_unique<FreezeCommon>();
    ASSERT_EQ(freezeCommon->Init(), true);
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
    ASSERT_EQ(freezeCommon->IsFreezeEvent("AAFWK", "THREAD_BLOCK_3S"), true);
    ASSERT_EQ(freezeCommon->IsFreezeEvent("AAFWK1", "THREAD_BLOCK_3S"), false);
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
    ASSERT_TRUE(freezeCommon != nullptr);
    freezeCommon->GetPrincipalStringIds();
    freezeCommon->Init();
    freezeCommon->GetPrincipalStringIds();
}

/**
 * @tc.name: FreezeCommon_006
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeCommon_006, TestSize.Level3)
{
    auto freezeCommon = std::make_unique<FreezeCommon>();
    freezeCommon->WriteTimeInfoToFd(0, "FreezeCommon_006 test");
    freezeCommon->WriteTimeInfoToFd(0, "FreezeCommon_006 test", false);
    ASSERT_TRUE(freezeCommon != nullptr);
}

/**
 * @tc.name: FreezeWatchPoint_001
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeWatchPoint_001, TestSize.Level0)
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
        .InitTerminalThreadStack("threadStaskTest")
        .InitTelemetryId("telemetryIdTest")
        .InitHitraceIdInfo("hitraceId: 123")
        .InitProcStatm("123 45 678")
        .InitHostResourceWarning("Yes")
        .InitAppRunningUniqueId("20250924")
        .InitTaskName("testTaskName")
        .InitClusterRaw("testClusterRaw")
        .Build();
    auto wp1 = std::make_unique<WatchPoint>(watchPoint);
    ASSERT_EQ(wp1->GetTid(), 1000);
    ASSERT_EQ(wp1->GetTerminalThreadStack(), "threadStaskTest");
    ASSERT_EQ(wp1->GetTelemetryId(), "telemetryIdTest");
    ASSERT_EQ(wp1->GetProcStatm(), "123 45 678");
    ASSERT_EQ(wp1->GetHostResourceWarning(), "Yes");
    ASSERT_EQ(wp1->GetAppRunningUniqueId(), "20250924");
    ASSERT_EQ(wp1->GetTaskName(), "testTaskName");
    ASSERT_EQ(wp1->GetClusterRaw(), "testClusterRaw");
}

/**
 * @tc.name: FreezeWatchPoint_006
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeWatchPoint_006, TestSize.Level0)
{
    std::string logPath = "/data/log/testLog";
    WatchPoint point = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR")
        .InitStringId("SCREEN_ON")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .InitFreezeExtFile(logPath)
        .Build();
    EXPECT_EQ(point.GetFreezeExtFile(), logPath);
}

/**
 * @tc.name: FreezeWatchPoint_007
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeWatchPoint_007, TestSize.Level0)
{
    std::string logPath = "/data/log/testLog111";
    WatchPoint point = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitDomain("KERNEL_VENDOR")
        .InitStringId("SCREEN_ON")
        .InitTimestamp(TimeUtil::GetMilliseconds())
        .InitFreezeExtFile(logPath)
        .Build();
    point.SetFreezeExtFile(logPath);
    EXPECT_EQ(point.GetFreezeExtFile(), logPath);
}

/**
 * @tc.name: FreezeWatchPoint_008
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeWatchPoint_008, TestSize.Level3)
{
    WatchPoint point = OHOS::HiviewDFX::WatchPoint::Builder()
        .InitTimeoutEventId("123454")
        .InitLastDispatchEventId("812434")
        .InitLastProcessEventId("125374")
        .InitLastMarkedEventId("751234")
        .InitThermalLevel("5")
        .Build();
    auto wp1 = std::make_unique<WatchPoint>(point);
    ASSERT_EQ(wp1->GetTimeoutEventId(), "123454");
    ASSERT_EQ(wp1->GetLastDispatchEventId(), "812434");
    ASSERT_EQ(wp1->GetLastProcessEventId(), "125374");
    ASSERT_EQ(wp1->GetLastMarkedEventId(), "751234");
    ASSERT_EQ(wp1->GetThermalLevel(), "5");

    wp1->SetTimeoutEventId("123");
    wp1->SetLastDispatchEventId("1234");
    wp1->SetLastProcessEventId("12345");
    wp1->SetLastMarkedEventId("123456");
    wp1->SetThermalLevel("1");

    ASSERT_EQ(wp1->GetTimeoutEventId(), "123");
    ASSERT_EQ(wp1->GetLastDispatchEventId(), "1234");
    ASSERT_EQ(wp1->GetLastProcessEventId(), "12345");
    ASSERT_EQ(wp1->GetLastMarkedEventId(), "123456");
    ASSERT_EQ(wp1->GetThermalLevel(), "1");
}

/**
 * @tc.name: FreezeDBHelper_001
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeDBHelper_001, TestSize.Level0)
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
    DBHelper::WatchParams params = {watchPoint.GetPid(), 0, watchPoint.GetTimestamp(), watchPoint.GetPackageName()};
    db->SelectEventFromDB(start, end, list, params, result);
    std::vector<std::string> eventNames;
    eventNames.push_back("UI_BLOCK_3S");
    eventNames.push_back("UI_BLOCK_6S");
    db->SelectRecords(start, end, "ACE", eventNames);
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
    DBHelper::WatchParams params = {watchPoint.GetPid(), 0, watchPoint.GetTimestamp(), watchPoint.GetPackageName()};
    db->SelectEventFromDB(start, end, list, params, result);
}

/**
 * @tc.name: FreezeDBHelper_003
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeDBHelper_003, TestSize.Level3)
{
    std::string jsonStr = R"~({"domain_":"AAFWK", "name_":"THREAD_BLOCK_3S", "type_":1, "time_":1501973701070, "tz_":
    "+0800", "pid_":12000, "tid_":12000, "uid_":0, "FAULT_TYPE":"4", "PID":12000, "UID":0,
    "MODULE":"FreezeDetectorUnittest", "REASON":"unittest for FreezeDetectorUnittest",
    "SUMMARY":"summary for FreezeDBHelper", "LOG_PATH":"/data/log/test", "VERSION":"",
    "HAPPEN_TIME":"1501973701", "PNAME":"/", "FIRST_FRAME":"/", "SECOND_FRAME":"/", "LAST_FRAME":"/", "FINGERPRINT":
    "04c0d6f03c73da531f00eb112479a8a2f19f59fafba6a474dcbe455a13288f4d", "level_":"CRITICAL", "tag_":"STABILITY", "id_":
    "17165544771317691984", "info_":""})~";
    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, jsonStr);
    sysEvent->SetLevel("MINOR");
    sysEvent->SetEventSeq(447); // 447: test seq
    EventStore::SysEventDao::Insert(sysEvent);

    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon->Init();
    ASSERT_EQ(ret1, true);
    auto db = std::make_unique<DBHelper>(freezeCommon);
    ASSERT_NE(db, nullptr);
    std::vector <WatchPoint> list;
    unsigned long long start = 1501973701070;
    unsigned long long end = 1501973701170;
    DBHelper::WatchParams params = {12000, 0, start, "FreezeDetectorUnittest"};
    auto result = FreezeResult(-14, "AAFWK", "THREAD_BLOCK_3S");
    db->SelectEventFromDB(start, end, list, params, result);
    auto result1 = FreezeResult(-6, "ACE", "UI_BLOCK_3S");
    db->SelectEventFromDB(start, end, list, params, result1);
    params = {10000, 10000, start, "FreezeDetectorUnittest"};
    auto result2 = FreezeResult(-10, "AAFWK", "THREAD_BLOCK_3S");
    result2.SetSamePackage("true");
    db->SelectEventFromDB(start, end, list, params, result2);
    ASSERT_TRUE(list.size() > 0);
}

/**
 * @tc.name: FreezeDBHelper_004
 * @tc.desc: FreezeDetector
 */
HWTEST_F(FreezeDetectorUnittest, FreezeDBHelper_004, TestSize.Level3)
{
    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool ret1 = freezeCommon->Init();
    ASSERT_EQ(ret1, true);
    auto db = std::make_unique<DBHelper>(freezeCommon);
    ASSERT_NE(db, nullptr);
    unsigned long long start = 1501973701070;
    unsigned long long end = 1501973701170;
    std::string domain = "AAFWK";
    std::vector<std::string> eventNames;
    eventNames.push_back("UI_BLOCK_3S");
    eventNames.push_back("THREAD_BLOCK_3S");
    eventNames.push_back("THREAD_BLOCK_6S");
    std::vector<SysEvent> result = db->SelectRecords(start, end, domain, eventNames);
    ASSERT_TRUE(result.size() > 0);
}
}
}
