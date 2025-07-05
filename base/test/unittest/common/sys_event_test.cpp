/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "sys_event_test.h"

#include <ctime>
#include <iostream>
#include <limits>
#include <memory>
#include <regex>
#include <vector>

#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr int SCALE_FACTOR = 2;
const std::vector<int64_t> ORIGIN_INT_VEC = {1, 2, 3};
const std::vector<uint64_t> ORIGIN_UINT_VEC = {1, 2, 3};
const std::vector<double> ORIGIN_INT_TO_DOUBLE_VEC = {1.0, 2.0, 3.0};
const std::vector<double> ORIGIN_DOUBLE_VEC = {1.0, 2.2, 4.8};
const std::vector<int64_t> ORIGIN_DOUBLE_TO_INT_VEC = {1, 2, 4};
const std::vector<uint64_t> ORIGIN_DOUBLE_TO_UINT_VEC = {1, 2, 4};

template<typename T>
std::string TransNumberVecToStr(const std::vector<T>& nums)
{
    std::string jsonStr = "[";
    if (nums.empty()) {
        jsonStr.append("]");
        return jsonStr;
    }
    for (auto num : nums) {
        jsonStr.append(std::to_string(num));
        jsonStr.append(",");
    }
    jsonStr.erase(jsonStr.length() - 1);
    jsonStr.append("]");
    return jsonStr;
}

std::string GetOriginTestString()
{
    std::string jsonStr = R"~({"domain_":"DEMO","name_":"NAME1","type_":1,"tz_":"+0800","time_":1620271291188,
        "pid_":6527, "tid_":6527, "traceid_":"f0ed6160bb2df4b", "spanid_":"10", "pspanid_":"20", "trace_flag_":4,)~";
    jsonStr.append(R"~("EMPTY_ARRAY":[],"INT_ARRAY1":[)~");
    jsonStr.append(std::to_string(std::numeric_limits<int64_t>::min()));
    jsonStr.append(R"~(,)~");
    jsonStr.append(std::to_string(std::numeric_limits<int64_t>::max()));
    jsonStr.append(R"~(],"INT_ARRAY2":)~");
    jsonStr.append(TransNumberVecToStr(ORIGIN_INT_VEC));
    jsonStr.append(R"~(,"UINT_ARRAY1":[)~");
    jsonStr.append(std::to_string(std::numeric_limits<uint64_t>::min()));
    jsonStr.append(R"~(,)~");
    jsonStr.append(std::to_string(std::numeric_limits<uint64_t>::max()));
    jsonStr.append(R"~(],"UINT_ARRAY2":)~");
    jsonStr.append(TransNumberVecToStr(ORIGIN_UINT_VEC));
    jsonStr.append(R"~(,"FLOAT_ARRAY1":[)~");
    jsonStr.append(std::to_string((static_cast<double>(std::numeric_limits<int64_t>::min()) * SCALE_FACTOR)));
    jsonStr.append(R"~(,)~");
    jsonStr.append(std::to_string((static_cast<double>(std::numeric_limits<uint64_t>::max()) * SCALE_FACTOR)));
    jsonStr.append(R"~(],"FLOAT_ARRAY2":)~");
    jsonStr.append(TransNumberVecToStr(ORIGIN_DOUBLE_VEC));
    jsonStr.append(R"~(,"STR_ARRAY":["STR1","STR2","STR3"]})~");
    return jsonStr;
}
}
void SysEventTest::SetUpTestCase()
{
}

void SysEventTest::TearDownTestCase()
{
}

void SysEventTest::SetUp()
{
}

void SysEventTest::TearDown()
{
}

/**
 * @tc.name: TestSendBaseType001
 * @tc.desc: create base sys event
 * @tc.type: FUNC
 * @tc.require: AR000FT2Q2 AR000FT2Q3
 */
HWTEST_F(SysEventTest, TestSendBaseType001, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create base sys event
     */
    std::cout << "TestSendBaseType001 test base type" << std::endl;
    SysEventCreator sysEventCreator("DEMO", "EVENT_NAME", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("KEY", 1);
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    std::regex expValue(R"~(\{"domain_":"DEMO","name_":"EVENT_NAME","type_":1,"time_":\d+,"tz_":"[\+\-]\d+",)~"
        R"~("pid_":\d+,"tid_":\d+,"uid_":\d+,"log_":0,"id_":"\d+","KEY":1\})~");
    std::cout << "size=" << sysEvent->AsJsonStr().size() << ", jsonStr:" << sysEvent->AsJsonStr() << std::endl;
    std::smatch baseMatch;
    auto eventJsonStr = sysEvent->AsJsonStr();
    bool isMatch = std::regex_match(eventJsonStr, baseMatch, expValue);
    ASSERT_TRUE(isMatch);
}

/**
 * @tc.name: TestSendIntVectorType002
 * @tc.desc: create base sys event
 * @tc.type: FUNC
 * @tc.require: AR000FT2Q2 AR000FT2Q3
 */
HWTEST_F(SysEventTest, TestSendIntVectorType002, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create base sys event
     */
    std::cout << "TestSendIntVectorType002 test vector<int> type" << std::endl;
    SysEventCreator sysEventCreator("DEMO", "EVENT_NAME", SysEventCreator::FAULT);
    std::vector<int> values = {1, 2, 3};
    sysEventCreator.SetKeyValue("KEY", values);
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    std::regex expValue(R"~(\{"domain_":"DEMO","name_":"EVENT_NAME","type_":1,"time_":\d+,"tz_":"[\+\-]\d+",)~"
        R"~("pid_":\d+,"tid_":\d+,"uid_":\d+,"log_":0,"id_":"\d+","KEY":\[1,2,3\]\})~");
    std::cout << "size=" << sysEvent->AsJsonStr().size() << ", jsonStr:" << sysEvent->AsJsonStr() << std::endl;
    std::smatch baseMatch;
    auto eventJsonStr = sysEvent->AsJsonStr();
    bool isMatch = std::regex_match(eventJsonStr, baseMatch, expValue);
    ASSERT_TRUE(isMatch);
}

/**
 * @tc.name: TestSysEventValueParse001
 * @tc.desc: Parse customized value as int64_t type from sys event
 * @tc.type: FUNC
 * @tc.require: issueI7V7ZA
 */
HWTEST_F(SysEventTest, TestSysEventValueParse001, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create base sys event
     */
    std::string jsonStr = R"~({"domain_":"DEMO","name_":"VALUE_PARSE001","type_":1,"tz_":"+0800","time_":1620271291188,
        "pid_":6527,"tid_":6527,"traceid_":"f0ed5160bb2df4b","spanid_":"10","pspanid_":"20","trace_flag_":4,)~";
    jsonStr.append(R"~("INT_VAL1":-1,"INT_VAL2":1,"INT_VAL3":)~");
    jsonStr.append(std::to_string(std::numeric_limits<int64_t>::min()));
    jsonStr.append(R"~(,"INT_VAL4":)~");
    jsonStr.append(std::to_string(std::numeric_limits<int64_t>::max()));
    jsonStr.append(R"~(,"UINT_VAL1":10,"UINT_VAL2":1000,"UINT_VAL3":)~");
    jsonStr.append(std::to_string(std::numeric_limits<uint64_t>::min()));
    jsonStr.append(R"~(,"UINT_VAL4":)~");
    jsonStr.append(std::to_string(std::numeric_limits<uint64_t>::max()));
    jsonStr.append("}");

    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, jsonStr);
    ASSERT_TRUE(sysEvent != nullptr);
    int64_t dest = sysEvent->GetEventIntValue("INT_VAL1");
    ASSERT_EQ(dest, -1); // test value
    dest = sysEvent->GetEventIntValue("INT_VAL2");
    ASSERT_EQ(dest, 1); // test value
    dest = sysEvent->GetEventIntValue("INT_VAL3");
    ASSERT_EQ(dest, std::numeric_limits<int64_t>::min());
    dest = sysEvent->GetEventIntValue("INT_VAL4");
    ASSERT_EQ(dest, std::numeric_limits<int64_t>::max());
    dest = sysEvent->GetEventIntValue("UINT_VAL1");
    ASSERT_EQ(dest, 10); // test value
    dest = sysEvent->GetEventIntValue("UINT_VAL2");
    ASSERT_EQ(dest, 1000); // test value
    dest = sysEvent->GetEventIntValue("UINT_VAL3");
    ASSERT_EQ(dest, static_cast<int64_t>(std::numeric_limits<uint64_t>::min()));
    dest = sysEvent->GetEventIntValue("UINT_VAL4");
    ASSERT_EQ(dest, 0); // test value
}

/**
 * @tc.name: TestSysEventValueParse002
 * @tc.desc: Parse customized value as uint64_t type from sys event
 * @tc.type: FUNC
 * @tc.require: issueI7V7ZA
 */
HWTEST_F(SysEventTest, TestSysEventValueParse002, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create base sys event
     */
    std::string jsonStr = R"~({"domain_":"DEMO","name_":"VALUE_PARSE001","type_":1,"tz_":"+0800","time_":1620271291188,
        "pid_":6527,"tid_":6527,"traceid_":"f0ed5160bb2df4b","spanid_":"10","pspanid_":"20","trace_flag_":4,)~";
    jsonStr.append(R"~("INT_VAL1":-1,"INT_VAL2":1,"INT_VAL3":)~");
    jsonStr.append(std::to_string(std::numeric_limits<int64_t>::min()));
    jsonStr.append(R"~(,"INT_VAL4":)~");
    jsonStr.append(std::to_string(std::numeric_limits<int64_t>::max()));
    jsonStr.append(R"~(,"UINT_VAL1":10,"UINT_VAL2":1000,"UINT_VAL3":)~");
    jsonStr.append(std::to_string(std::numeric_limits<uint64_t>::min()));
    jsonStr.append(R"~(,"UINT_VAL4":)~");
    jsonStr.append(std::to_string(std::numeric_limits<uint64_t>::max()));
    jsonStr.append("}");

    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, jsonStr);
    ASSERT_TRUE(sysEvent != nullptr);
    uint64_t dest = sysEvent->GetEventUintValue("INT_VAL1");
    ASSERT_EQ(dest, 0); // test value
    dest = sysEvent->GetEventUintValue("INT_VAL2");
    ASSERT_EQ(dest, 1); // test value
    dest = sysEvent->GetEventUintValue("INT_VAL3");
    ASSERT_EQ(dest, 0); // test value
    dest = sysEvent->GetEventUintValue("INT_VAL4");
    ASSERT_EQ(dest, static_cast<uint64_t>(std::numeric_limits<int64_t>::max()));
    dest = sysEvent->GetEventUintValue("UINT_VAL1");
    ASSERT_EQ(dest, 10); // test value
    dest = sysEvent->GetEventUintValue("UINT_VAL2");
    ASSERT_EQ(dest, 1000); // test value
    dest = sysEvent->GetEventUintValue("UINT_VAL3");
    ASSERT_EQ(dest, std::numeric_limits<uint64_t>::min());
    dest = sysEvent->GetEventUintValue("UINT_VAL4");
    ASSERT_EQ(dest, std::numeric_limits<uint64_t>::max());
}

/**
 * @tc.name: TestSysEventValueParse003
 * @tc.desc: Parse customized value as double type from sys event
 * @tc.type: FUNC
 * @tc.require: issueI7V7ZA
 */
HWTEST_F(SysEventTest, TestSysEventValueParse003, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create base sys event
     */
    std::string jsonStr = R"~({"domain_":"DEMO", "name_":"VALUE_PARSE001", "type_":1, "tz_":"+0800",
        "time_":1620271291188, "pid_":6527, "tid_":6527, "traceid_":"f0ed5160bb2df4b", "spanid_":"10",
        "pspanid_":"20", "trace_flag_":4, "FLOAT_VAL1":-1.0, "FLOAT_VAL2":1.0})~";

    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, jsonStr);
    ASSERT_TRUE(sysEvent != nullptr);
    double dest = sysEvent->GetEventDoubleValue("FLOAT_VAL1");
    ASSERT_EQ(dest, -1); // test value
    dest = sysEvent->GetEventDoubleValue("FLOAT_VAL2");
    ASSERT_EQ(dest, 1); // test value
}

/**
 * @tc.name: TestSysEventValueParse004
 * @tc.desc: Parse base value as int64_t type from sys event
 * @tc.type: FUNC
 * @tc.require: issueI7V7ZA
 */
HWTEST_F(SysEventTest, TestSysEventValueParse004, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create base sys event
     */
    std::string jsonStr = R"~({"domain_":"DEMO", "name_":"VALUE_PARSE001", "type_":1, "tz_":"+0800",
        "time_":1620271291188, "pid_":6527, "tid_":-6527, "traceid_":"f0ed5160bb2df4b", "spanid_":"10",
        "pspanid_":"20", "trace_flag_":4, "FLOAT_VAL1":-1.0, "FLOAT_VAL2":1.0,)~";
    jsonStr.append(R"~("FLOAT_VAL3":)~");
    jsonStr.append(std::to_string((static_cast<double>(std::numeric_limits<int64_t>::min()) * SCALE_FACTOR)));
    jsonStr.append(R"~("FLOAT_VAL4":)~");
    jsonStr.append(std::to_string((static_cast<double>(std::numeric_limits<int64_t>::max()) * SCALE_FACTOR)));
    jsonStr.append("}");

    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, jsonStr);
    ASSERT_TRUE(sysEvent != nullptr);
    int64_t dest = sysEvent->GetEventIntValue("domain_");
    ASSERT_EQ(dest, 0); // test value
    dest = sysEvent->GetEventIntValue("type_");
    ASSERT_EQ(dest, 1); // test value
    dest = sysEvent->GetEventIntValue("time_");
    ASSERT_EQ(dest, 1620271291188); // test value
    dest = sysEvent->GetEventIntValue("tz_");
    ASSERT_EQ(dest, 27); // test value
    dest = sysEvent->GetEventIntValue("pid_");
    ASSERT_EQ(dest, 6527); // test value
    dest = sysEvent->GetEventIntValue("traceid_");
    ASSERT_EQ(dest, 1085038850905136971); // test value: f0ed5160bb2df4b
    dest = sysEvent->GetEventIntValue("trace_flag_");
    ASSERT_EQ(dest, 4); // test value
    dest = sysEvent->GetEventIntValue("FLOAT_VAL1");
    ASSERT_EQ(dest, -1); // test value
    dest = sysEvent->GetEventIntValue("FLOAT_VAL2");
    ASSERT_EQ(dest, 1); // test value
    dest = sysEvent->GetEventIntValue("FLOAT_VAL3");
    ASSERT_EQ(dest, 0); // test value
    dest = sysEvent->GetEventIntValue("FLOAT_VAL4");
    ASSERT_EQ(dest, 0); // test value
}

/**
 * @tc.name: TestSysEventValueParse005
 * @tc.desc: Parse base value as uint64_t type from sys event
 * @tc.type: FUNC
 * @tc.require: issueI7V7ZA
 */
HWTEST_F(SysEventTest, TestSysEventValueParse005, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create base sys event
     */
    std::string jsonStr = R"~({"domain_":"DEMO", "name_":"VALUE_PARSE001", "type_":1, "tz_":"+0800",
        "time_":1620271291188, "pid_":6527, "tid_":6527, "traceid_":"f0ed5160bb2df4b", "spanid_":"10",
        "pspanid_":"20", "trace_flag_":4, "FLOAT_VAL1":-1.0, "FLOAT_VAL2":1.0,)~";
    jsonStr.append(R"~("FLOAT_VAL3":)~");
    jsonStr.append(std::to_string((static_cast<double>(std::numeric_limits<uint64_t>::min()) * SCALE_FACTOR)));
    jsonStr.append(R"~("FLOAT_VAL4":)~");
    jsonStr.append(std::to_string((static_cast<double>(std::numeric_limits<uint64_t>::max()) * SCALE_FACTOR)));
    jsonStr.append("}");

    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, jsonStr);
    ASSERT_TRUE(sysEvent != nullptr);
    uint64_t dest = sysEvent->GetEventUintValue("domain_");
    ASSERT_EQ(dest, 0); // test value
    dest = sysEvent->GetEventUintValue("type_");
    ASSERT_EQ(dest, 1); // test value
    dest = sysEvent->GetEventUintValue("time_");
    ASSERT_EQ(dest, 1620271291188); // test value
    dest = sysEvent->GetEventUintValue("tz_");
    ASSERT_EQ(dest, 27); // test value
    dest = sysEvent->GetEventUintValue("pid_");
    ASSERT_EQ(dest, 6527); // test value
    dest = sysEvent->GetEventUintValue("traceid_");
    ASSERT_EQ(dest, 1085038850905136971); // test value: f0ed5160bb2df4b
    dest = sysEvent->GetEventUintValue("trace_flag_");
    ASSERT_EQ(dest, 4); // test value
    dest = sysEvent->GetEventUintValue("FLOAT_VAL1");
    ASSERT_EQ(dest, 0); // test value
    dest = sysEvent->GetEventUintValue("FLOAT_VAL2");
    ASSERT_EQ(dest, 1); // test value
    dest = sysEvent->GetEventUintValue("FLOAT_VAL3");
    ASSERT_EQ(dest, 0); // test value
    dest = sysEvent->GetEventUintValue("FLOAT_VAL4");
    ASSERT_EQ(dest, 0); // test value
}

/**
 * @tc.name: TestSysEventValueParse006
 * @tc.desc: Parse base value as double type from sys event
 * @tc.type: FUNC
 * @tc.require: issueI7V7ZA
 */
HWTEST_F(SysEventTest, TestSysEventValueParse006, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create base sys event
     */
    std::string jsonStr = R"~({"domain_":"DEMO", "name_":"VALUE_PARSE001", "type_":1, "tz_":"+0800",
        "time_":1620271291188, "pid_":6527, "tid_":6527, "traceid_":"f0ed5160bb2df4b", "spanid_":"10",
        "pspanid_":"20", "trace_flag_":4, "FLOAT_VAL1":-1.0, "FLOAT_VAL2":1.0})~";

    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, jsonStr);
    ASSERT_TRUE(sysEvent != nullptr);
    double dest = sysEvent->GetEventDoubleValue("domain_");
    ASSERT_EQ(dest, 0.0); // test value
    dest = sysEvent->GetEventDoubleValue("type_");
    ASSERT_EQ(dest, 1); // test value
    dest = sysEvent->GetEventDoubleValue("time_");
    ASSERT_EQ(dest, 1620271291188); // test value
    dest = sysEvent->GetEventDoubleValue("tz_");
    ASSERT_EQ(dest, 27); // test value
    dest = sysEvent->GetEventDoubleValue("pid_");
    ASSERT_EQ(dest, 6527); // test value
    dest = sysEvent->GetEventDoubleValue("trace_flag_");
    ASSERT_EQ(dest, 4); // test value
    dest = sysEvent->GetEventDoubleValue("FLOAT_VAL1");
    ASSERT_EQ(dest, -1); // test value
    dest = sysEvent->GetEventDoubleValue("FLOAT_VAL2");
    ASSERT_EQ(dest, 1); // test value
}

/**
 * @tc.name: TestSysEventValueParse007
 * @tc.desc: Parse base value as string type from sys event
 * @tc.type: FUNC
 * @tc.require: issueI7V7ZA
 */
HWTEST_F(SysEventTest, TestSysEventValueParse007, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create base sys event
     */
    std::string jsonStr = R"~({"domain_":"DEMO", "name_":"VALUE_PARSE001", "type_":1, "tz_":"+0800",
        "time_":1620271291188, "pid_":6527, "tid_":6527, "traceid_":"f0ed5160bb2df4b", "spanid_":"10",
        "pspanid_":"20", "trace_flag_":4, "FLOAT_VAL1":-1.0, "FLOAT_VAL2":1.0})~";

    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, jsonStr);
    ASSERT_TRUE(sysEvent != nullptr);
    std::string dest = sysEvent->GetEventValue("domain_");
    ASSERT_EQ(dest, "DEMO"); // test value
    dest = sysEvent->GetEventValue("type_");
    ASSERT_EQ(dest, ""); // test value
    dest = sysEvent->GetEventValue("time_");
    ASSERT_EQ(dest, ""); // test value
    dest = sysEvent->GetEventValue("tz_");
    ASSERT_EQ(dest, "+0800"); // test value
    dest = sysEvent->GetEventValue("pid_");
    ASSERT_EQ(dest, ""); // test value
    dest = sysEvent->GetEventValue("traceid_");
    ASSERT_EQ(dest, "f0ed5160bb2df4b"); // test value
    dest = sysEvent->GetEventValue("trace_flag_");
    ASSERT_EQ(dest, ""); // test value
    dest = sysEvent->GetEventValue("FLOAT_VAL1");
    ASSERT_EQ(dest, ""); // test value
    dest = sysEvent->GetEventValue("FLOAT_VAL2");
    ASSERT_EQ(dest, ""); // test value
}

/**
 * @tc.name: TestSysEventValueParse008
 * @tc.desc: Parse base value as int64_t array from sys event
 * @tc.type: FUNC
 * @tc.require: issueI7V7ZA
 */
HWTEST_F(SysEventTest, TestSysEventValueParse008, testing::ext::TestSize.Level3)
{
    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, GetOriginTestString());
    ASSERT_TRUE(sysEvent != nullptr);
    std::vector<int64_t> dest1;
    ASSERT_TRUE(sysEvent->GetEventIntArrayValue("INT_ARRAY1", dest1));
    ASSERT_EQ(dest1.size(), 2); // test value
    ASSERT_TRUE((dest1[0] == std::numeric_limits<int64_t>::min()) &&
        (dest1[1] == std::numeric_limits<int64_t>::max()));
    std::vector<int64_t> dest2;
    ASSERT_TRUE(sysEvent->GetEventIntArrayValue("INT_ARRAY2", dest2));
    ASSERT_EQ(dest2, ORIGIN_INT_VEC);
    std::vector<int64_t> dest3;
    ASSERT_TRUE(!sysEvent->GetEventIntArrayValue("UINT_ARRAY1", dest3));
    ASSERT_TRUE(dest3.empty());
    std::vector<int64_t> dest4;
    ASSERT_TRUE(sysEvent->GetEventIntArrayValue("UINT_ARRAY2", dest4));
    ASSERT_EQ(dest4, ORIGIN_INT_VEC);
    std::vector<int64_t> dest5;
    ASSERT_TRUE(!sysEvent->GetEventIntArrayValue("FLOAT_ARRAY1", dest5));
    ASSERT_TRUE(dest5.empty());
    std::vector<int64_t> dest6;
    ASSERT_TRUE(sysEvent->GetEventIntArrayValue("FLOAT_ARRAY2", dest6));
    ASSERT_EQ(dest6, ORIGIN_DOUBLE_TO_INT_VEC);
    std::vector<int64_t> dest7;
    ASSERT_TRUE(!sysEvent->GetEventIntArrayValue("STR_ARRAY", dest7));
    ASSERT_TRUE(dest7.empty());
    std::vector<int64_t> dest8;
    ASSERT_TRUE(sysEvent->GetEventIntArrayValue("EMPTY_ARRAY", dest8));
    ASSERT_TRUE(dest8.empty());
}

/**
 * @tc.name: TestSysEventValueParse009
 * @tc.desc: Parse base value as uint64_t array from sys event
 * @tc.type: FUNC
 * @tc.require: issueI7V7ZA
 */
HWTEST_F(SysEventTest, TestSysEventValueParse009, testing::ext::TestSize.Level3)
{
    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, GetOriginTestString());
    ASSERT_TRUE(sysEvent != nullptr);
    std::vector<uint64_t> dest1;
    ASSERT_TRUE(!sysEvent->GetEventUintArrayValue("INT_ARRAY1", dest1));
    ASSERT_TRUE(dest1.empty());
    std::vector<uint64_t> dest2;
    ASSERT_TRUE(sysEvent->GetEventUintArrayValue("INT_ARRAY2", dest2));
    ASSERT_EQ(dest2, ORIGIN_UINT_VEC);
    std::vector<uint64_t> dest3;
    ASSERT_TRUE(sysEvent->GetEventUintArrayValue("UINT_ARRAY1", dest3));
    ASSERT_EQ(dest3.size(), 2); // test value
    ASSERT_TRUE((dest3[0] == std::numeric_limits<uint64_t>::min()) &&
        (dest3[1] == std::numeric_limits<uint64_t>::max()));
    std::vector<uint64_t> dest4;
    ASSERT_TRUE(sysEvent->GetEventUintArrayValue("UINT_ARRAY2", dest4));
    ASSERT_EQ(dest4, ORIGIN_UINT_VEC);
    std::vector<uint64_t> dest5;
    ASSERT_TRUE(!sysEvent->GetEventUintArrayValue("FLOAT_ARRAY1", dest5));
    ASSERT_TRUE(dest5.empty());
    std::vector<uint64_t> dest6;
    ASSERT_TRUE(sysEvent->GetEventUintArrayValue("FLOAT_ARRAY2", dest6));
    ASSERT_EQ(dest6, ORIGIN_DOUBLE_TO_UINT_VEC);
    std::vector<uint64_t> dest7;
    ASSERT_TRUE(!sysEvent->GetEventUintArrayValue("STR_ARRAY", dest7));
    ASSERT_TRUE(dest7.empty());
    std::vector<uint64_t> dest8;
    ASSERT_TRUE(sysEvent->GetEventUintArrayValue("EMPTY_ARRAY", dest8));
    ASSERT_TRUE(dest8.empty());
}

/**
 * @tc.name: TestSysEventValueParse010
 * @tc.desc: Parse base value as double array from sys event
 * @tc.type: FUNC
 * @tc.require: issueI7V7ZA
 */
HWTEST_F(SysEventTest, TestSysEventValueParse010, testing::ext::TestSize.Level3)
{
    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, GetOriginTestString());
    ASSERT_TRUE(sysEvent != nullptr);
    std::vector<double> dest1;
    ASSERT_TRUE(sysEvent->GetEventDoubleArrayValue("INT_ARRAY1", dest1));
    ASSERT_EQ(dest1.size(), 2); // test value
    ASSERT_EQ(dest1[0], std::numeric_limits<int64_t>::min());
    ASSERT_EQ(dest1[1], std::numeric_limits<int64_t>::max());
    std::vector<double> dest2;
    ASSERT_TRUE(sysEvent->GetEventDoubleArrayValue("INT_ARRAY2", dest2));
    ASSERT_EQ(dest2, ORIGIN_INT_TO_DOUBLE_VEC);
    std::vector<double> dest3;
    ASSERT_TRUE(sysEvent->GetEventDoubleArrayValue("UINT_ARRAY1", dest3));
    ASSERT_EQ(dest3.size(), 2); // test value
    ASSERT_EQ(dest3[0], std::numeric_limits<uint64_t>::min());
    ASSERT_EQ(dest3[1], std::numeric_limits<uint64_t>::max());
    std::vector<double> dest4;
    ASSERT_TRUE(sysEvent->GetEventDoubleArrayValue("UINT_ARRAY2", dest4));
    ASSERT_EQ(dest4, ORIGIN_INT_TO_DOUBLE_VEC);
    std::vector<double> dest5;
    ASSERT_TRUE(sysEvent->GetEventDoubleArrayValue("FLOAT_ARRAY1", dest5));
    ASSERT_EQ(dest5.size(), 2); // test value
    ASSERT_EQ(dest5[0], (static_cast<double>(std::numeric_limits<int64_t>::min()) * SCALE_FACTOR));
    ASSERT_EQ(dest5[1], (static_cast<double>(std::numeric_limits<uint64_t>::max()) * SCALE_FACTOR));
    std::vector<double> dest6;
    ASSERT_TRUE(sysEvent->GetEventDoubleArrayValue("FLOAT_ARRAY2", dest6));
    ASSERT_EQ(dest6, ORIGIN_DOUBLE_VEC);
    std::vector<double> dest7;
    ASSERT_TRUE(!sysEvent->GetEventDoubleArrayValue("STR_ARRAY", dest7));
    ASSERT_TRUE(dest7.empty());
    std::vector<double> dest8;
    ASSERT_TRUE(sysEvent->GetEventDoubleArrayValue("EMPTY_ARRAY", dest8));
    ASSERT_TRUE(dest8.empty());
}

/**
 * @tc.name: TestSysEventValueParse011
 * @tc.desc: Parse base value as string array from sys event
 * @tc.type: FUNC
 * @tc.require: issueI7V7ZA
 */
HWTEST_F(SysEventTest, TestSysEventValueParse011, testing::ext::TestSize.Level3)
{
    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, GetOriginTestString());
    ASSERT_TRUE(sysEvent != nullptr);
    std::vector<std::string> dest1;
    ASSERT_TRUE(!sysEvent->GetEventStringArrayValue("INT_ARRAY1", dest1));
    ASSERT_TRUE(dest1.empty());
    std::vector<std::string> dest2;
    ASSERT_TRUE(!sysEvent->GetEventStringArrayValue("INT_ARRAY2", dest2));
    ASSERT_TRUE(dest2.empty());
    std::vector<std::string> dest3;
    ASSERT_TRUE(!sysEvent->GetEventStringArrayValue("UINT_ARRAY1", dest3));
    ASSERT_TRUE(dest3.empty());
    std::vector<std::string> dest4;
    ASSERT_TRUE(!sysEvent->GetEventStringArrayValue("UINT_ARRAY2", dest4));
    ASSERT_TRUE(dest4.empty());
    std::vector<std::string> dest5;
    ASSERT_TRUE(!sysEvent->GetEventStringArrayValue("FLOAT_ARRAY1", dest5));
    ASSERT_TRUE(dest5.empty());
    std::vector<std::string> dest6;
    ASSERT_TRUE(!sysEvent->GetEventStringArrayValue("FLOAT_ARRAY2", dest6));
    ASSERT_TRUE(dest6.empty());
    std::vector<std::string> dest7;
    ASSERT_TRUE(sysEvent->GetEventStringArrayValue("STR_ARRAY", dest7));
    std::vector<std::string> origin = {"STR1", "STR2", "STR3"};
    ASSERT_EQ(dest7, origin);
    std::vector<std::string> dest8;
    ASSERT_TRUE(sysEvent->GetEventStringArrayValue("EMPTY_ARRAY", dest8));
    ASSERT_TRUE(dest8.empty());
}

/**
 * @tc.name: TestSysEventValueParse012
 * @tc.desc: test param interface of sysevent
 * @tc.type: FUNC
 * @tc.require: IBEQMI
 */
HWTEST_F(SysEventTest, TestSysEventValueParse012, testing::ext::TestSize.Level3)
{
    const std::string testParamName("test_new_param");
    const std::string testParamValue("any value");
    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, GetOriginTestString());
    ASSERT_FALSE(sysEvent->IsParamExist(testParamName));

    sysEvent->SetEventValue(testParamName, testParamValue);
    ASSERT_TRUE(sysEvent->IsParamExist(testParamName));
    ASSERT_EQ(testParamValue, sysEvent->GetEventValue(testParamName));

    sysEvent->RemoveParam(testParamName);
    ASSERT_FALSE(sysEvent->IsParamExist(testParamName));
    ASSERT_EQ("", sysEvent->GetEventValue(testParamName));
}

/**
 * @tc.name: TestSysEventTranslation01
 * @tc.desc: Test translation from sys event to json string
 * @tc.type: FUNC
 * @tc.require: issueI91RBZ
 */
HWTEST_F(SysEventTest, TestSysEventTranslation01, testing::ext::TestSize.Level3)
{
    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, GetOriginTestString());
    ASSERT_TRUE(sysEvent != nullptr);
    auto jsonStrOrigin = sysEvent->AsJsonStr();
    auto jsonStrBeforeValueAppend = sysEvent->AsJsonStr();
    ASSERT_EQ(jsonStrOrigin, jsonStrBeforeValueAppend);
    sysEvent->SetEventValue("TEST_KEY", "test value1");
    auto jsonStrAfterValueAppend = sysEvent->AsJsonStr();
    auto foundRet = jsonStrBeforeValueAppend.find("\"TEST_KEY\":\"test value1\"");
    ASSERT_TRUE(foundRet == std::string::npos);
    foundRet = jsonStrAfterValueAppend.find("\"TEST_KEY\":\"test value1\"");
    ASSERT_TRUE(foundRet != std::string::npos);
    sysEvent->SetEventValue("TEST_KEY", "test value2");
    jsonStrAfterValueAppend = sysEvent->AsJsonStr();
    foundRet = jsonStrAfterValueAppend.find("\"TEST_KEY\":\"test value1\"");
    ASSERT_TRUE(foundRet == std::string::npos);
    foundRet = jsonStrAfterValueAppend.find("\"TEST_KEY\":\"test value2\"");
    ASSERT_TRUE(foundRet != std::string::npos);
}

/**
 * @tc.name: TestSysEventTranslation02
 * @tc.desc: Test translation from sys event to binary raw data
 * @tc.type: FUNC
 * @tc.require: issueI91RBZ
 */
HWTEST_F(SysEventTest, TestSysEventTranslation02, testing::ext::TestSize.Level3)
{
    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, GetOriginTestString());
    ASSERT_TRUE(sysEvent != nullptr);
    auto rawData = sysEvent->AsRawData();
    int32_t dataLength = *(reinterpret_cast<int32_t*>(rawData));
    ASSERT_TRUE(rawData != nullptr);
    auto rawDataBeforeValueAppend = sysEvent->AsRawData();
    ASSERT_TRUE(rawDataBeforeValueAppend != nullptr);
    ASSERT_EQ(dataLength, *(reinterpret_cast<int32_t*>(rawDataBeforeValueAppend)));
    sysEvent->SetEventValue("TEST_KEY", "test value1");
    auto rawDataAfterValueAppend = sysEvent->AsRawData();
    ASSERT_TRUE(rawDataAfterValueAppend != nullptr);
    int32_t dataLengthAfterValueAppend = *(reinterpret_cast<int32_t*>(rawDataAfterValueAppend));
    ASSERT_NE(dataLength, dataLengthAfterValueAppend);
    sysEvent->SetEventValue("TEST_KEY", "test value123");
    auto rawDataAfterValueAppendNew = sysEvent->AsRawData();
    int32_t dataLengthAfterValueAppendNew = *(reinterpret_cast<int32_t*>(rawDataAfterValueAppendNew));
    ASSERT_NE(dataLengthAfterValueAppend, dataLengthAfterValueAppendNew);
}

/**
 * @tc.name: TestSetIdAndSetLog001
 * @tc.desc: Test SetId & SetLog apis of SysEvent
 * @tc.type: FUNC
 * @tc.require: issueIAH9IC
 */
HWTEST_F(SysEventTest, TestSetIdAndSetLog001, testing::ext::TestSize.Level3)
{
    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, GetOriginTestString());
    ASSERT_TRUE(sysEvent != nullptr);
    uint64_t id = 1; // 1 is a test id value
    sysEvent->SetId(id);
    uint8_t log = 0; // 0 is a test log value
    sysEvent->SetLog(log);
    auto eventStr = sysEvent->AsJsonStr();
    ASSERT_NE(eventStr, "");
    std::string matchedIdContent = std::string("\"id_\":\"00000000000000000001\"");
    ASSERT_NE(eventStr.find(matchedIdContent), std::string::npos);
    std::string matchedLogContent = std::string("\"log_\":") + std::to_string(log);
    ASSERT_NE(eventStr.find(matchedLogContent), std::string::npos);
}

/**
 * @tc.name: TestEventPeriodSeqInfo001
 * @tc.desc: Test SetEventPeriodSeqInfo & GetEventPeriodSeqInfo & UpdatePeriodSeqInfoToRawData apis of SysEvent
 * @tc.type: FUNC
 * @tc.require: issueIBUPS7
 */
HWTEST_F(SysEventTest, TestEventPeriodSeqInfo001, testing::ext::TestSize.Level3)
{
    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, GetOriginTestString());
    ASSERT_NE(sysEvent, nullptr);
    auto eventPeriodSeqInfo = sysEvent->GetEventPeriodSeqInfo();
    ASSERT_TRUE(eventPeriodSeqInfo.timeStamp.empty());
    EventPeriodSeqInfo periodSeqInfo {"3025010101", true, 0};
    sysEvent->SetEventPeriodSeqInfo(periodSeqInfo);
    eventPeriodSeqInfo = sysEvent->GetEventPeriodSeqInfo();
    ASSERT_EQ(periodSeqInfo.timeStamp, "3025010101");
    ASSERT_TRUE(periodSeqInfo.isNeedExport);
    ASSERT_EQ(periodSeqInfo.periodSeq, 0);
    ASSERT_TRUE(sysEvent->GetEventValue("period_seq_").empty());
    sysEvent->SetEventValue("period_seq_", sysEvent->GetValue("period_seq_"));
    ASSERT_EQ(sysEvent->GetEventValue("period_seq_"), "3025010101 1 0");
}

/**
 * @tc.name: TestEventReportInterval001
 * @tc.desc: Test SetReportInterval & GetReportInterval apis of SysEvent
 * @tc.type: FUNC
 * @tc.require: issueICI4CO
 */
HWTEST_F(SysEventTest, TestEventReportInterval001, testing::ext::TestSize.Level3)
{
    std::string jsonStr = R"~({"domain_":"TEST_DEMO","name_":"TEST_NAME","type_":1,"tz_":"+0800","time_":1620271291188,
        "pid_":1, "tid_":1, "traceid_":"f0ed6160bb2df3a", "spanid_":"11", "pspanid_":"12", "trace_flag_":1,
        "reportInterval_":-1})~";
    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, jsonStr);
    ASSERT_EQ(sysEvent->GetReportInterval(), UNINIT_REPORT_INTERVAL);
    int reportIntervalVal = 1000; //1000 is a test value
    sysEvent->SetReportInterval(reportIntervalVal);
    ASSERT_EQ(sysEvent->GetReportInterval(), reportIntervalVal);
}
} // HiviewDFX
} // OHOS