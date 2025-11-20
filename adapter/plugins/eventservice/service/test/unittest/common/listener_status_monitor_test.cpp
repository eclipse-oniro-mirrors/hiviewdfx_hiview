/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "listener_status_monitor_test.h"

#include <unistd.h>

#include "common_utils.h"
#include "listener_status_util.h"
#include "hisysevent.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr size_t TEST_CAPACITY = 100;
constexpr uint64_t TEST_INTERVAL = 1;
constexpr int32_t TEST_UID = 10000;
const std::string TEST_NAME = "TEST_NAME";
const std::string TEST_LAST_DATE = "TEST_DATE";
const std::string TEST_LOOPER = "test_looper";
const std::string TEST_EVENT_DOMAIN = "test_event_domain";
const std::string TEST_EVENT_NAME = "test_event_name";
const std::string TEST_EVENT_TAG = "test_event_tag";
const std::string TEST_DIFF_STR = "test_diff_str";
}
using ListenerCallerMap = std::unordered_map<ListenerCallerInfo, ListenerStatusInfo, ListenerCallerInfo::Hash>;
using OHOS::HiviewDFX::HiSysEvent;

void ListenerStatusMonitorTest::SetUpTestCase() {}

void ListenerStatusMonitorTest::TearDownTestCase() {}

void ListenerStatusMonitorTest::SetUp() {}

void ListenerStatusMonitorTest::TearDown() {}

void ListenerStatusMonitorTest::SetLastDate(std::shared_ptr<ListenerStatusMonitor> monitor, const std::string& value)
{
    monitor->lastDate_ = value;
}

void ListenerStatusMonitorTest::GetCallers(std::shared_ptr<ListenerStatusMonitor> monitor,
    std::unordered_map<ListenerCallerInfo, ListenerStatusInfo, ListenerCallerInfo::Hash>& listenerCallers)
{
    listenerCallers = monitor->listenerCallers_;
}

/**
 * @tc.name: ListenerStatusMonitorTest001
 * @tc.desc: Test SetWorkLoop, null looper
 * @tc.type: FUNC
 * @tc.require: issue3048
 */
HWTEST_F(ListenerStatusMonitorTest, ListenerStatusMonitorTest001, testing::ext::TestSize.Level3)
{
    // 1. set null looper
    auto monitor = std::make_shared<ListenerStatusMonitor>(TEST_CAPACITY, TEST_INTERVAL);
    monitor->SetWorkLoop(nullptr);

    // 2. add caller
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(), true);
    ListenerCallerMap callers;
    GetCallers(monitor, callers);
    ASSERT_NE(callers.size(), 0);

    // 3. try to report event
    SetLastDate(monitor, TEST_LAST_DATE);
    sleep(TEST_INTERVAL * 2); // 2s

    // 4. check the result
    GetCallers(monitor, callers);
    ASSERT_NE(callers.size(), 0);
}

/**
 * @tc.name: ListenerStatusMonitorTest002
 * @tc.desc: Test SetWorkLoop, normal looper
 * @tc.type: FUNC
 * @tc.require: issue3048
 */
HWTEST_F(ListenerStatusMonitorTest, ListenerStatusMonitorTest002, testing::ext::TestSize.Level3)
{
    // 1. set normal looper
    auto monitor = std::make_shared<ListenerStatusMonitor>(TEST_CAPACITY, TEST_INTERVAL);
    auto looper = std::make_shared<EventLoop>(TEST_LOOPER);
    looper->StartLoop();
    monitor->SetWorkLoop(looper);

    // 2. add caller
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME), true);
    ListenerCallerMap callers;
    GetCallers(monitor, callers);
    ASSERT_NE(callers.size(), 0);

    // 3. try to report event
    SetLastDate(monitor, TEST_LAST_DATE);
    sleep(TEST_INTERVAL); // 2s

    // 4. check the result
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 0);
}

/**
 * @tc.name: ListenerStatusMonitorTest003
 * @tc.desc: Test RecordAddListener, success
 * @tc.type: FUNC
 * @tc.require: issue3048
 */
HWTEST_F(ListenerStatusMonitorTest, ListenerStatusMonitorTest003, testing::ext::TestSize.Level3)
{
    // 1. add first call
    auto monitor = std::make_shared<ListenerStatusMonitor>(TEST_CAPACITY, TEST_INTERVAL);
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME), true);
    ListenerCallerMap callers;
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 1); // 1 caller
    auto caller = callers.begin();
    ASSERT_EQ(caller->second.addSuccCount, 1); // 1 succ time

    // 2. add second call
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME), true);
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 1); // 1 caller
    caller = callers.begin();
    ASSERT_EQ(caller->second.addSuccCount, 2); // 2 succ times
}

/**
 * @tc.name: ListenerStatusMonitorTest004
 * @tc.desc: Test RecordAddListener, failed
 * @tc.type: FUNC
 * @tc.require: issue3048
 */
HWTEST_F(ListenerStatusMonitorTest, ListenerStatusMonitorTest004, testing::ext::TestSize.Level3)
{
    // 1. add first call
    auto monitor = std::make_shared<ListenerStatusMonitor>(TEST_CAPACITY, TEST_INTERVAL);
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME), false);
    ListenerCallerMap callers;
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 1); // 1 caller
    auto caller = callers.begin();
    ASSERT_EQ(caller->second.addFaultCount, 1); // 1 failed time

    // 2. add second call
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME), false);
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 1); // 1 caller
    caller = callers.begin();
    ASSERT_EQ(caller->second.addFaultCount, 2); // 2 failed times
}

/**
 * @tc.name: ListenerStatusMonitorTest005
 * @tc.desc: Test RecordRemoveListener, success
 * @tc.type: FUNC
 * @tc.require: issue3048
 */
HWTEST_F(ListenerStatusMonitorTest, ListenerStatusMonitorTest005, testing::ext::TestSize.Level3)
{
    // 1. add first caller
    auto monitor = std::make_shared<ListenerStatusMonitor>(TEST_CAPACITY, TEST_INTERVAL);
    monitor->RecordRemoveListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME), true);
    ListenerCallerMap callers;
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 1); // 1 caller
    auto caller = callers.begin();
    ASSERT_EQ(caller->second.removeSuccCount, 1); // 1 succ time

    // 2. add second caller
    monitor->RecordRemoveListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME), true);
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 1); // 1 caller
    caller = callers.begin();
    ASSERT_EQ(caller->second.removeSuccCount, 2); // 2 succ times
}

/**
 * @tc.name: ListenerStatusMonitorTest006
 * @tc.desc: Test RecordRemoveListener, failed
 * @tc.type: FUNC
 * @tc.require: issue3048
 */
HWTEST_F(ListenerStatusMonitorTest, ListenerStatusMonitorTest006, testing::ext::TestSize.Level3)
{
    // 1. add first call
    auto monitor = std::make_shared<ListenerStatusMonitor>(TEST_CAPACITY, TEST_INTERVAL);
    monitor->RecordRemoveListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME), false);
    ListenerCallerMap callers;
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 1); // 1 caller
    auto caller = callers.begin();
    ASSERT_EQ(caller->second.removeFaultCount, 1); // 1 failed time

    // 2. add second call
    monitor->RecordRemoveListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME), false);
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 1); // 1 caller
    caller = callers.begin();
    ASSERT_EQ(caller->second.removeFaultCount, 2); // 2 failed times
}

/**
 * @tc.name: ListenerStatusMonitorTest007
 * @tc.desc: Test RecordAddListener and RecordRemoveListener
 * @tc.type: FUNC
 * @tc.require: issue3048
 */
HWTEST_F(ListenerStatusMonitorTest, ListenerStatusMonitorTest007, testing::ext::TestSize.Level3)
{
    auto monitor = std::make_shared<ListenerStatusMonitor>(TEST_CAPACITY, TEST_INTERVAL);
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME), true);
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME), false);
    monitor->RecordRemoveListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME), true);
    monitor->RecordRemoveListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME), false);
    ListenerCallerMap callers;
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 1); // 1 caller
    auto caller = callers.begin();
    ASSERT_EQ(caller->first.listenerUid, TEST_UID);
    ASSERT_EQ(caller->first.listenerName, TEST_NAME);
    ASSERT_EQ(caller->first.eventRule, ""); // empty eventRule
    ASSERT_EQ(caller->second.addSuccCount, 1); // 1 succ time
    ASSERT_EQ(caller->second.addFaultCount, 1); // 1 failed time
    ASSERT_EQ(caller->second.removeSuccCount, 1); // 1 succ time
    ASSERT_EQ(caller->second.removeFaultCount, 1); // 1 failed time
}

/**
 * @tc.name: ListenerStatusMonitorTest008
 * @tc.desc: Test different callers, different uid and name
 * @tc.type: FUNC
 * @tc.require: issue3048
 */
HWTEST_F(ListenerStatusMonitorTest, ListenerStatusMonitorTest008, testing::ext::TestSize.Level3)
{
    auto monitor = std::make_shared<ListenerStatusMonitor>(TEST_CAPACITY, TEST_INTERVAL);
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(), true);
    ListenerCallerMap callers;
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 1); // 1 caller

    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME), false);
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 2); // 2 callers

    SysEventRule rule1(TEST_EVENT_DOMAIN, TEST_EVENT_NAME, TEST_EVENT_TAG, 0, 0);
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME, {rule1}), true);
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 3); // 3 callers

    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME, {rule1, rule1}), false);
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 4); // 4 callers
}

/**
 * @tc.name: ListenerStatusMonitorTest009
 * @tc.desc: Test different callers, different uid
 * @tc.type: FUNC
 * @tc.require: issue3048
 */
HWTEST_F(ListenerStatusMonitorTest, ListenerStatusMonitorTest009, testing::ext::TestSize.Level3)
{
    auto monitor = std::make_shared<ListenerStatusMonitor>(TEST_CAPACITY, TEST_INTERVAL);
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME), true);
    ListenerCallerMap callers;
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 1); // 1 caller

    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(0, TEST_NAME), false);
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 2); // 2 callers
}

/**
 * @tc.name: ListenerStatusMonitorTest010
 * @tc.desc: Test different callers, different name
 * @tc.type: FUNC
 * @tc.require: issue3048
 */
HWTEST_F(ListenerStatusMonitorTest, ListenerStatusMonitorTest010, testing::ext::TestSize.Level3)
{
    auto monitor = std::make_shared<ListenerStatusMonitor>(TEST_CAPACITY, TEST_INTERVAL);
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME), true);
    ListenerCallerMap callers;
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 1); // 1 caller

    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_DIFF_STR), false);
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 2); // 2 callers
}

/**
 * @tc.name: ListenerStatusMonitorTest011
 * @tc.desc: Test different callers, different rule.domain
 * @tc.type: FUNC
 * @tc.require: issue3048
 */
HWTEST_F(ListenerStatusMonitorTest, ListenerStatusMonitorTest011, testing::ext::TestSize.Level3)
{
    auto monitor = std::make_shared<ListenerStatusMonitor>(TEST_CAPACITY, TEST_INTERVAL);
    SysEventRule rule1(TEST_EVENT_DOMAIN, TEST_EVENT_NAME, TEST_EVENT_TAG, 0, 0);
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME, {rule1}), true);
    ListenerCallerMap callers;
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 1); // 1 caller

    rule1.domain = TEST_DIFF_STR;
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME, {rule1}), true);
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 2); // 2 callers
}

/**
 * @tc.name: ListenerStatusMonitorTest012
 * @tc.desc: Test different callers, different rule.name
 * @tc.type: FUNC
 * @tc.require: issue3048
 */
HWTEST_F(ListenerStatusMonitorTest, ListenerStatusMonitorTest012, testing::ext::TestSize.Level3)
{
    auto monitor = std::make_shared<ListenerStatusMonitor>(TEST_CAPACITY, TEST_INTERVAL);
    SysEventRule rule1(TEST_EVENT_DOMAIN, TEST_EVENT_NAME, TEST_EVENT_TAG, 0, 0);
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME, {rule1}), true);
    ListenerCallerMap callers;
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 1); // 1 caller

    rule1.eventName = TEST_DIFF_STR;
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME, {rule1}), true);
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 2); // 2 callers
}

/**
 * @tc.name: ListenerStatusMonitorTest013
 * @tc.desc: Test different callers, different rule.tag
 * @tc.type: FUNC
 * @tc.require: issue3048
 */
HWTEST_F(ListenerStatusMonitorTest, ListenerStatusMonitorTest013, testing::ext::TestSize.Level3)
{
    auto monitor = std::make_shared<ListenerStatusMonitor>(TEST_CAPACITY, TEST_INTERVAL);
    SysEventRule rule1(TEST_EVENT_DOMAIN, TEST_EVENT_NAME, TEST_EVENT_TAG, 0, 0);
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME, {rule1}), true);
    ListenerCallerMap callers;
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 1); // 1 caller

    rule1.tag = TEST_DIFF_STR;
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME, {rule1}), true);
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 2); // 2 callers
}

/**
 * @tc.name: ListenerStatusMonitorTest014
 * @tc.desc: Test different callers, different rule.ruleType
 * @tc.type: FUNC
 * @tc.require: issue3048
 */
HWTEST_F(ListenerStatusMonitorTest, ListenerStatusMonitorTest014, testing::ext::TestSize.Level3)
{
    auto monitor = std::make_shared<ListenerStatusMonitor>(TEST_CAPACITY, TEST_INTERVAL);
    SysEventRule rule1(TEST_EVENT_DOMAIN, TEST_EVENT_NAME, TEST_EVENT_TAG, 0, 0);
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME, {rule1}), true);
    ListenerCallerMap callers;
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 1); // 1 caller, default WHOLE_WORD

    rule1.ruleType = RuleType::WHOLE_WORD;
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME, {rule1}), true);
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 1); // 1 caller, same as ruleType=0

    rule1.ruleType = RuleType::PREFIX;
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME, {rule1}), true);
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 2); // 2 callers

    rule1.ruleType = RuleType::REGULAR;
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME, {rule1}), true);
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 3); // 3 callers
}

/**
 * @tc.name: ListenerStatusMonitorTest015
 * @tc.desc: Test different callers, different rule.eventType
 * @tc.type: FUNC
 * @tc.require: issue3048
 */
HWTEST_F(ListenerStatusMonitorTest, ListenerStatusMonitorTest015, testing::ext::TestSize.Level3)
{
    auto monitor = std::make_shared<ListenerStatusMonitor>(TEST_CAPACITY, TEST_INTERVAL);
    SysEventRule rule1(TEST_EVENT_DOMAIN, TEST_EVENT_NAME, TEST_EVENT_TAG, 0, 0);
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME, {rule1}), true);
    ListenerCallerMap callers;
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 1); // 1 caller

    rule1.eventType = HiSysEvent::EventType::FAULT;
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME, {rule1}), true);
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 2); // 2 callers

    rule1.eventType = HiSysEvent::EventType::STATISTIC;
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME, {rule1}), true);
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 3); // 3 callers

    rule1.eventType = HiSysEvent::EventType::SECURITY;
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME, {rule1}), true);
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 4); // 4 callers

    rule1.eventType = HiSysEvent::EventType::BEHAVIOR;
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME, {rule1}), true);
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 5); // 5 callers
}

/**
 * @tc.name: ListenerStatusMonitorTest016
 * @tc.desc: Test different callers, multiple callers
 * @tc.type: FUNC
 * @tc.require: issue3048
 */
HWTEST_F(ListenerStatusMonitorTest, ListenerStatusMonitorTest016, testing::ext::TestSize.Level3)
{
    auto monitor = std::make_shared<ListenerStatusMonitor>(TEST_CAPACITY, TEST_INTERVAL);
    SysEventRule rule1(TEST_EVENT_DOMAIN, TEST_EVENT_NAME, TEST_EVENT_TAG, 0, 0);
    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME, {rule1}), true);
    ListenerCallerMap callers;
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 1); // 1 caller

    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME, {rule1, rule1}), true);
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 2); // 2 callers

    monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(TEST_UID, TEST_NAME,
        {rule1, rule1, rule1}), true);
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), 3); // 3 callers
}

/**
 * @tc.name: ListenerStatusMonitorTest017
 * @tc.desc: Test max capacity callers, multiple callers
 * @tc.type: FUNC
 * @tc.require: issue3048
 */
HWTEST_F(ListenerStatusMonitorTest, ListenerStatusMonitorTest017, testing::ext::TestSize.Level3)
{
    auto monitor = std::make_shared<ListenerStatusMonitor>(TEST_CAPACITY, TEST_INTERVAL);
    for (auto i = 0; i <= TEST_CAPACITY; i++) {
        auto testUid = TEST_UID + i;
        monitor->RecordAddListener(ListenerStatusUtil::GetListenerCallerInfo(testUid, TEST_NAME), true);
    }
    ListenerCallerMap callers;
    GetCallers(monitor, callers);
    ASSERT_EQ(callers.size(), TEST_CAPACITY);
}
} // HiviewDFX
} // OHOS
