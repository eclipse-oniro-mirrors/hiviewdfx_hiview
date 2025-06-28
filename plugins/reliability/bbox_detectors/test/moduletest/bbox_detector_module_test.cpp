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
#include "bbox_detector_module_test.h"

#include "bbox_detector_plugin.h"

#include "bbox_detectors_mock.h"
#include "common_defines.h"
#include "event.h"
#include "file_util.h"
#include "hisysevent_util_mock.h"
#include "log_util.h"
#include "string_util.h"
#include "sys_event.h"
#include "time_util.h"

using namespace std;
namespace OHOS {
namespace HiviewDFX {
using namespace testing;
using namespace testing::ext;
void BBoxDetectorModuleTest::SetUpTestCase(void) {}

void BBoxDetectorModuleTest::TearDownTestCase(void) {}

void BBoxDetectorModuleTest::SetUp(void)
{
    EXPECT_CALL(MockHisyseventUtil::GetInstance(), IsEventProcessed).WillRepeatedly(Return(false));
}

void BBoxDetectorModuleTest::TearDown(void) {}

/**
 * @tc.name: BBoxDetectorModuleTest001
 * @tc.desc: check whether fault is processed.
 *           1. check whether event is valid;
 *           2. check whether category and reason is ignored;
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST_F(BBoxDetectorModuleTest, BBoxDetectorModuleTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. construct PANIC SysEvent
     * @tc.steps: step2. construct BBOXDetectorPlugin
     * @tc.steps: step3. OnEvent
     * @tc.steps: step4. check result
     */
    SysEventCreator sysEventCreator("KERNEL_VENDOR", "PANIC", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("SUMMARY", "bootup_keypoint:97");
    sysEventCreator.SetKeyValue("name_", "PANIC");
    sysEventCreator.SetKeyValue("HAPPEN_TIME", "443990995");
    sysEventCreator.SetKeyValue("LOG_PATH", "/data/test/hisi_logs/");
    sysEventCreator.SetKeyValue("SUB_LOG_PATH", "19700106031950-00001111");
    sysEventCreator.SetKeyValue("MODULE", "AP");
    sysEventCreator.SetKeyValue("REASON", "AP_S_PANIC");

    FileUtil::ForceCreateDirectory("/data/test/hisi_logs/19700106031950-00001111/");
    FileUtil::SaveStringToFile("/data/test/hisi_logs/19700106031950-00001111/DONE", "done", true);
    auto sysEvent = make_shared<SysEvent>("test", nullptr, sysEventCreator);
    auto testPlugin = make_shared<BBoxDetectorPlugin>();
    MockHiviewContext hiviewContext;
    auto eventLoop = std::make_shared<MockEventLoop>();
    EXPECT_CALL(*(eventLoop.get()), GetMockInterval()).WillRepeatedly(Return(1));
    EXPECT_CALL(hiviewContext, GetSharedWorkLoop()).WillRepeatedly(Return(eventLoop));
    testPlugin->SetHiviewContext(&hiviewContext);
    shared_ptr<Event> event = dynamic_pointer_cast<Event>(sysEvent);
    testPlugin->OnLoad();
    testPlugin->OnEvent(event);
    ASSERT_EQ(sysEvent->GetEventValue("MODULE"), "AP");
    ASSERT_EQ(sysEvent->GetEventValue("REASON"), "AP_S_PANIC");
    ASSERT_EQ(sysEvent->GetEventValue("LOG_PATH"), "/data/test/hisi_logs/19700106031950-00001111");
}

/**
 * @tc.name: BBoxDetectorModuleTest002
 * @tc.desc: check whether fault is processed.
 *           1. check whether event is valid;
 *           2. check whether category and reason is ignored;
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST_F(BBoxDetectorModuleTest, BBoxDetectorModuleTest002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. construct HWWATCHDOG SysEvent
     * @tc.steps: step2. construct BBOXDetectorPlugin
     * @tc.steps: step3. OnEvent
     * @tc.steps: step4. check result
     */
    SysEventCreator sysEventCreator("KERNEL_VENDOR", "HWWATCHDOG", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("SUMMARY", "bootup_keypoint:97");
    sysEventCreator.SetKeyValue("name_", "HWWATCHDOG");
    sysEventCreator.SetKeyValue("HAPPEN_TIME", "443990995");
    sysEventCreator.SetKeyValue("LOG_PATH", "/data/test/hisi_logs/");
    sysEventCreator.SetKeyValue("SUB_LOG_PATH", "19700106031950-00001111");
    sysEventCreator.SetKeyValue("MODULE", "AP");
    sysEventCreator.SetKeyValue("REASON", "AP_S_HWWATCHDOG");

    FileUtil::ForceCreateDirectory("/data/test/hisi_logs/19700106031950-00001111/");
    FileUtil::SaveStringToFile("/data/test/hisi_logs/19700106031950-00001111/DONE", "done", true);
    auto sysEvent = make_shared<SysEvent>("test", nullptr, sysEventCreator);
    auto testPlugin = make_shared<BBoxDetectorPlugin>();
    MockHiviewContext hiviewContext;
    auto eventLoop = std::make_shared<MockEventLoop>();
    EXPECT_CALL(*(eventLoop.get()), GetMockInterval()).WillRepeatedly(Return(1));
    EXPECT_CALL(hiviewContext, GetSharedWorkLoop()).WillRepeatedly(Return(eventLoop));
    testPlugin->SetHiviewContext(&hiviewContext);
    shared_ptr<Event> event = dynamic_pointer_cast<Event>(sysEvent);
    testPlugin->OnLoad();
    testPlugin->OnEvent(event);
    ASSERT_EQ(sysEvent->GetEventValue("MODULE"), "AP");
    ASSERT_EQ(sysEvent->GetEventValue("REASON"), "AP_S_HWWATCHDOG");
    ASSERT_EQ(sysEvent->GetEventValue("LOG_PATH"), "/data/test/hisi_logs/19700106031950-00001111");
}

/**
 * @tc.name: BBoxDetectorModuleTest003
 * @tc.desc: check whether fault is processed.
 *           1. check whether event is valid;
 *           2. check whether category and reason is ignored;
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST_F(BBoxDetectorModuleTest, BBoxDetectorModuleTest003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. construct HWWATCHDOG SysEvent
     * @tc.steps: step2. construct BBOXDetectorPlugin
     * @tc.steps: step3. OnEvent
     * @tc.steps: step4. check result
     */
    SysEventCreator sysEventCreator("KERNEL_VENDOR", "MODEMCRASH", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("SUMMARY", "bootup_keypoint:97");
    sysEventCreator.SetKeyValue("name_", "MODEMCRASH");
    sysEventCreator.SetKeyValue("HAPPEN_TIME", "443990995");
    sysEventCreator.SetKeyValue("LOG_PATH", "/data/test/hisi_logs/");
    sysEventCreator.SetKeyValue("SUB_LOG_PATH", "19700106031950-00001111");
    sysEventCreator.SetKeyValue("MODULE", "AP");
    sysEventCreator.SetKeyValue("REASON", "MODEMCRASH");

    FileUtil::ForceCreateDirectory("/data/test/hisi_logs/19700106031950-00001111/");
    FileUtil::SaveStringToFile("/data/test/hisi_logs/19700106031950-00001111/DONE", "done", true);
    auto sysEvent = make_shared<SysEvent>("test", nullptr, sysEventCreator);
    auto testPlugin = make_shared<BBoxDetectorPlugin>();
    MockHiviewContext hiviewContext;
    auto eventLoop = std::make_shared<MockEventLoop>();
    EXPECT_CALL(*(eventLoop.get()), GetMockInterval()).WillRepeatedly(Return(1));
    EXPECT_CALL(hiviewContext, GetSharedWorkLoop()).WillRepeatedly(Return(eventLoop));
    testPlugin->SetHiviewContext(&hiviewContext);
    shared_ptr<Event> event = dynamic_pointer_cast<Event>(sysEvent);
    testPlugin->OnLoad();
    testPlugin->OnEvent(event);
    ASSERT_EQ(sysEvent->GetEventValue("MODULE"), "AP");
    ASSERT_EQ(sysEvent->GetEventValue("REASON"), "MODEMCRASH");
    ASSERT_EQ(sysEvent->GetEventValue("LOG_PATH"), "/data/test/hisi_logs/19700106031950-00001111");
}

/**
 * @tc.name: BBoxDetectorModuleTest004
 * @tc.desc: check whether fault is processed.
 *           1. check whether event is invalid;
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST_F(BBoxDetectorModuleTest, BBoxDetectorModuleTest004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. construct PANIC SysEvent
     * @tc.steps: step2. construct BBOXDetectorPlugin
     * @tc.steps: step3. OnEvent
     * @tc.steps: step4. check result should return true
     */
    SysEventCreator sysEventCreator("KERNEL_VENDOR", "PANIC", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("SUMMARY", "bootup_keypoint:97");
    sysEventCreator.SetKeyValue("name_", "PANIC");
    sysEventCreator.SetKeyValue("HAPPEN_TIME", "443990995");
    sysEventCreator.SetKeyValue("LOG_PATH", "/data/test/hisi_logs/");
    sysEventCreator.SetKeyValue("SUB_LOG_PATH", "19700106031950-00001111");
    sysEventCreator.SetKeyValue("MODULE", "AP");
    sysEventCreator.SetKeyValue("REASON", "MODEMCRASH");

    FileUtil::ForceCreateDirectory("/data/test/hisi_logs/19700106031950-00001111/");
    FileUtil::SaveStringToFile("/data/test/hisi_logs/19700106031950-00001111/DONE", "done", true);
    auto sysEvent = make_shared<SysEvent>("test", nullptr, sysEventCreator);
    auto testPlugin = make_shared<BBoxDetectorPlugin>();
    MockHiviewContext hiviewContext;
    auto eventLoop = std::make_shared<MockEventLoop>();
    EXPECT_CALL(*(eventLoop.get()), GetMockInterval()).WillRepeatedly(Return(1));
    EXPECT_CALL(hiviewContext, GetSharedWorkLoop()).WillRepeatedly(Return(eventLoop));
    testPlugin->SetHiviewContext(&hiviewContext);
    shared_ptr<Event> event = dynamic_pointer_cast<Event>(sysEvent);
    testPlugin->OnLoad();
    ASSERT_EQ(testPlugin->OnEvent(event), true);
}

/**
 * @tc.name: BBoxDetectorModuleTest005
 * @tc.desc: check event is null.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(BBoxDetectorModuleTest, BBoxDetectorModuleTest005, TestSize.Level1)
{
    auto testPlugin = make_shared<BBoxDetectorPlugin>();
    MockHiviewContext hiviewContext;
    auto eventLoop = std::make_shared<MockEventLoop>();
    EXPECT_CALL(*(eventLoop.get()), GetMockInterval()).WillRepeatedly(Return(1));
    EXPECT_CALL(hiviewContext, GetSharedWorkLoop()).WillRepeatedly(Return(eventLoop));
    testPlugin->SetHiviewContext(&hiviewContext);
    shared_ptr<Event> event = nullptr;
    testPlugin->OnLoad();
    ASSERT_EQ(testPlugin->OnEvent(event), false);
}

/**
 * @tc.name: BBoxDetectorModuleTest006
 * @tc.desc: check domain is not KERNEL_VENDOR
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(BBoxDetectorModuleTest, BBoxDetectorModuleTest006, TestSize.Level1)
{
    /**
     * @tc.steps: step1. construct PANIC SysEvent
     * @tc.steps: step2. construct BBOXDetectorPlugin
     * @tc.steps: step3. OnEvent
     * @tc.steps: step4. check result should return true
     */
    SysEventCreator sysEventCreator("TEST", "PANIC", SysEventCreator::FAULT);
    auto sysEvent = make_shared<SysEvent>("test", nullptr, sysEventCreator);
    auto testPlugin = make_shared<BBoxDetectorPlugin>();
    MockHiviewContext hiviewContext;
    auto eventLoop = std::make_shared<MockEventLoop>();
    EXPECT_CALL(*(eventLoop.get()), GetMockInterval()).WillRepeatedly(Return(1));
    EXPECT_CALL(hiviewContext, GetSharedWorkLoop()).WillRepeatedly(Return(eventLoop));
    testPlugin->SetHiviewContext(&hiviewContext);
    shared_ptr<Event> event = dynamic_pointer_cast<Event>(sysEvent);
    testPlugin->OnLoad();
    ASSERT_EQ(testPlugin->OnEvent(event), false);
    testPlugin->OnUnload();
}

/**
 * @tc.name: BBoxDetectorModuleTest007
 * @tc.desc: test CUSTOM
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(BBoxDetectorModuleTest, BBoxDetectorModuleTest007, TestSize.Level1)
{
    /**
     * @tc.steps: step1. construct CUSTOM SysEvent
     * @tc.steps: step2. construct BBOXDetectorPlugin
     * @tc.steps: step3. OnEvent
     * @tc.steps: step4. check result should return true
     */
    SysEventCreator sysEventCreator("KERNEL_VENDOR", "CUSTOM", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("SUMMARY", "bootup_keypoint:97");
    sysEventCreator.SetKeyValue("HAPPEN_TIME", "443990995");
    sysEventCreator.SetKeyValue("BBOX_TIME", "443990995");
    sysEventCreator.SetKeyValue("BBOX_SYSRESET", "443990995");
    sysEventCreator.SetKeyValue("LOG_PATH", "/data/test/hisi_logs/");
    sysEventCreator.SetKeyValue("SUB_LOG_PATH", "19700106031950-00001111");
    sysEventCreator.SetKeyValue("MODULE", "AP");
    sysEventCreator.SetKeyValue("REASON", "MODEMCRASH");

    FileUtil::ForceCreateDirectory("/data/test/hisi_logs/19700106031950-00001111/");
    FileUtil::SaveStringToFile("/data/test/hisi_logs/19700106031950-00001111/DONE", "done", true);
    auto sysEvent = make_shared<SysEvent>("test", nullptr, sysEventCreator);
    auto testPlugin = make_shared<BBoxDetectorPlugin>();
    MockHiviewContext hiviewContext;
    auto eventLoop = std::make_shared<MockEventLoop>();
    EXPECT_CALL(*(eventLoop.get()), GetMockInterval()).WillRepeatedly(Return(1));
    EXPECT_CALL(hiviewContext, GetSharedWorkLoop()).WillRepeatedly(Return(eventLoop));
    testPlugin->SetHiviewContext(&hiviewContext);
    shared_ptr<Event> event = dynamic_pointer_cast<Event>(sysEvent);
    testPlugin->OnLoad();

    ASSERT_EQ(testPlugin->OnEvent(event), true);
    testPlugin->OnUnload();
}

/**
 * @tc.name: BBoxDetectorModuleTest008
 * @tc.desc: check event is recovery panic
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(BBoxDetectorModuleTest, BBoxDetectorModuleTest008, TestSize.Level1)
{
    /**
     * @tc.steps: step1. construct PANIC" SysEvent
     * @tc.steps: step2. construct BBOXDetectorPlugin
     * @tc.steps: step3. OnEvent
     * @tc.steps: step4. check result should return true
     */
    SysEventCreator sysEventCreator("KERNEL_VENDOR", "PANIC", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("SUMMARY", "bootup_keypoint:97");
    sysEventCreator.SetKeyValue("HAPPEN_TIME", "443990995");
    sysEventCreator.SetKeyValue("LOG_PATH", "/data/test/bbox/panic_log/");
    sysEventCreator.SetKeyValue("SUB_LOG_PATH", "19700106031950-00001111");
    sysEventCreator.SetKeyValue("MODULE", "AP");
    sysEventCreator.SetKeyValue("REASON", "HM_PANIC:HM_PANIC_SYSMGR");

    auto sysEvent = make_shared<SysEvent>("test", nullptr, sysEventCreator);
    auto testPlugin = make_shared<BBoxDetectorPlugin>();
    MockHiviewContext hiviewContext;
    auto eventLoop = std::make_shared<MockEventLoop>();
    EXPECT_CALL(*(eventLoop.get()), GetMockInterval()).WillRepeatedly(Return(1));
    EXPECT_CALL(hiviewContext, GetSharedWorkLoop()).WillRepeatedly(Return(eventLoop));
    testPlugin->SetHiviewContext(&hiviewContext);
    shared_ptr<Event> event = dynamic_pointer_cast<Event>(sysEvent);
    testPlugin->OnLoad();

    ASSERT_EQ(testPlugin->OnEvent(event), true);
    testPlugin->OnUnload();
    ASSERT_TRUE(sysEvent->GetEventValue("FIRST_FRAME").empty());
    ASSERT_TRUE(sysEvent->GetEventValue("SECOND_FRAME").empty());
    ASSERT_TRUE(sysEvent->GetEventValue("LAST_FRAME").empty());
}

/**
 * @tc.name: BBoxDetectorModuleTest009
 * @tc.desc: The event has been processed and completed
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(BBoxDetectorModuleTest, BBoxDetectorModuleTest009, TestSize.Level1)
{
    /**
     * @tc.steps: step1. construct PANIC SysEvent
     * @tc.steps: step2. construct BBOXDetectorPlugin
     * @tc.steps: step3. OnEvent
     * @tc.steps: step4. check result should return true
     */
    SysEventCreator sysEventCreator("KERNEL_VENDOR", "PANIC", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("HAPPEN_TIME", "443990995");
    sysEventCreator.SetKeyValue("LOG_PATH", "/data/test/hisi_logs/");
    sysEventCreator.SetKeyValue("SUB_LOG_PATH", "19700106031950-00001111");
    sysEventCreator.SetKeyValue("MODULE", "AP");
    sysEventCreator.SetKeyValue("REASON", "HM_PANIC:HM_PANIC_SYSMGR");

    auto sysEvent = make_shared<SysEvent>("test", nullptr, sysEventCreator);
    auto testPlugin = make_shared<BBoxDetectorPlugin>();
    MockHiviewContext hiviewContext;
    auto eventLoop = std::make_shared<MockEventLoop>();
    EXPECT_CALL(*(eventLoop.get()), GetMockInterval()).WillRepeatedly(Return(1));
    EXPECT_CALL(hiviewContext, GetSharedWorkLoop()).WillRepeatedly(Return(eventLoop));
    EXPECT_CALL(MockHisyseventUtil::GetInstance(), IsEventProcessed).WillRepeatedly(Return(true));
    testPlugin->SetHiviewContext(&hiviewContext);
    shared_ptr<Event> event = dynamic_pointer_cast<Event>(sysEvent);
    testPlugin->OnLoad();

    ASSERT_EQ(testPlugin->OnEvent(event), true);
    testPlugin->OnUnload();
    ASSERT_TRUE(sysEvent->GetEventValue("FIRST_FRAME").empty());
    ASSERT_TRUE(sysEvent->GetEventValue("SECOND_FRAME").empty());
    ASSERT_TRUE(sysEvent->GetEventValue("LAST_FRAME").empty());
}

/**
 * @tc.name: BBoxDetectorModuleTest010
 * @tc.desc: check BFM_S_NATIVE_DATA_FAIL
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(BBoxDetectorModuleTest, BBoxDetectorModuleTest010, TestSize.Level1)
{
    /**
     * @tc.steps: step1. construct PANIC SysEvent
     * @tc.steps: step2. construct BBOXDetectorPlugin
     * @tc.steps: step3. OnEvent
     * @tc.steps: step4. check result should return true
     */
    SysEventCreator sysEventCreator("KERNEL_VENDOR", "PANIC", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("MODULE", "AP");
    sysEventCreator.SetKeyValue("REASON", "BFM_S_NATIVE_DATA_FAIL");

    auto sysEvent = make_shared<SysEvent>("test", nullptr, sysEventCreator);
    auto testPlugin = make_shared<BBoxDetectorPlugin>();
    MockHiviewContext hiviewContext;
    auto eventLoop = std::make_shared<MockEventLoop>();
    EXPECT_CALL(*(eventLoop.get()), GetMockInterval()).WillRepeatedly(Return(1));
    EXPECT_CALL(hiviewContext, GetSharedWorkLoop()).WillRepeatedly(Return(eventLoop));
    testPlugin->SetHiviewContext(&hiviewContext);
    shared_ptr<Event> event = dynamic_pointer_cast<Event>(sysEvent);
    testPlugin->OnLoad();

    ASSERT_EQ(testPlugin->OnEvent(event), true);
    ASSERT_TRUE(event->HasFinish());
}
}  // namespace HiviewDFX
}  // namespace OHOS
