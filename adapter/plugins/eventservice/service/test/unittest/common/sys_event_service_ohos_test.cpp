/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "sys_event_service_ohos_test.h"

#include <cstdlib>
#include <semaphore.h>
#include <string>
#include <vector>

#include "ash_mem_utils.h"
#include "event.h"
#include "event_query_wrapper_builder.h"
#include "file_util.h"
#include "hiview_global.h"
#include "if_system_ability_manager.h"
#include "ipc_skeleton.h"
#include "iquery_sys_event_callback.h"
#include "iservice_registry.h"
#include "isys_event_callback.h"
#include "isys_event_service.h"
#include "query_argument.h"
#include "query_sys_event_callback_proxy.h"
#include "plugin.h"
#include "ret_code.h"
#include "running_status_log_util.h"
#include "string_ex.h"
#include "sys_event.h"
#include "sys_event_rule.h"
#include "sys_event_service_adapter.h"
#include "sys_event_service_ohos.h"
#include "system_ability.h"
#include "string_ex.h"
#include "string_util.h"
#include "sys_event_callback_proxy.h"
#include "sys_event_service_stub.h"
#include "time_util.h"

using namespace std;

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr char ASH_MEM_NAME[] = "TestSharedMemory";
constexpr int32_t ASH_MEM_SIZE = 1024 * 2; // 2K
constexpr char TEST_LOG_DIR[] = "/data/log/hiview/sys_event_test";
const std::vector<int> EVENT_TYPES = {1, 2, 3, 4}; // FAULT = 1, STATISTIC = 2 SECURITY = 3, BEHAVIOR = 4

sptr<Ashmem> GetAshmem()
{
    auto ashmem = Ashmem::CreateAshmem(ASH_MEM_NAME, ASH_MEM_SIZE);
    if (ashmem == nullptr) {
        return nullptr;
    }
    if (!ashmem->MapReadAndWriteAshmem()) {
        return ashmem;
    }
    return ashmem;
}

class TestSysEventServiceStub : public SysEventServiceStub {
public:
    TestSysEventServiceStub() {}
    virtual ~TestSysEventServiceStub() {}

    ErrCode AddListener(const std::vector<SysEventRule>& rules, const sptr<ISysEventCallback>& callback)
    {
        return 0;
    }

    ErrCode RemoveListener(const sptr<ISysEventCallback>& callback)
    {
        return 0;
    }

    ErrCode Query(const QueryArgument& queryArgument, const std::vector<SysEventQueryRule>& rules,
        const sptr<IQuerySysEventCallback>& callback)
    {
        return 0;
    }

    ErrCode AddSubscriber(const std::vector<SysEventQueryRule> &rules, int64_t& funcResult)
    {
        return TimeUtil::GetMilliseconds();
    }

    ErrCode RemoveSubscriber()
    {
        return 0;
    }

    ErrCode Export(const QueryArgument &queryArgument, const std::vector<SysEventQueryRule> &rules, int64_t& funcResult)
    {
        return TimeUtil::GetMilliseconds();
    }

public:
    enum Code {
        DEFAULT = -1,
        ADD_SYS_EVENT_LISTENER = 0,
        REMOVE_SYS_EVENT_LISTENER,
        QUERY_SYS_EVENT,
        SET_DEBUG_MODE,
        ADD_SYS_EVENT_SUBSCRIBER,
        REMOVE_SYS_EVENT_SUBSCRIBER,
        EXPORT_SYS_EVENT
    };
};

class HiviewTestContext : public HiviewContext {
public:
    std::string GetHiViewDirectory(DirectoryType type __UNUSED)
    {
        return TEST_LOG_DIR;
    }
};
}

void SysEventServiceOhosTest::SetUpTestCase() {}

void SysEventServiceOhosTest::TearDownTestCase() {}

void SysEventServiceOhosTest::SetUp() {}

void SysEventServiceOhosTest::TearDown()
{
    (void)FileUtil::ForceRemoveDirectory(TEST_LOG_DIR);
}

static SysEventRule GetTestRule(int type, const string &domain, const string &eventName)
{
    SysEventRule rule;
    rule.ruleType = type;
    rule.domain = domain;
    rule.eventName = eventName;
    return rule;
}

static vector<SysEventRule> GetTestRules(int type, const string &domain, const string &eventName)
{
    vector<SysEventRule> rules;
    rules.push_back(GetTestRule(type, domain, eventName));
    return rules;
}

/**
 * @tc.name: SysEventServiceAdapterTest
 * @tc.desc: test apis of SysEventServiceAdapter
 * @tc.type: FUNC
 * @tc.require: issueI62BDW
 */
HWTEST_F(SysEventServiceOhosTest, SysEventServiceAdapterTest, testing::ext::TestSize.Level3)
{
    OHOS::HiviewDFX::SysEventServiceAdapter::StartService(nullptr);
    std::shared_ptr<SysEvent> sysEvent = nullptr;
    OHOS::HiviewDFX::SysEventServiceAdapter::OnSysEvent(sysEvent);
    SysEventCreator sysEventCreator("DEMO", "EVENT_NAME", SysEventCreator::FAULT);
    std::vector<int> values = {1, 2, 3};
    sysEventCreator.SetKeyValue("KEY", values);
    sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    OHOS::HiviewDFX::SysEventServiceAdapter::OnSysEvent(sysEvent);
    ASSERT_TRUE(true);
}

/**
 * @tc.name: TestAshMemory
 * @tc.desc: Ashmemory test
 * @tc.type: FUNC
 * @tc.require: issueI62WJT
 */
HWTEST_F(SysEventServiceOhosTest, TestAshMemory, testing::ext::TestSize.Level1)
{
    MessageParcel msgParcel;
    std::vector<std::u16string> from = {
        Str8ToStr16(std::string("11")),
        Str8ToStr16(std::string("22")),
    };
    auto result = AshMemUtils::WriteBulkData(msgParcel, from);
    ASSERT_TRUE(result != nullptr);
    std::vector<std::u16string> to;
    auto result1 = AshMemUtils::ReadBulkData(msgParcel, to);
    ASSERT_TRUE(result1);
    ASSERT_TRUE(from.size() == to.size());
    ASSERT_TRUE(Str16ToStr8(to[0]) == "11" && Str16ToStr8(to[1]) == "22");
    AshMemUtils::CloseAshmem(nullptr);
    ASSERT_TRUE(true);
    AshMemUtils::CloseAshmem(GetAshmem());
    ASSERT_TRUE(true);
}

/**
 * @tc.name: TestSysEventService001
 * @tc.desc: SysEventServiceStub test
 * @tc.type: FUNC
 * @tc.require: issueI62WJT
 */
HWTEST_F(SysEventServiceOhosTest, TestSysEventService001, testing::ext::TestSize.Level1)
{
    SysEventServiceStub* sysEventService = new(std::nothrow) TestSysEventServiceStub();
    MessageParcel data, reply;
    MessageOption option;
    sysEventService->OnRemoteRequest(TestSysEventServiceStub::Code::DEFAULT, data, reply, option);
    ASSERT_TRUE(true);
    sysEventService->OnRemoteRequest(TestSysEventServiceStub::Code::ADD_SYS_EVENT_LISTENER, data, reply, option);
    ASSERT_TRUE(true);
    sysEventService->OnRemoteRequest(TestSysEventServiceStub::Code::REMOVE_SYS_EVENT_LISTENER, data, reply,
        option);
    ASSERT_TRUE(true);
    sysEventService->OnRemoteRequest(TestSysEventServiceStub::Code::QUERY_SYS_EVENT, data, reply, option);
    ASSERT_TRUE(true);
    sysEventService->OnRemoteRequest(TestSysEventServiceStub::Code::SET_DEBUG_MODE, data, reply, option);
    ASSERT_TRUE(true);
}

/**
 * @tc.name: TestSysEventService002
 * @tc.desc: SysEventServiceStub test
 * @tc.type: FUNC
 * @tc.require: SR000I1G42
 */
HWTEST_F(SysEventServiceOhosTest, TestSysEventService002, testing::ext::TestSize.Level1)
{
    SysEventServiceStub* sysEventService = new(std::nothrow) TestSysEventServiceStub();
    MessageParcel data, reply;
    MessageOption option;
    sysEventService->OnRemoteRequest(TestSysEventServiceStub::Code::ADD_SYS_EVENT_SUBSCRIBER, data, reply, option);
    ASSERT_TRUE(true);
    sysEventService->OnRemoteRequest(TestSysEventServiceStub::Code::REMOVE_SYS_EVENT_SUBSCRIBER, data, reply, option);
    ASSERT_TRUE(true);
    sysEventService->OnRemoteRequest(TestSysEventServiceStub::Code::EXPORT_SYS_EVENT, data, reply, option);
    ASSERT_TRUE(true);
}

/**
 * @tc.name: MarshallingTAndUnmarshallingTest
 * @tc.desc: Unmarshalling test
 * @tc.type: FUNC
 * @tc.require: issueI62WJT
 */
HWTEST_F(SysEventServiceOhosTest, MarshallingTAndUnmarshallingTest, testing::ext::TestSize.Level1)
{
    long long defaultTimeStap = -1;
    int queryCount = 10;
    OHOS::HiviewDFX::QueryArgument argument(defaultTimeStap, defaultTimeStap, queryCount);
    MessageParcel parcel1;
    auto ret = argument.Marshalling(parcel1);
    ASSERT_TRUE(ret);
    QueryArgument* argsPtr = argument.Unmarshalling(parcel1);
    ASSERT_TRUE(argsPtr != nullptr && argsPtr->maxEvents == 10 && argsPtr->beginTime == -1);
    OHOS::HiviewDFX::SysEventRule rule("DOMAIN1", "EVENT_NAME2", "TAG3", OHOS::HiviewDFX::RuleType::WHOLE_WORD);
    MessageParcel parcel2;
    ret = rule.Marshalling(parcel2);
    ASSERT_TRUE(ret);
    OHOS::HiviewDFX::SysEventRule* rulePtr = rule.Unmarshalling(parcel2);
    ASSERT_TRUE(rulePtr != nullptr && rulePtr->domain == "DOMAIN1" &&
        rulePtr->eventName == "EVENT_NAME2" && rulePtr->tag == "TAG3");

    std::vector<std::string> eventNames { "EVENT_NAME1", "EVENT_NAME2" };
    OHOS::HiviewDFX::SysEventQueryRule eventQueryRule("DOMAIN", eventNames);
    MessageParcel parcel3;
    ret = eventQueryRule.Marshalling(parcel3);
    ASSERT_TRUE(ret);
    OHOS::HiviewDFX::SysEventQueryRule* eventQueryRulePtr = eventQueryRule.Unmarshalling(parcel3);
    ASSERT_TRUE(eventQueryRulePtr != nullptr && eventQueryRulePtr->domain == "DOMAIN" &&
        eventQueryRulePtr->eventList.size() == 2 && eventQueryRulePtr->eventList[0] == "EVENT_NAME1");
}

/**
 * @tc.name: RunningStatusLogUtilTest
 * @tc.desc: Test apis of RunningStatusLogUtil
 * @tc.type: FUNC
 * @tc.require: issueI62WJT
 */
HWTEST_F(SysEventServiceOhosTest, RunningStatusLogUtilTest, testing::ext::TestSize.Level1)
{
    HiviewTestContext hiviewTestContext;
    HiviewGlobal::CreateInstance(hiviewTestContext);
    std::vector<OHOS::HiviewDFX::SysEventQueryRule> queryRules;
    std::vector<std::string> eventNames { "EVENT_NAME1", "EVENT_NAME2" };
    OHOS::HiviewDFX::SysEventQueryRule queryRule("DOMAIN", eventNames);
    RunningStatusLogUtil::LogTooManyQueryRules(queryRules);
    ASSERT_TRUE(true);
    queryRules.emplace_back(queryRule);
    RunningStatusLogUtil::LogTooManyQueryRules(queryRules);
    ASSERT_TRUE(true);
    vector<SysEventRule> sysEventRules1;
    RunningStatusLogUtil::LogTooManyWatchRules(sysEventRules1);
    ASSERT_TRUE(true);
    vector<SysEventRule> sysEventRules2 = GetTestRules(1, "", "");
    RunningStatusLogUtil::LogTooManyWatchRules(sysEventRules2);
    ASSERT_TRUE(true);
    RunningStatusLogUtil::LogTooManyWatchers(30);
}

/**
 * @tc.name: ConditionParserTest01
 * @tc.desc: Test apis of ConditionParser
 * @tc.type: FUNC
 * @tc.require: issueI62WJT
 */
HWTEST_F(SysEventServiceOhosTest, ConditionParserTest01, testing::ext::TestSize.Level1)
{
    OHOS::HiviewDFX::ConditionParser parser;
    EventStore::Cond cond;
    std::string condStr = R"~({"version":"V1", "condition":{"and":[{"param":"NAME", "op":"=",
        "value":"SysEventService"}]}})~";
    auto ret = parser.ParseCondition(condStr, cond);
    ASSERT_TRUE(ret);
    std::string condStr2 = R"~({"version":"V1", "condition":{"and":[{"param":"NAME", "op":"=",
        "value":"SysEventService"}, {"param":"uid_", "op":"=", "value":1201}]}})~";
    ret = parser.ParseCondition(condStr2, cond);
    ASSERT_TRUE(ret);

    std::string condStr3 = R"~({"version":"V1", "condition":{"and":[{"param":"type_", "op":">", "value":0},
        {"param":"uid_", "op":"=", "value":1201}]}})~";
    ret = parser.ParseCondition(condStr3, cond);
    ASSERT_TRUE(ret);

    std::string condStr4 = R"~({"version":"V1", "condition":{"and":[{"param1":"type_", "op":">", "value":0},
        {"param2":"uid_", "op":"=", "value":1201}]}})~";
    ret = parser.ParseCondition(condStr4, cond);
    ASSERT_TRUE(!ret);

    std::string condSt5 = R"~({"version":"V1", "condition":{"and":[{"param":"", "op":">", "value":0},
        {"param":"", "op":"=", "value":1201}]}})~";
    ret = parser.ParseCondition(condSt5, cond);
    ASSERT_TRUE(!ret);

    std::string condSt6 = R"~({"version":"V1", "condition":{"and":[{"param":"type_", "op1":">", "value":0},
        {"param":"uid_", "op2":"=", "value":1201}]}})~";
    ret = parser.ParseCondition(condSt6, cond);
    ASSERT_TRUE(!ret);
}

/**
 * @tc.name: ConditionParserTest02
 * @tc.desc: Test apis of ConditionParser
 * @tc.type: FUNC
 * @tc.require: issueI62WJT
 */
HWTEST_F(SysEventServiceOhosTest, ConditionParserTest02, testing::ext::TestSize.Level1)
{
    OHOS::HiviewDFX::ConditionParser parser;
    EventStore::Cond cond;
    std::string condSt7 = R"~({"version":"V1", "condition":{"and":[{"param":"type_", "op":">", "value11":0},
        {"param":"uid_", "op":"=", "value2":1201}]}})~";
    auto ret = parser.ParseCondition(condSt7, cond);
    ASSERT_TRUE(!ret);

    std::string condStr8 = R"~({"version":"V1", "condition":{"and":[{"param":"type_", "op":">", "value":[]},
        {"param":"uid_", "op":"=", "value":[]}]}})~";
    ret = parser.ParseCondition(condStr8, cond);
    ASSERT_TRUE(!ret);

    std::string condStr9 = R"~({"version":"V1", "condition1":{"and":[{"param":"type_", "op":">", "value":0},
        {"param":"uid_", "op":"=", "value":1201}]}})~";
    ret = parser.ParseCondition(condStr9, cond);
    ASSERT_TRUE(!ret);

    std::string condStr10 = R"~({"version":"V1", "condition":1})~";
    ret = parser.ParseCondition(condStr10, cond);
    ASSERT_TRUE(!ret);

    std::string condStr11 = R"~({"version":"V2", "condition":{"and":[{"param1":"type_", "op":">", "value":0},
        {"param2":"uid_", "op":"=", "value":1201}]}})~";
    ret = parser.ParseCondition(condStr11, cond);
    ASSERT_TRUE(!ret);
}

/**
 * @tc.name: QueryWrapperTest01
 * @tc.desc: BUild query wrapper with all event types
 * @tc.type: FUNC
 * @tc.require: issueI62WJT
 */
HWTEST_F(SysEventServiceOhosTest, QueryWrapperTest01, testing::ext::TestSize.Level1)
{
    HiviewTestContext hiviewTestContext;
    HiviewGlobal::CreateInstance(hiviewTestContext);
    QueryArgument queryArgument1(-1, -1, 10);
    auto queryWrapperBuilder1 = std::make_shared<EventQueryWrapperBuilder>(queryArgument1);
    QueryArgument queryArgument2(-1, -1, 10, 1, 20);
    auto queryWrapperBuilder2 = std::make_shared<EventQueryWrapperBuilder>(queryArgument2);
    auto queryWrapper1 = queryWrapperBuilder1->Build();
    ASSERT_TRUE(queryWrapper1 != nullptr);
    auto queryWrapper2 = queryWrapperBuilder2->Build();
    ASSERT_TRUE(queryWrapper2 != nullptr);

    ASSERT_FALSE(queryWrapperBuilder1->IsValid());
    ASSERT_FALSE(queryWrapperBuilder2->IsValid());
    queryWrapperBuilder1->Append("DOMAIN1", "EVENTNAME1", 0, "");
    queryWrapperBuilder2->Append("DOMAIN2", "EVENTNAME2", 0, "");
    ASSERT_TRUE(queryWrapperBuilder1->IsValid());
    ASSERT_TRUE(queryWrapperBuilder2->IsValid());
}

/**
 * @tc.name: QueryWrapperTest02
 * @tc.desc: BUild query wrapper with domain, event name and event type.
 * @tc.type: FUNC
 * @tc.require: issueI62WJT
 */
HWTEST_F(SysEventServiceOhosTest, QueryWrapperTest02, testing::ext::TestSize.Level1)
{
    HiviewTestContext hiviewTestContext;
    HiviewGlobal::CreateInstance(hiviewTestContext);
    QueryArgument queryArgument1(-1, -1, 10);
    auto queryWrapperBuilder = std::make_shared<EventQueryWrapperBuilder>(queryArgument1);
    queryWrapperBuilder->Append("DOMAIN1", "EVENTNAME1", 1, R"~({"version":"V1", "condition":
        {"and":[{"param":"NAME", "op":"=", "value":"SysEventService"}]}})~");
    queryWrapperBuilder->Append("DOMAIN2", "EVENTNAME2", 3, R"~({"version":"V1", "condition":
        {"and":[{"param":"NAME", "op":"=", "value":"SysEventService"}]}})~");
    auto queryWrapper = queryWrapperBuilder->Build();
    ASSERT_TRUE(queryWrapper != nullptr);
}
} // namespace HiviewDFX
} // namespace OHOS