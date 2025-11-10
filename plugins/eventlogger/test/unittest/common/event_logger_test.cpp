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
#include "event_logger_test.h"

#include <fcntl.h>
#include "common_utils.h"
#include "hisysevent.h"
#include "hiview_platform.h"

#define private public
#include "event_logger.h"
#include "freeze_manager.h"
#undef private
#include "event.h"
#include "hiview_platform.h"
#include "sysevent_source.h"
#ifdef WINDOW_MANAGER_ENABLE
#include "focus_change_info.h"
#include "event_focus_listener.h"
#endif
#include "time_util.h"
#include "eventlogger_util_test.h"
#include "parameters.h"
#include "db_helper.h"
#include "freeze_common.h"
#include "event_logger_util.h"
#include "event_log_task.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace HiviewDFX {
SysEventSource source;
static std::string TEST_PATH = "/data/test/log/test.txt";
void EventLoggerTest::SetUp()
{
    printf("SetUp.\n");
    InitSeLinuxEnabled();
}

void EventLoggerTest::TearDown()
{
    printf("TearDown.\n");
    CancelSeLinuxEnabled();
}

void EventLoggerTest::SetUpTestCase()
{
    HiviewPlatform platform;
    source.SetHiviewContext(&platform);
    source.OnLoad();
}

void EventLoggerTest::TearDownTestCase()
{
    source.OnUnload();
}

/**
 * @tc.name: EventLoggerTest_OnEvent_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_OnEvent_001, TestSize.Level0)
{
    auto eventLogger = std::make_shared<EventLogger>();
    std::shared_ptr<Event> event = nullptr;
    EXPECT_FALSE(eventLogger->OnEvent(event));
}

/**
 * @tc.name: EventLoggerTest_OnEvent_002
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_OnEvent_002, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::shared_ptr<SysEvent> sysEvent1 = std::make_shared<SysEvent>("GESTURE_NAVIGATION_BACK",
        nullptr, jsonStr);
    sysEvent1->eventName_ = "GESTURE_NAVIGATION_BACK";
    sysEvent1->SetEventValue("PID", getpid());
    std::shared_ptr<OHOS::HiviewDFX::Event> event1 = std::static_pointer_cast<Event>(sysEvent1);
    EXPECT_EQ(eventLogger->OnEvent(event1), true);
#ifdef WINDOW_MANAGER_ENABLE
    sptr<Rosen::FocusChangeInfo> focusChangeInfo;
    sptr<EventFocusListener> eventFocusListener_ = EventFocusListener::GetInstance();
    eventFocusListener_->OnFocused(focusChangeInfo);
    eventFocusListener_->OnUnfocused(focusChangeInfo);
    EventFocusListener::registerState_ = EventFocusListener::REGISTERED;
    EXPECT_EQ(eventLogger->OnEvent(event1), true);
#endif
}

/**
 * @tc.name: EventLoggerTest_OnEvent_003
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_OnEvent_003, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::string testName = "EventLoggerTest_OnEvent_003";
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    EXPECT_EQ(eventLogger->IsHandleAppfreeze(sysEvent), true);
    sysEvent->SetEventValue("PACKAGE_NAME", testName);
    sysEvent->SetEventValue("PID", 0);
    sysEvent->SetEventValue("eventLog_action", "");
    std::shared_ptr<OHOS::HiviewDFX::Event> event = std::static_pointer_cast<Event>(sysEvent);
    EXPECT_EQ(eventLogger->OnEvent(event), true);
    sysEvent->eventName_ = "THREAD_BLOCK_6S";
    event = std::static_pointer_cast<Event>(sysEvent);
    sysEvent->SetValue("eventLog_action", "pb:1");
    EXPECT_EQ(eventLogger->OnEvent(event), true);
    OHOS::system::SetParameter("hiviewdfx.appfreeze.filter_bundle_name", testName);
    EXPECT_FALSE(eventLogger->IsHandleAppfreeze(sysEvent));
    event = std::static_pointer_cast<Event>(sysEvent);
    EXPECT_EQ(eventLogger->OnEvent(event), true);
    OHOS::system::SetParameter("hiviewdfx.appfreeze.filter_bundle_name", "test");
}

/**
 * @tc.name: EventLoggerTest_OnEvent_004
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_OnEvent_004, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    eventLogger->OnLoad();
    sleep(1);

    auto jsonStr = "{\"domain_\":\"FORM_MANAGER\"}";
    long pid = getpid();
#ifdef WINDOW_MANAGER_ENABLE
    EventFocusListener::RegisterFocusListener();
    EventFocusListener::registerState_ = EventFocusListener::REGISTERED;
#endif
    uint64_t curentTime = TimeUtil::GetMilliseconds();
    for (int i = 0; i < 5 ; i++) {
        std::shared_ptr<SysEvent> sysEvent1 = std::make_shared<SysEvent>("GESTURE_NAVIGATION_BACK",
            nullptr, jsonStr);
        sysEvent1->SetEventValue("PID", pid);
        sysEvent1->happenTime_ = curentTime;
        std::shared_ptr<OHOS::HiviewDFX::Event> event1 = std::static_pointer_cast<Event>(sysEvent1);
        EXPECT_EQ(eventLogger->OnEvent(event1), true);
        usleep(200 * 1000);
        curentTime += 200;
    }
    std::shared_ptr<SysEvent> sysEvent2 = std::make_shared<SysEvent>("FREQUENT_CLICK_WARNING",
        nullptr, jsonStr);
    sysEvent2->SetEventValue("PID", pid);
    sysEvent2->happenTime_ = TimeUtil::GetMilliseconds();
    std::shared_ptr<OHOS::HiviewDFX::Event> event2 = std::static_pointer_cast<Event>(sysEvent2);
    EXPECT_EQ(eventLogger->OnEvent(event2), true);
    eventLogger->OnUnload();
}

/**
 * @tc.name: EventLoggerTest_IsInterestedPipelineEvent_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_IsInterestedPipelineEvent_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    EXPECT_FALSE(eventLogger->IsInterestedPipelineEvent(nullptr));
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::string testName = "EventLoggerTest_002";
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    sysEvent->eventId_ = 1000001;
    EXPECT_FALSE(eventLogger->IsInterestedPipelineEvent(sysEvent));
    sysEvent->eventId_ = 1;
    sysEvent->eventName_ = "UninterestedEvent";
    EXPECT_FALSE(eventLogger->IsInterestedPipelineEvent(sysEvent));
    sysEvent->eventName_ = "InterestedEvent";
    eventLogger->eventLoggerConfig_[sysEvent->eventName_] =
        EventLoggerConfig::EventLoggerConfigData();
    EXPECT_TRUE(eventLogger->IsInterestedPipelineEvent(sysEvent));
}

/**
 * @tc.name: EventLoggerTest_CheckProcessRepeatFreeze_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_CheckProcessRepeatFreeze_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    long pid = getprocpid();
    eventLogger->lastPid_ = pid;
    bool ret = eventLogger->CheckProcessRepeatFreeze("THREAD_BLOCK_6S", pid);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: EventLoggerTest_WriteCommonHead_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_WriteCommonHead_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::string testName = "EventLoggerTest_002";
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    sysEvent->SetEventValue("EVENTNAME", testName);
    sysEvent->SetEventValue("MODULE_NAME", testName);
    sysEvent->SetEventValue("PACKAGE_NAME", testName);
    sysEvent->SetEventValue("PROCESS_NAME", testName);
    sysEvent->SetEventValue("eventLog_action", "pb:1");
    sysEvent->SetEventValue("eventLog_interval", 1);
    sysEvent->SetEventValue("STACK", "TEST\\nTEST\\nTEST");
    sysEvent->SetEventValue("MSG", "TEST\\nTEST\\nTEST");
    EXPECT_EQ(eventLogger->WriteCommonHead(1, sysEvent), true);
    sysEvent->SetEventValue("TID", gettid());
    EXPECT_EQ(eventLogger->WriteCommonHead(1, sysEvent), true);
}

/**
 * @tc.name: EventLoggerTest_CheckEventOnContinue_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_CheckEventOnContinue_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::string testName = "EventLoggerTest_CheckEventOnContinue_001";
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    eventLogger->CheckEventOnContinue(sysEvent);
    EXPECT_TRUE(sysEvent != nullptr);
}

/**
 * @tc.name: EventLoggerTest_WriteFreezeJsonInfo_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_WriteFreezeJsonInfo_001, TestSize.Level0)
{
    auto eventLogger = std::make_shared<EventLogger>();
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::string testName = "EventLoggerTest_WriteFreezeJsonInfo_001";
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    sysEvent->SetEventValue("EVENTNAME", testName);
    sysEvent->SetEventValue("MODULE_NAME", testName);
    sysEvent->SetEventValue("PACKAGE_NAME", testName);
    sysEvent->SetEventValue("PROCESS_NAME", testName);
    sysEvent->SetEventValue("eventLog_action", "pb:1");
    sysEvent->SetEventValue("eventLog_interval", 1);
    sysEvent->SetEventValue("STACK", "TEST\\nTEST\\nTEST");
    sysEvent->SetEventValue("MSG", "TEST\\nTEST\\nTEST");
    sysEvent->eventName_ = "UI_BLOCK_6S";
    sysEvent->SetEventValue("BINDER_INFO", "async\\nEventLoggerTest");
    std::vector<std::string> binderPids;
    std::string threadStack;
    EXPECT_EQ(eventLogger->WriteFreezeJsonInfo(1, 1, sysEvent, binderPids, threadStack), true);
    sysEvent->SetEventValue("BINDER_INFO", "context");
    binderPids.clear();
    EXPECT_EQ(eventLogger->WriteFreezeJsonInfo(1, 1, sysEvent, binderPids, threadStack), true);
    std::string binderInfo = "1:1\\n1:1\\n" + std::to_string(getpid()) +
        ":1\\n1:1\\n1:1\\n1:1\\n1:1";
    sysEvent->SetEventValue("BINDER_INFO", binderInfo);
    binderPids.clear();
    EXPECT_EQ(eventLogger->WriteFreezeJsonInfo(1, 1, sysEvent, binderPids, threadStack), true);
}

/**
 * @tc.name: EventLoggerTest_WriteFreezeJsonInfo_002
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_WriteFreezeJsonInfo_002, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::string testName = "EventLoggerTest_WriteFreezeJsonInfo_002";
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    std::vector<std::string> binderPids;
    sysEvent->eventName_ = "THREAD_BLOCK_6S";
    sysEvent->SetEventValue("BINDER_INFO", TEST_PATH + ", "
        "async\\tEventLoggerTest\\n 1:2 2:3 3:4 3:4 context");
    std::string threadStack;
    EXPECT_EQ(eventLogger->WriteFreezeJsonInfo(1, 1, sysEvent, binderPids, threadStack), true);
    sysEvent->eventName_ = "LIFECYCLE_TIMEOUT";
    EXPECT_EQ(eventLogger->WriteFreezeJsonInfo(1, 1, sysEvent, binderPids, threadStack), true);
    sysEvent->SetEventValue("BINDER_INFO", TEST_PATH + ", "
        "22000:22000 to 12001:12001 code 9 wait:1 s test");
    EXPECT_EQ(eventLogger->WriteFreezeJsonInfo(1, 1, sysEvent, binderPids, threadStack), true);
}

/**
 * @tc.name: EventLoggerTest_WriteFreezeJsonInfo_003
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_WriteFreezeJsonInfo_003, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::string testName = "EventLoggerTest_WriteFreezeJsonInfo_003";
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    std::vector<std::string> binderPids;
    EXPECT_TRUE(FileUtil::FileExists("/data/test/log/test.txt"));
    sysEvent->eventName_ = "LIFECYCLE_TIMEOUT";
    std::string threadStack;
    EXPECT_EQ(eventLogger->WriteFreezeJsonInfo(1, 1, sysEvent, binderPids, threadStack), true);
}

/**
 * @tc.name: EventLoggerTest_JudgmentRateLimiting_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_JudgmentRateLimiting_001, TestSize.Level3)
{
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::string testName = "EventLoggerTest_JudgmentRateLimiting_001";
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    sysEvent->SetEventValue("eventLog_interval", 0);
    auto eventLogger = std::make_shared<EventLogger>();
    bool ret = eventLogger->JudgmentRateLimiting(sysEvent);
    EXPECT_EQ(ret, true);
    sysEvent->SetEventValue("eventLog_interval", 1);
    sysEvent->SetEventValue("PID", getpid());
    sysEvent->SetEventValue("NAME", testName);
    eventLogger->eventTagTime_["NAME"] = 100;
    eventLogger->eventTagTime_[testName] = 100;
    ret = eventLogger->JudgmentRateLimiting(sysEvent);
    EXPECT_EQ(ret, true);
    sysEvent->SetValue("eventLog_interval", 0);
    ret = eventLogger->JudgmentRateLimiting(sysEvent);
    EXPECT_EQ(ret, true);
    int32_t interval = sysEvent->GetIntValue("eventLog_interval");
    EXPECT_EQ(interval, 0);
}

/**
 * @tc.name: EventLoggerTest_StartLogCollect_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_StartLogCollect_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::string testName = "EventLoggerTest_StartLogCollect_001";
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    sysEvent->SetEventValue("eventLog_interval", 1);
    sysEvent->eventName_ = "GET_DISPLAY_SNAPSHOT";
    sysEvent->SetEventValue("PID", getpid());
    sysEvent->happenTime_ = TimeUtil::GetMilliseconds();
    sysEvent->SetEventValue("UID", getuid());
    sysEvent->SetValue("eventLog_action", "pb:1\npb:2");
    std::shared_ptr<EventLoop> loop = std::make_shared<EventLoop>("eventLoop");
    loop->StartLoop();
    eventLogger->BindWorkLoop(loop);
    eventLogger->threadLoop_ = loop;
    eventLogger->StartLogCollect(sysEvent);
    sysEvent->eventName_ = "THREAD_BLOCK_3S";
    eventLogger->StartLogCollect(sysEvent);
    EXPECT_TRUE(sysEvent != nullptr);
    sysEvent->SetEventValue("MSG", "Test\nnotifyAppFault exception\n");
    eventLogger->StartLogCollect(sysEvent);
    EXPECT_TRUE(sysEvent != nullptr);
}

/**
 * @tc.name: EventLoggerTest_UpdateDB_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_UpdateDB_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::string testName = "EventLoggerTest_UpdateDB_001";
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    bool ret = eventLogger->UpdateDB(sysEvent, "nolog");
    EXPECT_TRUE(ret);
    ret = eventLogger->UpdateDB(sysEvent, "log");
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: EventLoggerTest_GetCmdlineContent_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_GetCmdlineContent_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    eventLogger->cmdlinePath_ = "";
    eventLogger->GetCmdlineContent();
    eventLogger->cmdlinePath_ = "/proc/cmdline";
    EXPECT_TRUE(!eventLogger->cmdlinePath_.empty());
}

/**
 * @tc.name: EventLoggerTest_ProcessRebootEvent_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_ProcessRebootEvent_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    eventLogger->cmdlineContent_ = "reboot_reason = EventLoggerTest "
        "normal_reset_type = EventLoggerTest\\n";
    eventLogger->rebootReasons_.push_back("EventLoggerTest");
    std::string ret = eventLogger->GetRebootReason();
    EXPECT_EQ(ret, "LONG_PRESS");
    eventLogger->ProcessRebootEvent();
    eventLogger->cmdlineContent_ = "reboot_reason";
    ret = eventLogger->GetRebootReason();
    EXPECT_EQ(ret, "");
    eventLogger->ProcessRebootEvent();
}

/**
 * @tc.name: EventLoggerTest_GetListenerName_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_GetListenerName_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    EXPECT_EQ(eventLogger->GetListenerName(), "EventLogger");
}

/**
 * @tc.name: EventLoggerTest_GetConfig_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_GetConfig_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    eventLogger->GetCmdlineContent();
    eventLogger->GetRebootReasonConfig();
    EXPECT_TRUE(eventLogger != nullptr);
}

/**
 * @tc.name: EventLoggerTest_OnUnorderedEvent_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_OnUnorderedEvent_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    auto event = std::make_shared<Event>("sender", "event");
    event->messageType_ = Event::MessageType::PLUGIN_MAINTENANCE;
    bool ret = eventLogger->CanProcessRebootEvent(*(event.get()));
    EXPECT_EQ(ret, true);
    std::shared_ptr<EventLoop> loop = std::make_shared<EventLoop>("eventLoop");
    loop->StartLoop();
    eventLogger->BindWorkLoop(loop);
    eventLogger->threadLoop_ = loop;
    eventLogger->OnUnorderedEvent(*(event.get()));
}

/**
 * @tc.name: EventLoggerTest_OnUnorderedEvent_002
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_OnUnorderedEvent_002, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    auto event = std::make_shared<Event>("sender", "event");
    event->messageType_ = Event::MessageType::TELEMETRY_EVENT;
#ifdef HILOG_CATCHER_ENABLE
    eventLogger->OnUnorderedEvent(*(event.get()));
#endif
    bool ret = eventLogger->CanProcessRebootEvent(*(event.get()));
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name: EventLoggerTest_ClearOldFile_001
 * @tc.desc: Loging aging test
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_ClearOldFile_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    sleep(1);
    HiSysEventWrite(HiSysEvent::Domain::AAFWK, "THREAD_BLOCK_3S", HiSysEvent::EventType::FAULT,
        "MODULE", "foundation", "MSG", "test remove");
    sleep(3);
    HiSysEventWrite(HiSysEvent::Domain::AAFWK, "THREAD_BLOCK_6S", HiSysEvent::EventType::FAULT,
        "MODULE", "foundation", "MSG", "test remove", "HITRACE_ID", "1234", "SPAN_ID", "34");
    sleep(3);
    HiSysEventWrite(HiSysEvent::Domain::AAFWK, "LIFECYCLE_HALF_TIMEOUT", HiSysEvent::EventType::FAULT,
        "MODULE", "foundation", "MSG", "test remove");
    std::vector<LogFile> logFileList = FreezeManager::GetInstance()->eventLogStore_->GetLogFiles();
    auto beforeSize = static_cast<long>(logFileList.size());
    printf("Before-- logFileList num: %ld\n", beforeSize);
    auto iter = logFileList.begin();
    while (iter != logFileList.end()) {
        auto beforeIter = iter;
        iter++;
        EXPECT_TRUE(beforeIter < iter);
    }
    auto folderSize = FileUtil::GetFolderSize(FreezeManager::LOGGER_EVENT_LOG_PATH);
    uint32_t maxSize = 10240; // test value
    FreezeManager::GetInstance()->eventLogStore_->SetMaxSize(maxSize);
    FreezeManager::GetInstance()->eventLogStore_->ClearOldestFilesIfNeeded();
    auto size = FileUtil::GetFolderSize(FreezeManager::LOGGER_EVENT_LOG_PATH);
    auto listSize = static_cast<long>(FreezeManager::GetInstance()->eventLogStore_->GetLogFiles().size());
    printf("After-- logFileList num: %ld\n", listSize);
    if (listSize == beforeSize) {
        EXPECT_TRUE(size == folderSize);
    } else {
        EXPECT_TRUE(size < folderSize);
    }
}

/**
 * @tc.name: OnEventListeningCallbackTest_001
 * @tc.desc: OnEventListeningCallbackTest_001 matching IPC_FULL events and HIVIEW's UID
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, OnEventListeningCallbackTest_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    sleep(1);
    HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, "IPC_FULL_WARNING", HiSysEvent::EventType::FAULT,
        "MODULE", "foundation", "UID", 1201);
    sleep(3);
    HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, "IPC_FULL", HiSysEvent::EventType::FAULT,
        "MODULE", "foundation", "MSG", "test remove", "HITRACE_ID", "1234", "UID", 1201);
    EXPECT_TRUE(eventLogger != nullptr);
    eventLogger->OnUnload();
}

/**
 * @tc.name: EventLoggerTest_GetFile_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_GetFile_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::string testName = "GET_DISPLAY_SNAPSHOT";
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    sysEvent->SetEventValue("PID", getpid());
    sysEvent->eventName_ = "GET_DISPLAY_SNAPSHOT";
    sysEvent->happenTime_ = TimeUtil::GetMilliseconds();
    sysEvent->eventId_ = 1;
    std::string logFile = "";
    eventLogger->StartFfrtDump(sysEvent);
    int result = eventLogger->GetFile(sysEvent, logFile, true);
    printf("GetFile result=%d\n", result);
    EXPECT_TRUE(logFile.size() > 0);
    result = eventLogger->GetFile(sysEvent, logFile, false);
    EXPECT_TRUE(result > 0);
    EXPECT_TRUE(logFile.size() > 0);
    sysEvent->eventName_ = "TEST";
    sysEvent->SetEventValue("PID", 10001); // test value
    eventLogger->StartFfrtDump(sysEvent);
    result = eventLogger->GetFile(sysEvent, logFile, false);
    EXPECT_TRUE(result > 0);
    EXPECT_TRUE(logFile.size() > 0);
    result = eventLogger->GetFile(sysEvent, logFile, true);
    printf("GetFile result=%d\n", result);
    EXPECT_TRUE(logFile.size() > 0);
}

/**
 * @tc.name: EventLoggerTest_ReportUserPanicWarning_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_ReportUserPanicWarning_001, TestSize.Level0)
{
    auto eventLogger = std::make_shared<EventLogger>();
    eventLogger->OnLoad();
    sleep(1);

    auto jsonStr = "{\"domain_\":\"FORM_MANAGER\"}";
    long pid = getpid();
#ifdef WINDOW_MANAGER_ENABLE
    uint64_t curentTime = TimeUtil::GetMilliseconds();
    while (eventLogger->backTimes_.size() < 4) {
        eventLogger->backTimes_.push_back(curentTime);
        curentTime += 100;
    }

    std::shared_ptr<SysEvent> sysEvent2 = std::make_shared<SysEvent>("GESTURE_NAVIGATION_BACK",
        nullptr, jsonStr);
    sysEvent2->SetEventValue("PID", pid);
    sysEvent2->happenTime_ = TimeUtil::GetMilliseconds();
    EventFocusListener::lastChangedTime_ = 0;
    eventLogger->ReportUserPanicWarning(sysEvent2, pid);
#endif

    std::shared_ptr<SysEvent> sysEvent3 = std::make_shared<SysEvent>("FREQUENT_CLICK_WARNING",
        nullptr, jsonStr);
    sysEvent3->SetEventValue("PID", pid);
    sysEvent3->happenTime_ = 4000; // test value
#ifdef WINDOW_MANAGER_ENABLE
    eventLogger->ReportUserPanicWarning(sysEvent3, pid);
    sysEvent3->happenTime_ = 2500; // test value
    eventLogger->ReportUserPanicWarning(sysEvent3, pid);
#endif
    EXPECT_TRUE(true);
    eventLogger->OnUnload();
}

/**
 * @tc.name: EventLoggerTest_ReportUserPanicWarning_002
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_ReportUserPanicWarning_002, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    auto jsonStr = "{\"domain_\":\"FORM_MANAGER\"}";
    long pid = getpid();
    std::string testName = "FREQUENT_CLICK_WARNING";
    std::shared_ptr<SysEvent> event = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    event->eventName_ = testName;
    event->SetEventValue("PID", pid);
#ifdef WINDOW_MANAGER_ENABLE
    EventFocusListener::lastChangedTime_ = 900; // test value
    event->happenTime_ = 1000; // test value
    eventLogger->ReportUserPanicWarning(event, pid);
    EXPECT_TRUE(eventLogger->backTimes_.empty());
    event->happenTime_ = 4000; // test value
    event->SetEventValue("PROCESS_NAME", "EventLoggerTest_ReportUserPanicWarning_002");
    eventLogger->ReportUserPanicWarning(event, pid);
    EXPECT_TRUE(eventLogger->backTimes_.empty());
#endif
}

/**
 * @tc.name: EventLoggerTest_ReportUserPanicWarning_003
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_ReportUserPanicWarning_003, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    auto jsonStr = "{\"domain_\":\"FORM_MANAGER\"}";
    long pid = getpid();
    std::string testName = "EventLoggerTest_ReportUserPanicWarning_003";
    std::shared_ptr<SysEvent> event = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    event->eventName_ = testName;
    event->SetEventValue("PID", pid);
#ifdef WINDOW_MANAGER_ENABLE
    EXPECT_TRUE(eventLogger->backTimes_.empty());
    EventFocusListener::lastChangedTime_ = 0; // test value
    event->happenTime_ = 3000; // test value
    eventLogger->ReportUserPanicWarning(event, pid);
    EXPECT_EQ(eventLogger->backTimes_.size(), 1);
    while (eventLogger->backTimes_.size() <= 5) {
        int count = 1000; // test value
        eventLogger->backTimes_.push_back(count++);
    }
    EXPECT_TRUE(eventLogger->backTimes_.size() > 5);
    eventLogger->ReportUserPanicWarning(event, pid);
    EXPECT_TRUE(eventLogger->backTimes_.empty());
#endif
}

/**
 * @tc.name: EventLoggerTest_ReportUserPanicWarning_004
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_ReportUserPanicWarning_004, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    auto jsonStr = "{\"domain_\":\"FORM_MANAGER\"}";
    long pid = getpid();
    std::string testName = "EventLoggerTest_ReportUserPanicWarning_004";
    std::shared_ptr<SysEvent> event = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    event->eventName_ = testName;
    event->SetEventValue("PID", pid);
#ifdef WINDOW_MANAGER_ENABLE
    EXPECT_TRUE(eventLogger->backTimes_.empty());
    EventFocusListener::lastChangedTime_ = 0; // test value
    event->happenTime_ = 5000; // test value
    while (eventLogger->backTimes_.size() < 5) {
        int count = 1000; // test value
        eventLogger->backTimes_.push_back(count++);
    }
    EXPECT_TRUE(eventLogger->backTimes_.size() > 0);
    eventLogger->ReportUserPanicWarning(event, pid);
    EXPECT_EQ(eventLogger->backTimes_.size(), 4);
#endif
}

/**
 * @tc.name: EventLoggerTest_WriteCallStack_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_WriteCallStack_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    auto jsonStr = "{\"domain_\":\"FORM_MANAGER\"}";
    long pid = getpid();
    std::string testName = "EventLoggerTest_WriteCallStack_001";
    std::shared_ptr<SysEvent> event = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    eventLogger->WriteCallStack(event, 0);
    event->SetEventValue("PID", pid);
    event->SetEventValue("EVENT_KEY_FORM_BLOCK_CALLSTACK", testName);
    event->SetEventValue("EVENT_KEY_FORM_BLOCK_APPNAME", testName);
    event->eventName_ = "FORM_BLOCK_CALLSTACK";
    event->domain_ = "FORM_MANAGER";
    eventLogger->WriteCallStack(event, 0);
    EXPECT_TRUE(!event->GetEventValue("EVENT_KEY_FORM_BLOCK_APPNAME").empty());
}

/**
 * @tc.name: EventLoggerTest_RegisterFocusListener_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_RegisterFocusListener_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    eventLogger->OnLoad();
    sleep(1);
#ifdef WINDOW_MANAGER_ENABLE
    EventFocusListener::RegisterFocusListener();
    EventFocusListener::registerState_ = EventFocusListener::REGISTERED;
    eventLogger->OnUnload();
    EXPECT_EQ(EventFocusListener::registerState_, EventFocusListener::UNREGISTERED);
#else
    eventLogger->OnUnload();
#endif
}

/**
 * @tc.name: EventLoggerTest_FreezeFilterTraceOn_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_FreezeFilterTraceOn_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
#ifdef HITRACE_CATCHER_ENABLE
    std::string testName = "EventLoggerTest_FreezeFilterTraceOn_001";
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::shared_ptr<SysEvent> event = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    event->SetEventValue("PROCESS_NAME", "EventLoggerTest");
    event->eventName_ = "APP_INPUT_BLOCK";
    eventLogger->FreezeFilterTraceOn(event, true);
    eventLogger->FreezeFilterTraceOn(event, false);
    event->eventName_ = "THREAD_BLOCK_3S";
    eventLogger->FreezeFilterTraceOn(event, false);
#endif
    EXPECT_EQ(event->GetEventValue("PACKAGE_NAME"), "");
}

/**
 * @tc.name: EventLoggerTest_IsHandleAppfreeze_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_IsHandleAppfreeze_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    auto jsonStr = "{\"domain_\":\"FORM_MANAGER\"}";
    long pid = getpid();
    std::string testName = "EventLoggerTest_IsHandleAppfreeze_001";
    std::shared_ptr<SysEvent> event = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    event->SetEventValue("PACKAGE_NAME", testName);
    OHOS::system::SetParameter("hiviewdfx.appfreeze.filter_bundle_name", testName);
    EXPECT_FALSE(eventLogger->IsHandleAppfreeze(event));
}

/**
 * @tc.name: EventLoggerTest_IsKernelStack_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_IsKernelStack_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    std::string stack = "";
    bool result = eventLogger->IsKernelStack(stack);
    EXPECT_TRUE(!result);
    stack = "Stack backtrace";
    result = eventLogger->IsKernelStack(stack);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: EventLoggerTest_GetAppFreezeStack_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_GetAppFreezeStack_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    std::string stack = "TEST\\nTEST\\nTEST";
    std::string kernelStack = "";
    std::string contentStack = "Test";
    std::string bundleName = "Test bundleName";
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::string testName = "EventLoggerTest_GetAppFreezeStack_001";
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    sysEvent->SetEventValue("PROCESS_NAME", testName);
    sysEvent->SetEventValue("APP_RUNNING_UNIQUE_ID", "Test");
    sysEvent->SetEventValue("STACK", stack);
    sysEvent->SetEventValue("MSG", stack);
    sysEvent->eventName_ = "UI_BLOCK_6S";
    sysEvent->SetEventValue("BINDER_INFO", "async\\nEventLoggerTest");
    eventLogger->GetAppFreezeStack(-1, sysEvent, stack, "msg", kernelStack);
    EXPECT_TRUE(kernelStack.empty());
    eventLogger->GetAppFreezeStack(1, sysEvent, stack, "msg", kernelStack);
    EXPECT_TRUE(kernelStack.empty());
    eventLogger->GetNoJsonStack(stack, contentStack, kernelStack, false, bundleName);
    EXPECT_TRUE(kernelStack.empty());
    stack = "Test:Stack backtrace";
    sysEvent->SetEventValue("STACK", stack);
    eventLogger->GetAppFreezeStack(1, sysEvent, stack, "msg", kernelStack);
    EXPECT_TRUE(!kernelStack.empty());
    eventLogger->GetNoJsonStack(stack, contentStack, kernelStack, false, bundleName);
    EXPECT_TRUE(!kernelStack.empty());
    sysEvent->SetEventValue("APP_RUNNING_UNIQUE_ID", "Test");
    sysEvent->SetEventValue("STACK", "/data/test/log/test.txt");
    eventLogger->GetAppFreezeStack(1, sysEvent, stack, "msg", kernelStack);
    EXPECT_TRUE(!kernelStack.empty());
    std::string msg = "Fault time:Test\nmainHandler dump is:\n Test\nEvent "
        "{Test}\nLow priority event queue information:test\nTotal size of Low events : 10\n";
    eventLogger->GetAppFreezeStack(1, sysEvent, stack, msg, kernelStack);
    EXPECT_TRUE(!kernelStack.empty());
}

/**
 * @tc.name: EventLoggerTest_WriteKernelStackToFile_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_WriteKernelStackToFile_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    std::string stack = "";
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::string testName = "EventLoggerTest_WriteKernelStackToFile_001";
    std::shared_ptr<SysEvent> event = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    event->eventName_ = testName;
    int testValue = 1; // test value
    event->SetEventValue("PID", testValue);
    event->happenTime_ = TimeUtil::GetMilliseconds();
    std::string kernelStack = "";
    eventLogger->WriteKernelStackToFile(event, testValue, kernelStack);
    kernelStack = "Test";
    EXPECT_TRUE(!kernelStack.empty());
    eventLogger->WriteKernelStackToFile(event, testValue, kernelStack);
}

/**
 * @tc.name: EventLoggerTest_ParsePeerStack_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_ParsePeerStack_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    std::string binderInfo = "";
    std::string binderPeerStack = "";
    std::string bundleName = "";
    eventLogger->ParsePeerStack(binderInfo, binderPeerStack, bundleName);
    EXPECT_TRUE(binderPeerStack.empty());
    binderInfo = "Test";
    eventLogger->ParsePeerStack(binderInfo, binderPeerStack, bundleName);
    EXPECT_TRUE(binderPeerStack.empty());
    binderInfo = "Binder catcher stacktrace, type is peer, pid : 111\n Stack "
        "backtrace: Test\n Binder catcher stacktrace, type is peer, pid : 112\n Test";
    eventLogger->ParsePeerStack(binderInfo, binderPeerStack, bundleName);
    EXPECT_TRUE(!binderPeerStack.empty());
    binderPeerStack = "";
    binderInfo = "111\n Stack backtrace: Test\n 112\n Test";
    eventLogger->ParsePeerStack(binderInfo, binderPeerStack, bundleName);
    EXPECT_TRUE(binderPeerStack.empty());
}

/**
 * @tc.name: EventLoggerTest_GetEventPid_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_GetEventPid_001, TestSize.Level0)
{
    auto eventLogger = std::make_shared<EventLogger>();
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::string testName = "EventLoggerTest_GetEventPid_001";
    std::shared_ptr<SysEvent> event = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    event->SetEventValue("PID", 1);
    event->SetEventValue("PACKAGE_NAME", testName);
    event->eventName_ = testName;
    EXPECT_TRUE(eventLogger->GetEventPid(event) > 0);
}

/**
 * @tc.name: EventLoggerTest_GetEventPid_002
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_GetEventPid_002, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::string testName = "EventLoggerTest_GetEventPid_002";
    std::shared_ptr<SysEvent> event = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    event->SetEventValue("PID", 0);
    EXPECT_TRUE(event->GetEventIntValue("PID") <= 0);
    event->SetEventValue("PACKAGE_NAME", "foundation");
    event->eventName_ = testName;
    EXPECT_TRUE(eventLogger->GetEventPid(event) > 0);
}

/**
 * @tc.name: EventLoggerTest_SetEventTerminalBinder_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_SetEventTerminalBinder_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    eventLogger->OnLoad();
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::shared_ptr<SysEvent> event = std::make_shared<SysEvent>("testSender", nullptr, jsonStr);
    event->eventName_ = "THREAD_BLOCK_3S";
    std::string threadStack = "";
    eventLogger->SetEventTerminalBinder(event, threadStack, 0);
    EXPECT_EQ(event->GetEventValue("TERMINAL_THREAD_STACK"), "");
    threadStack = "thread_block_3s thread stack";
    int fd = FreezeManager::GetInstance()->eventLogStore_->CreateLogFile("test_set_terminal_binder");
    if (fd > 0) {
        eventLogger->SetEventTerminalBinder(event, threadStack, fd);
        EXPECT_EQ(event->GetEventValue("TERMINAL_THREAD_STACK"), "thread_block_3s thread stack");
        close(fd);
    }
    threadStack = "ipc_full thread stack";
    event->eventName_ = "IPC_FULL";
    eventLogger->SetEventTerminalBinder(event, threadStack, 0);
    EXPECT_EQ(event->GetEventValue("TERMINAL_THREAD_STACK"), "ipc_full thread stack");
    eventLogger->OnUnload();
}

/**
 * @tc.name: EventLoggerTest_CheckScreenOnRepeat_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_CheckScreenOnRepeat_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::string testName = "APP_INPUT_BLOCK";
    std::shared_ptr<SysEvent> event = std::make_shared<SysEvent>(testName, nullptr, jsonStr);
    event->eventName_ = testName;
    eventLogger->CheckScreenOnRepeat(event);
    EXPECT_TRUE(event->eventName_ != "SCREEN_ON");
}

/**
 * @tc.name: EventLoggerTest_AddBootScanEvent_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_AddBootScanEvent_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    eventLogger->AddBootScanEvent();
    EXPECT_TRUE(eventLogger != nullptr);
}

/**
 * @tc.name: EventLoggerTest_StartBootScan_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_StartBootScan_001, TestSize.Level3)
{
    std::string path = "/data/log/faultlog/freeze/appfreeze-com.test.demo-20020191-20250320154130";
    auto fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd < 0) {
        printf("Fail to create File. errno: %d\n", errno);
        FAIL();
    }
    FileUtil::SaveStringToFd(fd, "\ntesttest\nPID:12345\nSTRINGID:THREAD_BLOCK_6S\nTest\n");
    StartBootScan();
    close(fd);
}

/**
 * @tc.name: EventLoggerTest_StartBootScan_002
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_StartBootScan_002, TestSize.Level3)
{
    std::string path = "/data/log/faultlog/freeze/crash-com.test.demo-20020191-20250320154130";
    auto fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd < 0) {
        printf("Fail to create File. errno: %d\n", errno);
        FAIL();
    }
    FileUtil::SaveStringToFd(fd, "\ntesttest\nPID:12345\nSTRINGID:THREAD_BLOCK_6S\nTest\n");
    StartBootScan();
    close(fd);
}

/**
 * @tc.name: EventLoggerTest_GetFileLastAccessTimeStamp_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_GetFileLastAccessTimeStamp_001, TestSize.Level3)
{
    time_t ret = GetFileLastAccessTimeStamp("EventLoggerTest");
    ret = GetFileLastAccessTimeStamp("/data/test/log/test.txt");
    EXPECT_TRUE(ret >= 0);
}

/**
 * @tc.name: EventLoggerTest_CheckFfrtEvent_001
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_CheckFfrtEvent_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    auto jsonStr = "{\"domain_\":\"RELIABILITY\"}";
    std::string testName = "EventLoggerTest_CheckFfrtEvent_001";
    std::shared_ptr<SysEvent> event = std::make_shared<SysEvent>(testName,
        nullptr, jsonStr);
    bool ret = eventLogger->CheckFfrtEvent(event);
    EXPECT_TRUE(ret);
    event->eventName_ = "CONGESTION";
    ret = eventLogger->CheckFfrtEvent(event);
    EXPECT_FALSE(ret);
    event->SetEventValue("SCENARIO", "Task_Sch_Timeout");
    ret = eventLogger->CheckFfrtEvent(event);
    EXPECT_FALSE(ret);
    event->SetEventValue("PROCESS_NAME", "com.ohos.sceneboard");
    ret = eventLogger->CheckFfrtEvent(event);
    EXPECT_FALSE(ret);
    event->SetEventValue("SCENARIO", "Trigger_Escape");
    ret = eventLogger->CheckFfrtEvent(event);
    EXPECT_TRUE(ret);
    event->SetEventValue("SCENARIO", "Serial_Queue_Timeout");
    event->SetEventValue("PROCESS_NAME", "foundation");
    event->SetEventValue("TID", 2025);
    ret = eventLogger->CheckFfrtEvent(event);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: EventLoggerTest_CheckFfrtEvent_002
 * @tc.desc: EventLoggerTest
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_CheckFfrtEvent_002, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    int ret = HiSysEventWrite(HiSysEvent::Domain::FFRT, "CONGESTION", HiSysEvent::EventType::FAULT,
        "SENARIO", "Trigger_Escape",
        "PROCESS_NAME", "foundation",
        "MSG", "qos:2, qName:testName");
    sleep(1);
    EXPECT_TRUE(ret == 0);

    ret = HiSysEventWrite(HiSysEvent::Domain::FFRT, "CONGESTION", HiSysEvent::EventType::FAULT,
        "SENARIO", "Serial_Queue_Timeout",
        "PROCESS_NAME", "EventLoggerTest_CheckFfrtEvent_002",
        "MSG", "qos:2, qName:testName2");
    sleep(1);
    EXPECT_TRUE(ret == 0);

    ret = HiSysEventWrite(HiSysEvent::Domain::FFRT, "CONGESTION", HiSysEvent::EventType::FAULT,
        "SENARIO", "Task_Sch_Timeout",
        "PROCESS_NAME", "foundation",
        "MSG", "qos:2, qName:testName3");
    sleep(1);
    EXPECT_TRUE(ret == 0);
    EXPECT_TRUE(eventLogger);
}

/**
 * @tc.name: EventLoggerTest_GetWindowIdFromLine_001
 * @tc.desc: Test GetWindowIdFromLine with WindowId containing special characters
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_GetWindowIdFromLine_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    std::string line = "WindowName 0 100 123";
    std::string expected = "123";
    
    std::string result = eventLogger->GetWindowIdFromLine(line);
    EXPECT_EQ(expected, result);
}

/**
 * @tc.name: EventLoggerTest_GetWindowIdFromLine_002
 * @tc.desc: Test GetWindowIdFromLine with WindowId containing special characters
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_GetWindowIdFromLine_002, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    std::string line = "WindowName 0 100";
    std::string expected = "";
    
    std::string result = eventLogger->GetWindowIdFromLine(line);
    EXPECT_EQ(expected, result);
}

/**
 * @tc.name: EventLoggerTest_GetWindowIdFromLine_003
 * @tc.desc: Test GetWindowIdFromLine with WindowId containing special characters
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_GetWindowIdFromLine_003, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    std::string line = "WindowName 0 100 123 200";
    std::string expected = "123";
    
    std::string result = eventLogger->GetWindowIdFromLine(line);
    EXPECT_EQ(expected, result);
}

/**
 * @tc.name: EventLoggerTest_GetWindowIdFromLine_004
 * @tc.desc: Test GetWindowIdFromLine with WindowId containing special characters
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_GetWindowIdFromLine_004, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    std::string line = "";
    std::string expected = "";
    
    std::string result = eventLogger->GetWindowIdFromLine(line);
    EXPECT_EQ(expected, result);
}

/**
 * @tc.name: EventLoggerTest_HandleEventLoggerCmd_001
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerTest, EventLoggerTest_HandleEventLoggerCmd_001, TestSize.Level3)
{
    auto eventLogger = std::make_shared<EventLogger>();
    std::string cmd = "tr";
    auto fd = open(TEST_PATH.c_str(), O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd < 0) {
        printf("Fail to create File. errno: %d\n", errno);
        FAIL();
    }
    SysEventCreator sysEventCreator("HIVIEWDFX", "EventLoggerTest", SysEventCreator::FAULT);
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>("EventLoggerTest", nullptr, sysEventCreator);
    std::shared_ptr<EventLogTask> logTask = std::make_shared<EventLogTask>(fd, 1, sysEvent);
    eventLogger->queue_ = nullptr;
    eventLogger->queueSubmitTrace_ = nullptr;
    eventLogger->HandleEventLoggerCmd(cmd, sysEvent, fd, logTask);
    eventLogger->HandleEventLoggerCmd(cmd, sysEvent, fd, logTask);
    eventLogger->InitQueue();
    EXPECT_TRUE(eventLogger->queue_ != nullptr);
    EXPECT_TRUE(eventLogger->queueSubmitTrace_ != nullptr);
    eventLogger->HandleEventLoggerCmd(cmd, sysEvent, fd, logTask);
    cmd = "k:SysRqFile";
    eventLogger->HandleEventLoggerCmd(cmd, sysEvent, fd, logTask);
    cmd = "cmd:m";
    eventLogger->HandleEventLoggerCmd(cmd, sysEvent, fd, logTask);
    close(fd);
    EXPECT_TRUE(eventLogger);
}
} // namespace HiviewDFX
} // namespace OHOS
