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
#include "sys_event_dao_test.h"

#include <chrono>
#include <ctime>
#include <iostream>
#include <limits>
#include <memory>
#include <thread>

#include <gmock/gmock.h>
#include "event.h"
#include "hiview_platform.h"
#include "sys_event.h"
#include "sys_event_dao.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
const char TEST_LEVEL[] = "MINOR";
constexpr int32_t TEST_INT32_VALUE = 1;
constexpr int64_t TEST_INT64_VALUE = 1;
constexpr double TEST_DOU_VALUE = 123.456;
const std::string TEST_STR_VALUE = "test";
}
void SysEventDaoTest::SetUpTestCase()
{
    OHOS::HiviewDFX::HiviewPlatform &platform = HiviewPlatform::GetInstance();
    std::string defaultDir = "/data/test/test_data/hiview_platform_config";
    if (!platform.InitEnvironment(defaultDir)) {
        std::cout << "fail to init environment" << std::endl;
    } else {
        std::cout << "init environment successful" << std::endl;
    }
}

void SysEventDaoTest::TearDownTestCase()
{
}

void SysEventDaoTest::SetUp()
{
}

void SysEventDaoTest::TearDown()
{
}

/**
 * @tc.name: TestSysEventDaoInsert_001
 * @tc.desc: save event to doc store
 * @tc.type: FUNC
 * @tc.require: AR000FT2Q3
 * @tc.author: zhouhaifeng
 */
HWTEST_F(SysEventDaoTest, TestSysEventDaoInsert_001, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create pipeline event and set event id
     * @tc.steps: step2. invoke OnEvent func
     * @tc.expected: all ASSERT_TRUE work through.
     */
    std::string jsonStr = R"~({"domain_":"demo", "name_":"SysEventDaoTest_001", "type_":1, "tz_":8,
        "time_":1620271291188, "pid_":6527, "tid_":6527, "traceid_":"f0ed5160bb2df4b", "spanid_":"10", "pspanid_":"20",
        "trace_flag_":4, "keyBool":1, "keyChar":97, "keyShort":-100, "keyInt":-200, "KeyLong":-300, "KeyLongLong":-400,
        "keyUnsignedChar":97, "keyUnsignedShort":100, "keyUnsignedInt":200, "keyUnsignedLong":300,
        "keyUnsignedLongLong":400, "keyFloat":1.1, "keyDouble":2.2, "keyString1":"abc", "keyString2":"efg",
        "keyBools":[1, 1, 0], "keyChars":[97, 98, 99], "keyUnsignedChars":[97, 98, 99], "keyShorts":[-100, -200, -300],
        "keyUnsignedShorts":[100, 200, 300], "keyInts":[-1000, -2000, -3000], "keyUnsignedInts":[1000, 2000, 3000],
        "keyLongs":[-10000, -20000, -30000], "keyUnsignedLongs":[10000, 20000, 30000],
        "keyLongLongs":[-100000, -200000, -300000], "keyUnsignedLongLongs":[100000, 200000, 300000],
        "keyFloats":[1.1, 2.2, 3.3], "keyDoubles":[10.1, 20.2, 30.3], "keyStrings":["a", "b", "c"]})~";
    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, jsonStr);
    sysEvent->SetLevel(TEST_LEVEL);
    sysEvent->SetEventSeq(0);
    int retCode = EventStore::SysEventDao::Insert(sysEvent);
    ASSERT_TRUE(retCode == 0);
}

/**
 * @tc.name: TestEventDaoQuery_002
 * @tc.desc: query event from doc store
 * @tc.type: FUNC
 * @tc.require: AR000FT2PO
 * @tc.author: zhouhaifeng
 */
HWTEST_F(SysEventDaoTest, TestEventDaoQuery_002, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create pipeline event and set event id
     * @tc.steps: step2. invoke OnEvent func
     * @tc.expected: all ASSERT_TRUE work through.
     */
    std::string jsonStr1 = R"~({"domain_":"demo", "name_":"SysEventDaoTest_002", "type_":1, "tz_":8,
        "time_":162027129100, "pid_":6527, "tid_":6527, "traceid_":"f0ed5160bb2df4b", "spanid_":"10", "pspanid_":"20",
        "trace_flag_":4, "keyBool":1, "keyChar":97, "keyShort":-100, "keyInt":-200, "KeyLong":-300, "KeyLongLong":-400,
        "keyUnsignedChar":97, "keyUnsignedShort":100, "keyUnsignedInt":200, "keyUnsignedLong":300,
        "keyUnsignedLongLong":400, "keyFloat":1.1, "keyDouble":2.2, "keyString1":"abc", "keyString2":"efg",
        "keyBools":[1, 1, 0], "keyChars":[97, 98, 99], "keyUnsignedChars":[97, 98, 99], "keyShorts":[-100, -200, -300],
        "keyUnsignedShorts":[100, 200, 300], "keyInts":[-1000, -2000, 3000], "keyUnsignedInts":[1000, 2000, 3000],
        "keyLongs":[-10000, -20000, -30000], "keyUnsignedLongs":[10000, 20000, 30000],
        "keyLongLongs":[-100000, -200000, -300000], "keyUnsignedLongLongs":[100000, 200000, 300000],
        "keyFloats":[1.1, 2.2, 3.3], "keyDoubles":[10.1, 20.2, 30.3], "keyStrings":["a", "b", "c"]})~";
    auto sysEvent1 = std::make_shared<SysEvent>("SysEventSource", nullptr, jsonStr1);
    sysEvent1->SetLevel(TEST_LEVEL);
    sysEvent1->SetEventSeq(1); // 1: test seq
    int retCode1 = EventStore::SysEventDao::Insert(sysEvent1);
    ASSERT_TRUE(retCode1 == 0);

    std::string jsonStr2 = R"~({"domain_":"demo", "name_":"SysEventDaoTest_002", "type_":1, "tz_":8,
        "time_":162027129110, "pid_":6527, "tid_":6527, "traceid_":"f0ed5160bb2df4b", "spanid_":"10", "pspanid_":"20",
        "trace_flag_":4, "keyBool":1, "keyChar":97, "keyShort":-100, "keyInt":-200, "KeyLong":-300, "KeyLongLong":-400,
        "keyUnsignedChar":97, "keyUnsignedShort":100, "keyUnsignedInt":200, "keyUnsignedLong":300,
        "keyUnsignedLongLong":400, "keyFloat":1.1, "keyDouble":2.2, "keyString1":"abc", "keyString2":"efg",
        "keyBools":[1, 1, 0], "keyChars":[97, 98, 99], "keyUnsignedChars":[97, 98, 99], "keyShorts":[-100, -200, -300],
        "keyUnsignedShorts":[100, 200, 300], "keyInts":[-1000, -2000, -3000], "keyUnsignedInts":[1000, 2000, 3000],
        "keyLongs":[-10000, -20000, -30000], "keyUnsignedLongs":[10000, 20000, 30000],
        "keyLongLongs":[-100000, -200000, -300000], "keyUnsignedLongLongs":[100000, 200000, 300000],
        "keyFloats":[1.1, 2.2, 3.3], "keyDoubles":[10.1, 20.2, 30.3], "keyStrings":["a", "b", "c"]})~";
    auto sysEvent2 = std::make_shared<SysEvent>("SysEventSource", nullptr, jsonStr2);
    sysEvent2->SetLevel(TEST_LEVEL);
    sysEvent2->SetEventSeq(3); // 3: test seq
    int retCode2 = EventStore::SysEventDao::Insert(sysEvent2);
    ASSERT_TRUE(retCode2 == 0);

    auto sysEventQuery = EventStore::SysEventDao::BuildQuery("demo", {"SysEventDaoTest_002"});
    std::vector<std::string> selections { EventStore::EventCol::TS };
    EventStore::ResultSet resultSet = sysEventQuery->Select(selections).
        Where(EventStore::EventCol::TS, EventStore::Op::EQ, 162027129100).Execute();
    int count = 0;
    while (resultSet.HasNext()) {
        count++;
        EventStore::ResultSet::RecordIter it = resultSet.Next();
        ASSERT_TRUE(it->GetSeq() == sysEvent1->GetEventSeq());
        std::cout << "seq=" << it->GetSeq() << ", json=" << it->AsJsonStr() << std::endl;
    }
    ASSERT_TRUE(count >= 1);
}

/**
 * @tc.name: TestEventDaoQuery_003
 * @tc.desc: query event from doc store
 * @tc.type: FUNC
 * @tc.require: issue
 */
HWTEST_F(SysEventDaoTest, TestEventDaoQuery_003, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create pipeline event and set event id
     * @tc.steps: step2. invoke OnEvent func
     * @tc.expected: all ASSERT_TRUE work through.
     */
    std::string jsonStr1 = R"~({"domain_":"DEMO", "name_":"SYS_EVENT_DAO_TEST", "type_":1, "tz_":8,
        "time_":162027129100, "pid_":1201, "tid_":1201, "uid_":1201, "KEY_INT":-200, "KEY_DOUBLE":2.2,
        "KEY_STR":"abc"})~";
    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, jsonStr1);
    sysEvent->SetLevel(TEST_LEVEL);
    constexpr int64_t testSeq = 1;
    sysEvent->SetEventSeq(testSeq);
    int retCode = EventStore::SysEventDao::Insert(sysEvent);
    ASSERT_TRUE(retCode == 0);

    auto sysEventQuery = EventStore::SysEventDao::BuildQuery("DEMO", {"SYS_EVENT_DAO_TEST"});
    EventStore::ResultSet resultSet = sysEventQuery->Where(EventStore::EventCol::SEQ, EventStore::Op::EQ, testSeq).
        Where(EventStore::EventCol::PID, EventStore::Op::EQ, 1201). //1201 test pid value
        Where(EventStore::EventCol::TID, EventStore::Op::EQ, 1201). //1201 test tid value
        Where(EventStore::EventCol::UID, EventStore::Op::EQ, 1201). //1201 test uid value
        Where("KEY_INT", EventStore::Op::EQ, -200). // test int value -200
        Where("KEY_DOUBLE", EventStore::Op::EQ, 2.2). // test double value 2.2
        Where("KEY_STR", EventStore::Op::EQ, "abc").
        Execute();
    int count = 0;
    while (resultSet.HasNext()) {
        count++;
        EventStore::ResultSet::RecordIter it = resultSet.Next();
        ASSERT_TRUE(it->GetSeq() == sysEvent->GetEventSeq());
        std::cout << "event json=" << it->AsJsonStr() << std::endl;
    }
    ASSERT_TRUE(count >= 1);
}

/**
 * @tc.name: TestEventDaoQuery_004
 * @tc.desc: test embed sql
 * @tc.type: FUNC
 * @tc.require: AR000FT2Q5
 * @tc.author: zhouhaifeng
 */
HWTEST_F(SysEventDaoTest, TestEventDaoQuery_004, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create pipeline event and set event id
     * @tc.steps: step2. invoke OnEvent func
     * @tc.expected: all ASSERT_TRUE work through.
     */
    auto sysEventQuery = EventStore::SysEventDao::BuildQuery("dA", {"e11", "e12", "e13"});
    EventStore::Cond timeCond(EventStore::EventCol::TS, EventStore::Op::GE, 100);
    timeCond.And(EventStore::EventCol::TS, EventStore::Op::LT, 999);
    sysEventQuery->Where(timeCond);
    EventStore::ResultSet resultSet = sysEventQuery->Execute();
    int count = 0;
    while (resultSet.HasNext()) {
        count++;
    }
    ASSERT_TRUE(count == 0);
}

/**
 * @tc.name: TestEventDaoQuery_005
 * @tc.desc: test embed sql
 * @tc.type: FUNC
 * @tc.require: AR000FT2Q3
 * @tc.author: zhouhaifeng
 */
HWTEST_F(SysEventDaoTest, TestEventDaoQuery_005, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create pipeline event and set event id
     * @tc.steps: step2. invoke OnEvent func
     * @tc.expected: all ASSERT_TRUE work through.
     */
    auto sysEventQuery = EventStore::SysEventDao::BuildQuery("d1", {"e1"});
    sysEventQuery->And(EventStore::EventCol::TS, EventStore::Op::EQ, 100)
        .And(EventStore::EventCol::TS, EventStore::Op::EQ, 100.1f);
    EventStore::ResultSet resultSet = sysEventQuery->Execute();
    int count = 0;
    while (resultSet.HasNext()) {
        count++;
    }
    ASSERT_TRUE(count == 0);
}

/**
 * @tc.name: TestEventDaoQuery_008
 * @tc.desc: test query in different threads (more than 4) at the same time.
 * @tc.type: FUNC
 * @tc.require: issueI5L2RV
 */
HWTEST_F(SysEventDaoTest, TestEventDaoQuery_008, testing::ext::TestSize.Level3)
{
    int threadCount = 5;
    EventStore::DbQueryStatus queryStatus = EventStore::DbQueryStatus::SUCCEED;
    for (int i = 0; i < threadCount; i++) {
        std::thread t([&queryStatus] () {
            auto sysEventQuery = EventStore::SysEventDao::BuildQuery("d1", {"e1"});
            int queryCount = 10;
            (void)(sysEventQuery->Execute(queryCount, { true, false }, std::make_pair(EventStore::INNER_PROCESS_ID, ""),
                [&queryStatus] (EventStore::DbQueryStatus status) {
                    if (status != EventStore::DbQueryStatus::SUCCEED) {
                        queryStatus = status;
                    }
                }));
        });
        t.join();
    }
    ASSERT_TRUE((queryStatus == EventStore::DbQueryStatus::CONCURRENT) ||
        (queryStatus == EventStore::DbQueryStatus::SUCCEED));
}

/**
 * @tc.name: TestEventDaoQuery_009
 * @tc.desc: test query with over limit
 * @tc.type: FUNC
 * @tc.require: issueI5L2RV
 */
HWTEST_F(SysEventDaoTest, TestEventDaoQuery_009, testing::ext::TestSize.Level3)
{
    auto sysEventQuery = EventStore::SysEventDao::BuildQuery("d1", {"e1"});
    EventStore::DbQueryStatus queryStatus = EventStore::DbQueryStatus::SUCCEED;
    int queryCount = 51;
    (void)(sysEventQuery->Execute(queryCount, { true, true }, std::make_pair(EventStore::INNER_PROCESS_ID, ""),
        [&queryStatus] (EventStore::DbQueryStatus status) {
            if (status != EventStore::DbQueryStatus::SUCCEED) {
                queryStatus = status;
            }
        }));
    ASSERT_EQ(queryStatus, EventStore::DbQueryStatus::OVER_LIMIT);
}

/**
 * @tc.name: TestEventDaoQuery_010
 * @tc.desc: test query in high frequency, query 51 times in 1 second
 * @tc.type: FUNC
 * @tc.require: issueI5L2RV
 */
HWTEST_F(SysEventDaoTest, TestEventDaoQuery_010, testing::ext::TestSize.Level3)
{
    auto sysEventQuery = EventStore::SysEventDao::BuildQuery("d1", {"e1"});
    int queryCount = 10;
    const int threshhold = 50;
    const int delayDuration = 1; // 1 second
    for (int i = 0; i < 2; i++) { // 2 cycles
        sleep(delayDuration);
        for (int j = 0; j <= threshhold; j++) { // more than 50 queries in 1 second is never allowed
            EventStore::DbQueryStatus queryStatus = EventStore::DbQueryStatus::SUCCEED;
            (void)sysEventQuery->Execute(queryCount, { true, true }, std::make_pair(EventStore::INNER_PROCESS_ID, ""),
                [&queryStatus] (EventStore::DbQueryStatus status) {
                    if (status != EventStore::DbQueryStatus::SUCCEED) {
                        queryStatus = status;
                    }
                });
            ASSERT_TRUE((queryStatus == EventStore::DbQueryStatus::TOO_FREQENTLY) ||
                (queryStatus == EventStore::DbQueryStatus::SUCCEED));
        }
    }
}

/**
 * @tc.name: TestEventDaoQuery_011
 * @tc.desc: compare two result sets from different queries
 * @tc.type: FUNC
 * @tc.require: issueI8QSH0
 */
HWTEST_F(SysEventDaoTest, TestEventDaoQuery_011, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create pipeline event and set event id
     * @tc.steps: step2. invoke OnEvent func
     * @tc.expected: all ASSERT_TRUE work through.
     */
    std::string jsonStr1 = R"~({"domain_":"DEMO", "name_":"SYS_EVENT_DAO_TEST", "type_":1, "tz_":8,
        "time_":162027129100, "pid_":1201, "tid_":1201, "uid_":1201, "KEY_INT":-200, "KEY_DOUBLE":2.2,
        "KEY_STR":"abc"})~";
    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, jsonStr1);
    sysEvent->SetLevel(TEST_LEVEL);
    constexpr int64_t testSeq = 1;
    sysEvent->SetEventSeq(testSeq);
    int retCode = EventStore::SysEventDao::Insert(sysEvent);
    ASSERT_TRUE(retCode == 0);

    auto sysEventQuery1 = EventStore::SysEventDao::BuildQuery("DEMO", {"SYS_EVENT_DAO_TEST"});
    EventStore::ResultSet resultSet1 = sysEventQuery1->Execute();
    int queryCnt = 10;
    auto sysEventQuery2 = EventStore::SysEventDao::BuildQuery("DEMO", {"SYS_EVENT_DAO_TEST"}, 1,
        std::numeric_limits<int64_t>::max(), queryCnt);
    EventStore::ResultSet resultSet2 = sysEventQuery2->Execute();
    EventStore::QueryExtraInfo info { std::numeric_limits<int64_t>::max(), queryCnt,
        EventStore::DEFAULT_REPORT_INTERVAL };
    auto sysEventQuery3 = EventStore::SysEventDao::BuildQuery("DEMO", {"SYS_EVENT_DAO_TEST"}, 1, info);
    EventStore::ResultSet resultSet3 = sysEventQuery3->Execute();

    while (resultSet1.HasNext() && resultSet2.HasNext() && resultSet3.HasNext()) {
        EventStore::ResultSet::RecordIter it1 = resultSet1.Next();
        EventStore::ResultSet::RecordIter it2 = resultSet2.Next();
        EventStore::ResultSet::RecordIter it3 = resultSet3.Next();
        ASSERT_EQ(it1->GetSeq(), it2->GetSeq());
        ASSERT_EQ(it2->GetSeq(), it3->GetSeq());
    }
}

/**
 * @tc.name: FieldValueTest_01
 * @tc.desc: test the constructor function of FieldValue.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, FieldValueTest_01, testing::ext::TestSize.Level0)
{
    using namespace EventStore;
    // default value
    FieldValue value1;
    ASSERT_TRUE(value1.IsNumber());
    ASSERT_TRUE(value1.Index() == FieldValue::NUMBER);
    ASSERT_EQ(value1.GetFieldNumber().GetNumber<int64_t>(), 0);
}

/**
 * @tc.name: FieldValueTest_02
 * @tc.desc: test the constructor function of FieldValue.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, FieldValueTest_02, testing::ext::TestSize.Level0)
{
    using namespace EventStore;
    // int32_t value
    FieldValue value2(TEST_INT32_VALUE);
    ASSERT_TRUE(value2.IsNumber());
    ASSERT_TRUE(value2.Index() == FieldValue::NUMBER);
    ASSERT_EQ(value2.GetFieldNumber().GetNumber<int64_t>(), TEST_INT32_VALUE);
}

/**
 * @tc.name: FieldValueTest_03
 * @tc.desc: test the constructor function of FieldValue.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, FieldValueTest_03, testing::ext::TestSize.Level0)
{
    using namespace EventStore;
    // int64_t value
    FieldValue value3(TEST_INT64_VALUE);
    ASSERT_TRUE(value3.IsNumber());
    ASSERT_TRUE(value3.Index() == FieldValue::NUMBER);
    ASSERT_EQ(value3.GetFieldNumber().GetNumber<int64_t>(), TEST_INT64_VALUE);
}

/**
 * @tc.name: FieldValueTest_04
 * @tc.desc: test the constructor function of FieldValue.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, FieldValueTest_04, testing::ext::TestSize.Level0)
{
    using namespace EventStore;
    // double value
    FieldValue value4(TEST_DOU_VALUE);
    ASSERT_TRUE(value4.IsNumber());
    ASSERT_TRUE(value4.Index() == FieldValue::NUMBER);
    ASSERT_EQ(value4.GetFieldNumber().GetNumber<double>(), TEST_DOU_VALUE);
}

/**
 * @tc.name: FieldValueTest_05
 * @tc.desc: test the constructor function of FieldValue.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, FieldValueTest_05, testing::ext::TestSize.Level0)
{
    using namespace EventStore;
    // string value
    FieldValue value5(TEST_STR_VALUE);
    ASSERT_TRUE(value5.IsString());
    ASSERT_TRUE(value5.Index() == FieldValue::STRING);
    ASSERT_EQ(value5.GetString(), TEST_STR_VALUE);
}

/**
 * @tc.name: FieldValueTest_06
 * @tc.desc: test the constructor function of FieldValue.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, FieldValueTest_06, testing::ext::TestSize.Level0)
{
    using namespace EventStore;
    // ==
    FieldValue iValue1(TEST_INT32_VALUE);
    FieldValue iValue2(TEST_INT32_VALUE);
    ASSERT_TRUE(iValue1 == iValue2);
    FieldValue dValue1(TEST_DOU_VALUE);
    FieldValue dValue2(TEST_DOU_VALUE);
    ASSERT_TRUE(dValue1 == dValue2);
    FieldValue sValue1(TEST_STR_VALUE);
    FieldValue sValue2(TEST_STR_VALUE);
    ASSERT_TRUE(sValue1 == sValue2);
    ASSERT_FALSE(iValue1 == dValue1);
}

/**
 * @tc.name: FieldValueTest_07
 * @tc.desc: test the constructor function of FieldValue.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, FieldValueTest_07, testing::ext::TestSize.Level0)
{
    using namespace EventStore;
    // !=
    FieldValue iValue1(TEST_INT32_VALUE);
    FieldValue iValue3(0);
    ASSERT_TRUE(iValue1 != iValue3);
    FieldValue dValue1(TEST_DOU_VALUE);
    FieldValue dValue3(12.3456);
    ASSERT_TRUE(dValue1 != dValue3);
    FieldValue sValue1(TEST_STR_VALUE);
    FieldValue sValue3("tes");
    ASSERT_TRUE(sValue1 != sValue3);
    ASSERT_TRUE(iValue1 != dValue1);
}

/**
 * @tc.name: FieldValueTest_08
 * @tc.desc: test the constructor function of FieldValue.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, FieldValueTest_08, testing::ext::TestSize.Level0)
{
    using namespace EventStore;
    // <
    FieldValue iValue1(TEST_INT32_VALUE);
    FieldValue iValue3(0);
    ASSERT_TRUE(iValue3 < iValue1);
    FieldValue dValue1(TEST_DOU_VALUE);
    FieldValue dValue3(12.3456);
    ASSERT_TRUE(dValue3 < dValue1);
    FieldValue sValue1(TEST_STR_VALUE);
    FieldValue sValue3("tes");
    ASSERT_TRUE(sValue3 < sValue1);
    ASSERT_TRUE(iValue1 < dValue1);
}

/**
 * @tc.name: FieldValueTest_09
 * @tc.desc: test the constructor function of FieldValue.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, FieldValueTest_09, testing::ext::TestSize.Level0)
{
    using namespace EventStore;
    // <=
    FieldValue iValue1(TEST_INT32_VALUE);
    FieldValue iValue3(0);
    ASSERT_TRUE(iValue3 <= iValue1);
    FieldValue dValue1(TEST_DOU_VALUE);
    FieldValue dValue3(12.3456);
    ASSERT_TRUE(dValue3 <= dValue1);
    FieldValue sValue1(TEST_STR_VALUE);
    FieldValue sValue3("tes");
    ASSERT_TRUE(sValue3 <= sValue1);
    ASSERT_TRUE(iValue1 <= dValue1);
}

/**
 * @tc.name: FieldValueTest_10
 * @tc.desc: test the constructor function of FieldValue.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, FieldValueTest_10, testing::ext::TestSize.Level0)
{
    using namespace EventStore;
    // >
    FieldValue iValue1(TEST_INT32_VALUE);
    FieldValue iValue3(0);
    ASSERT_TRUE(iValue1 > iValue3);
    FieldValue dValue1(TEST_DOU_VALUE);
    FieldValue dValue3(12.3456);
    ASSERT_TRUE(dValue1 > dValue3);
    FieldValue sValue1(TEST_STR_VALUE);
    FieldValue sValue3("tes");
    ASSERT_TRUE(sValue1 > sValue3);
    ASSERT_FALSE(iValue1 > dValue1);
}

/**
 * @tc.name: FieldValueTest_11
 * @tc.desc: test the constructor function of FieldValue.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, FieldValueTest_11, testing::ext::TestSize.Level0)
{
    using namespace EventStore;
    // >=
    FieldValue iValue1(TEST_INT32_VALUE);
    FieldValue iValue3(0);
    ASSERT_TRUE(iValue1 >= iValue3);
    FieldValue dValue1(TEST_DOU_VALUE);
    FieldValue dValue3(12.3456);
    ASSERT_TRUE(dValue1 >= dValue3);
    FieldValue sValue1(TEST_STR_VALUE);
    FieldValue sValue3("tes");
    ASSERT_TRUE(sValue1 >= sValue3);
    ASSERT_FALSE(iValue1 >= dValue1);
}

/**
 * @tc.name: FieldValueTest_12
 * @tc.desc: test the operator function of FieldValue.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, FieldValueTest_12, testing::ext::TestSize.Level0)
{
    using namespace EventStore;

    // IsStartWith / IsNotStartWith for string
    FieldValue iValue1(TEST_INT32_VALUE);
    FieldValue iValue2(TEST_INT32_VALUE);
    FieldValue sValue1(TEST_STR_VALUE);
    FieldValue sValue3("tes");
    ASSERT_FALSE(sValue1.IsStartWith(iValue1));
    ASSERT_FALSE(sValue1.IsNotStartWith(iValue1));
    ASSERT_FALSE(iValue1.IsStartWith(iValue2));
    ASSERT_FALSE(iValue1.IsNotStartWith(iValue2));
    ASSERT_TRUE(sValue1.IsStartWith(sValue3));
    ASSERT_FALSE(sValue1.IsNotStartWith(sValue3));
}

/**
 * @tc.name: FieldValueTest_13
 * @tc.desc: test the base functions of FieldValue.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, FieldValueTest_13, testing::ext::TestSize.Level0)
{
    using namespace EventStore;

    int8_t i8v1 = 1; // test value
    FieldValue fv1(i8v1);
    double dv2 = 1.0; // test value
    FieldValue fv2(dv2);
    uint64_t ui64v1 = 3; // test value
    FieldValue fv3(ui64v1);
    std::string content = "test";
    FieldValue fv4(content);

    ASSERT_TRUE(fv1.IsNumber());
    ASSERT_FALSE(fv1.IsString());
    ASSERT_TRUE(fv2.IsNumber());
    ASSERT_FALSE(fv2.IsString());
    ASSERT_TRUE(fv3.IsNumber());
    ASSERT_FALSE(fv3.IsString());
    ASSERT_TRUE(fv4.IsString());
    ASSERT_FALSE(fv4.IsNumber());
    ASSERT_TRUE(fv1.GetFieldNumber() == fv2.GetFieldNumber());
    ASSERT_EQ(fv1.GetString(), "");
    ASSERT_EQ(fv4.GetFieldNumber().GetNumber<int64_t>(), 0);
    ASSERT_EQ(fv4.GetString(), content);
    ASSERT_TRUE(fv1.Index() == FieldValue::NUMBER);
    ASSERT_TRUE(fv2.Index() == FieldValue::NUMBER);
    ASSERT_TRUE(fv3.Index() == FieldValue::NUMBER);
    ASSERT_TRUE(fv4.Index() == FieldValue::STRING);
    ASSERT_EQ(fv1.FormatAsString(), std::to_string(i8v1));
    ASSERT_EQ(fv2.FormatAsString(), std::to_string(dv2));
    ASSERT_EQ(fv3.FormatAsString(), std::to_string(ui64v1));
    ASSERT_EQ(fv4.FormatAsString(), content);
}

/**
 * @tc.name: FieldNumberTest_01
 * @tc.desc: test the base functions of FieldNumber.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, FieldNumberTest_01, testing::ext::TestSize.Level0)
{
    using namespace EventStore;

    int8_t i8v1 = 1; // test value
    FieldNumber fn1(i8v1);
    double dv2 = 2.0; // test value
    FieldNumber fn2(dv2);
    uint64_t ui64v1 = 3; // test value
    FieldNumber fn3(ui64v1);
    ASSERT_TRUE(fn1.GetNumber<int64_t>() == i8v1);
    ASSERT_TRUE(fn1.FormatAsString() == std::to_string(i8v1));
    ASSERT_TRUE(fn2.GetNumber<double>() == dv2);
    ASSERT_TRUE(fn2.FormatAsString() == std::to_string(dv2));
    ASSERT_TRUE(fn3.GetNumber<uint64_t>() == ui64v1);
    ASSERT_TRUE(fn3.FormatAsString() == std::to_string(ui64v1));
    ASSERT_TRUE(fn1.Index() == FieldNumber::INT);
    ASSERT_TRUE(fn2.Index() == FieldNumber::DOUBLE);
    ASSERT_TRUE(fn3.Index() == FieldNumber::UINT);
}

/**
 * @tc.name: FieldNumberTest_02
 * @tc.desc: test the operator== && operator!= of FieldNumber.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, FieldNumberTest_02, testing::ext::TestSize.Level0)
{
    using namespace EventStore;

    int64_t i64v1 = 10; // test value
    FieldNumber fn1(i64v1);
    int64_t i64v2 = 11; // test value
    FieldNumber fn2(i64v2);
    uint64_t ui64v1 = 10; // test value
    FieldNumber fn3(ui64v1);
    uint64_t ui64v2 = 11; // test value
    FieldNumber fn4(ui64v2);
    double dv1 = 10.0; // test value
    FieldNumber fn5(dv1);
    double dv2 = 11.0; // test value
    FieldNumber fn6(dv2);

    ASSERT_TRUE((fn1 == fn3) && (fn1 == fn5) && (fn3 == fn5));
    ASSERT_FALSE((fn1 == fn2) || (fn1 == fn4) || (fn1 == fn6));
    ASSERT_TRUE((fn1 != fn2) && (fn1 != fn4) && (fn1 != fn6));
    ASSERT_FALSE((fn1 != fn3) || (fn1 != dv1) || (fn3 != dv1));
}

/**
 * @tc.name: FieldNumberTest_03
 * @tc.desc: test the operator< && operator> of FieldNumber.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, FieldNumberTest_03, testing::ext::TestSize.Level0)
{
    using namespace EventStore;

    int64_t i64v1 = -10; // test value
    FieldNumber fn1(i64v1);
    int64_t i64v2 = 11; // test value
    FieldNumber fn2(i64v2);
    uint64_t ui64v1 = 10; // test value
    FieldNumber fn3(ui64v1);
    uint64_t ui64v2 = 11; // test value
    FieldNumber fn4(ui64v2);
    double dv1 = 10.0; // test value
    FieldNumber fn5(dv1);
    double dv2 = 11.0; // test value
    FieldNumber fn6(dv2);

    ASSERT_TRUE((fn1 < fn2) && (fn1 < fn4) && (fn1 < fn6));
    ASSERT_FALSE((fn1 > fn3) || (fn1 > fn4) || (fn1 > fn5) || (fn1 > fn6));
}

/**
 * @tc.name: FieldNumberTest_04
 * @tc.desc: test the operator<= && operator>= of FieldNumber.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, FieldNumberTest_04, testing::ext::TestSize.Level0)
{
    using namespace EventStore;

    int64_t i64v1 = -10; // test value
    FieldNumber fn1(i64v1);
    int64_t i64v2 = 11; // test value
    FieldNumber fn2(i64v2);
    uint64_t ui64v1 = 10; // test value
    FieldNumber fn3(ui64v1);
    uint64_t ui64v2 = 11; // test value
    FieldNumber fn4(ui64v2);
    double dv1 = 10.0; // test value
    FieldNumber fn5(dv1);
    double dv2 = 11.0; // test value
    FieldNumber fn6(dv2);

    ASSERT_TRUE((fn1 <= fn2) && (fn1 <= fn4) && (fn1 <= fn5));
    ASSERT_TRUE((fn3 >= fn1) && (fn6 >= fn4));
    ASSERT_TRUE((fn2 >= fn1) && (fn3 >= fn1) && (fn6 >= fn3) && (fn6 >= fn5));
}

/**
 * @tc.name: FieldNumberTest_05
 * @tc.desc: test the comparasion between different number types by FieldNumber.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, FieldNumberTest_05, testing::ext::TestSize.Level0)
{
    using namespace EventStore;

    int64_t i64v1 = 1; // a random test value
    FieldNumber fn1(i64v1);
    uint64_t ui64V2 = std::numeric_limits<uint64_t>::max(); // 10 is a random offset
    FieldNumber fn2(ui64V2);
    uint64_t ui64V3 = std::numeric_limits<uint64_t>::max() / 2; // 10 is a random offset
    FieldNumber fn3(ui64V3);
    int64_t i64V2 = std::numeric_limits<int64_t>::max() - 2; // 2 is a random offset
    FieldNumber fn4(i64V2);
    int64_t i64V3 = -100; // a random test value
    FieldNumber fn5(i64V3);
    double dV1 = 3.45; // a random test value
    FieldNumber fn6(dV1);
    double dV2 = 100.45; // a random test value
    FieldNumber fn7(dV2);
    int64_t i64V4 = std::numeric_limits<int64_t>::min();
    FieldNumber fn8(i64V4);

    ASSERT_TRUE((fn1 < fn2) && (fn1 <= fn2) && (fn1 <= fn2) && (fn3 < fn2) && (fn3 <= fn2) &&
        (fn1 < fn4) && (fn4 < fn3) && (fn5 < fn2) && (fn5 <= fn2) && (fn5 < fn4) && (fn1 < fn6) &&
        (fn6 < fn2) && (fn6 < fn3) && (fn7 > fn6) && (fn8 < fn1) && (fn8 < fn2) && (fn8 < fn3) &&
        (fn8 < fn6) && (fn8 < fn7));
    ASSERT_TRUE((fn2 > fn1) && (fn2 >= fn1) && (fn2 > fn3) && (fn2 >= fn3) && (fn4 > fn1) &&
        (fn3 > fn4) && (fn2 > fn5) && (fn2 >= fn5) && (fn4 > fn5) && (fn6 > fn5) && (fn6 < fn7) &&
        (fn8 < fn7) && (fn8 < fn1) && (fn8 < fn2));
}

/**
 * @tc.name: CondTest_01
 * @tc.desc: test the function of Cond.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, CondTest_01, testing::ext::TestSize.Level0)
{
    using namespace EventStore;

    Cond invalidCond;
    ASSERT_EQ(invalidCond.ToString(), "INVALID COND");
}

/**
 * @tc.name: CondTest_02
 * @tc.desc: test the function of Cond.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, CondTest_02, testing::ext::TestSize.Level0)
{
    using namespace EventStore;
    Cond cond1("name", EQ, "event");
    ASSERT_EQ(cond1.ToString(), "name == event");
}

/**
 * @tc.name: CondTest_03
 * @tc.desc: test the function of Cond.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, CondTest_03, testing::ext::TestSize.Level0)
{
    using namespace EventStore;
    Cond cond2("name", NE, "event");
    ASSERT_EQ(cond2.ToString(), "name != event");
}

/**
 * @tc.name: CondTest_04
 * @tc.desc: test the function of Cond.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, CondTest_04, testing::ext::TestSize.Level0)
{
    using namespace EventStore;
    Cond cond3("num", LT, 0);
    ASSERT_EQ(cond3.ToString(), "num < 0");
}

/**
 * @tc.name: CondTest_05
 * @tc.desc: test the function of Cond.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, CondTest_05, testing::ext::TestSize.Level0)
{
    using namespace EventStore;
    Cond cond4("num", LE, 0);
    ASSERT_EQ(cond4.ToString(), "num <= 0");
}

/**
 * @tc.name: CondTest_06
 * @tc.desc: test the function of Cond.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, CondTest_06, testing::ext::TestSize.Level0)
{
    using namespace EventStore;
    Cond cond5("num", GT, TEST_DOU_VALUE);
    ASSERT_EQ(cond5.ToString(), "num > 123.456000");
}

/**
 * @tc.name: CondTest_07
 * @tc.desc: test the function of Cond.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, CondTest_07, testing::ext::TestSize.Level0)
{
    using namespace EventStore;
    Cond cond6("num", GE, TEST_DOU_VALUE);
    ASSERT_EQ(cond6.ToString(), "num >= 123.456000");
}

/**
 * @tc.name: CondTest_08
 * @tc.desc: test the function of Cond.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, CondTest_08, testing::ext::TestSize.Level0)
{
    using namespace EventStore;
    Cond cond7("name", SW, "event");
    ASSERT_EQ(cond7.ToString(), "name SW event");
}

/**
 * @tc.name: CondTest_09
 * @tc.desc: test the function of Cond.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, CondTest_09, testing::ext::TestSize.Level0)
{
    using namespace EventStore;
    Cond cond8("name", NSW, "event");
    ASSERT_EQ(cond8.ToString(), "name NSW event");
}

/**
 * @tc.name: DocQueryTest_01
 * @tc.desc: test the function of Cond.
 * @tc.type: FUNC
 * @tc.require: issueI7NUTO
 */
HWTEST_F(SysEventDaoTest, DocQueryTest_01, testing::ext::TestSize.Level0)
{
    using namespace EventStore;

    // test for nullptr
    DocQuery docQuery;
    ASSERT_TRUE(docQuery.IsContainInnerConds(nullptr));

    // test for invalid conditon
    Cond invalidCond(EventCol::DOMAIN, EQ, TEST_STR_VALUE);
    docQuery.And(invalidCond);
    ASSERT_TRUE(docQuery.ToString().empty());

    // test for IsContainExtraConds
    Cond cond1("INT_VALUE", EQ, 1);
    docQuery.And(cond1);
    Cond cond2("DOU_VALUE", GT, 1.0);
    docQuery.And(cond2);
    Cond cond3("STR_VALUE", NE, "invalid value");
    docQuery.And(cond3);
    Cond cond4("INT_VALUE", GE, 0);
    docQuery.And(cond4);
    Cond cond5("INT_VALUE", LT, 2); // INT_VALUE < 2
    docQuery.And(cond5);
    Cond cond6("INT_VALUE", LE, 2); // INT_VALUE <= 2
    docQuery.And(cond6);
    Cond cond7("STR_VALUE", SW, "test");
    docQuery.And(cond7);
    Cond cond8("STR_VALUE", NSW, "invalid");
    docQuery.And(cond8);
    ASSERT_FALSE(docQuery.ToString().empty());

    std::string jsonStr = R"~({"domain_":"demo", "name_":"DocQueryTest_01", "type_":1, "tz_":8, "time_":1620271291188,
        "pid_":6527, "tid_":6527, "INT_VALUE":1, "DOU_VALUE":1.23, "STR_VALUE":"test_event"})~";
    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, jsonStr);
    ASSERT_TRUE(docQuery.IsContainExtraConds(sysEvent->rawData_->GetData(), sysEvent->rawData_->GetDataLength()));
}
} // namespace HiviewDFX
} // namespace OHOS