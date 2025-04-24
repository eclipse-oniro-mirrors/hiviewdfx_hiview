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

#include "event_raw_encoded_and_decoded_test.h"

#include <memory>
#include <vector>

#include "decoded/decoded_event.h"
#include "decoded/decoded_param.h"
#include "decoded/raw_data_decoder.h"
#include "encoded/encoded_param.h"
#include "encoded/raw_data_builder_json_parser.h"
#include "encoded/raw_data_builder.h"
#include "encoded/raw_data_encoder.h"

namespace OHOS {
namespace HiviewDFX {
using namespace EventRaw;
namespace {
const std::string TEST_KEY = "KEY";
constexpr size_t MAX_LEN = 1000;
constexpr uint8_t CNT = 3;
}

void EventRawEncodedTest::SetUpTestCase()
{
}

void EventRawEncodedTest::TearDownTestCase()
{
}

void EventRawEncodedTest::SetUp()
{
    platform.GetPluginMap();
}

void EventRawEncodedTest::TearDown()
{
}

/**
 * @tc.name: EncodedParamTest001
 * @tc.desc: Test api interfaces of UnsignedVarintEncodedParam class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, EncodedParamTest001, testing::ext::TestSize.Level1)
{
    uint64_t val = 100; // test value
    auto encodedParam = std::make_shared<UnsignedVarintEncodedParam<uint64_t>>(TEST_KEY, val);
    ASSERT_TRUE(encodedParam != nullptr);
    std::shared_ptr<EncodedParam> param = encodedParam;
    ASSERT_EQ(param->GetKey(), TEST_KEY);
    ASSERT_TRUE(param->Encode());
    ASSERT_TRUE(!param->GetRawData().IsEmpty());
    uint64_t dest1 = 0;
    ASSERT_TRUE(param->AsUint64(dest1));
    ASSERT_EQ(dest1, val);
    int64_t dest2 = 0; // test value
    ASSERT_TRUE(!param->AsInt64(dest2));
    double dest3 = 0.0; // test value
    ASSERT_TRUE(!param->AsDouble(dest3));
    std::string dest4; // test value
    ASSERT_TRUE(param->AsString(dest4));
    ASSERT_EQ(dest4, std::to_string(val)); // test value
    std::vector<uint64_t> dest5;
    ASSERT_TRUE(!param->AsUint64Vec(dest5));
    std::vector<int64_t> dest6;
    ASSERT_TRUE(!param->AsInt64Vec(dest6));
    std::vector<double> dest7;
    ASSERT_TRUE(!param->AsDoubleVec(dest7));
    std::vector<std::string> dest8;
    ASSERT_TRUE(!param->AsStringVec(dest8));
}

/**
 * @tc.name: EncodedParamTest002
 * @tc.desc: Test api interfaces of UnsignedVarintEncodedArrayParam class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, EncodedParamTest002, testing::ext::TestSize.Level1)
{
    std::vector<uint64_t> vals = {1, 200, 3000}; // test value
    auto encodedParam = std::make_shared<UnsignedVarintEncodedArrayParam<uint64_t>>(TEST_KEY, vals);
    ASSERT_TRUE(encodedParam != nullptr);
    std::shared_ptr<EncodedParam> param = encodedParam;
    ASSERT_EQ(param->GetKey(), TEST_KEY);
    ASSERT_TRUE(param->Encode());
    ASSERT_TRUE(!param->GetRawData().IsEmpty());
    uint64_t dest1 = 0;
    ASSERT_TRUE(!param->AsUint64(dest1));
    int64_t dest2 = 0; // test value
    ASSERT_TRUE(!param->AsInt64(dest2));
    double dest3 = 0.0; // test value
    ASSERT_TRUE(!param->AsDouble(dest3));
    std::string dest4;  // test value
    ASSERT_TRUE(!param->AsString(dest4));
    std::vector<uint64_t> dest5;
    ASSERT_TRUE(param->AsUint64Vec(dest5));
    ASSERT_TRUE(dest5.size() == vals.size());
    ASSERT_EQ(dest5[0], vals[0]); // compare by index
    ASSERT_EQ(dest5[1], vals[1]); // compare by index
    ASSERT_EQ(dest5[2], vals[2]); // compare by index
    std::vector<int64_t> dest6;
    ASSERT_TRUE(!param->AsInt64Vec(dest6));
    std::vector<double> dest7;
    ASSERT_TRUE(!param->AsDoubleVec(dest7));
    std::vector<std::string> dest8;
    ASSERT_TRUE(!param->AsStringVec(dest8));
}

/**
 * @tc.name: EncodedParamTest003
 * @tc.desc: Test api interfaces of SignedVarintEncodedParam class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, EncodedParamTest003, testing::ext::TestSize.Level1)
{
    int64_t val = 100; // test value
    auto encodedParam = std::make_shared<SignedVarintEncodedParam<int64_t>>(TEST_KEY, val);
    ASSERT_TRUE(encodedParam != nullptr);
    std::shared_ptr<EncodedParam> param = encodedParam;
    ASSERT_EQ(param->GetKey(), TEST_KEY);
    ASSERT_TRUE(param->Encode());
    ASSERT_TRUE(!param->GetRawData().IsEmpty());
    uint64_t dest1 = 0;
    ASSERT_TRUE(!param->AsUint64(dest1));
    int64_t dest2 = 0; // test value
    ASSERT_TRUE(param->AsInt64(dest2));
    ASSERT_EQ(dest2, val);
    double dest3 = 0.0; // test value
    ASSERT_TRUE(!param->AsDouble(dest3));
    std::string dest4; // test value
    ASSERT_TRUE(param->AsString(dest4));
    ASSERT_EQ(dest4, std::to_string(val)); // test value
    std::vector<uint64_t> dest5;
    ASSERT_TRUE(!param->AsUint64Vec(dest5));
    std::vector<int64_t> dest6;
    ASSERT_TRUE(!param->AsInt64Vec(dest6));
    std::vector<double> dest7;
    ASSERT_TRUE(!param->AsDoubleVec(dest7));
    std::vector<std::string> dest8;
    ASSERT_TRUE(!param->AsStringVec(dest8));
}

/**
 * @tc.name: EncodedParamTest004
 * @tc.desc: Test api interfaces of SignedVarintEncodedArrayParam class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, EncodedParamTest004, testing::ext::TestSize.Level1)
{
    std::vector<int64_t> vals = {1, -200, 3000}; // test value
    auto encodedParam = std::make_shared<SignedVarintEncodedArrayParam<int64_t>>(TEST_KEY, vals);
    ASSERT_TRUE(encodedParam != nullptr);
    std::shared_ptr<EncodedParam> param = encodedParam;
    ASSERT_EQ(param->GetKey(), TEST_KEY);
    ASSERT_TRUE(param->Encode());
    ASSERT_TRUE(!param->GetRawData().IsEmpty());
    uint64_t dest1 = 0;
    ASSERT_TRUE(!param->AsUint64(dest1));
    int64_t dest2 = 0; // test value
    ASSERT_TRUE(!param->AsInt64(dest2));
    double dest3 = 0.0; // test value
    ASSERT_TRUE(!param->AsDouble(dest3));
    std::string dest4;  // test value
    ASSERT_TRUE(!param->AsString(dest4));
    std::vector<uint64_t> dest5;
    ASSERT_TRUE(!param->AsUint64Vec(dest5));
    std::vector<int64_t> dest6;
    ASSERT_TRUE(param->AsInt64Vec(dest6));
    ASSERT_TRUE(dest6.size() == vals.size());
    ASSERT_EQ(dest6[0], vals[0]); // compare by index
    ASSERT_EQ(dest6[1], vals[1]); // compare by index
    ASSERT_EQ(dest6[2], vals[2]); // compare by index
    std::vector<double> dest7;
    ASSERT_TRUE(!param->AsDoubleVec(dest7));
    std::vector<std::string> dest8;
    ASSERT_TRUE(!param->AsStringVec(dest8));
}

/**
 * @tc.name: EncodedParamTest005
 * @tc.desc: Test api interfaces of FloatingNumberEncodedParam class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, EncodedParamTest005, testing::ext::TestSize.Level1)
{
    double val = 100.0; // test value
    auto encodedParam = std::make_shared<FloatingNumberEncodedParam<double>>(TEST_KEY, val);
    ASSERT_TRUE(encodedParam != nullptr);
    std::shared_ptr<EncodedParam> param = encodedParam;
    ASSERT_EQ(param->GetKey(), TEST_KEY);
    ASSERT_TRUE(param->Encode());
    ASSERT_TRUE(!param->GetRawData().IsEmpty());
    uint64_t dest1 = 0;
    ASSERT_TRUE(!param->AsUint64(dest1));
    int64_t dest2 = 0; // test value
    ASSERT_TRUE(!param->AsInt64(dest2));
    double dest3 = 0.0; // test value
    ASSERT_TRUE(param->AsDouble(dest3));
    ASSERT_EQ(dest3, val);
    std::string dest4; // test value
    ASSERT_TRUE(param->AsString(dest4));
    ASSERT_EQ(dest4, std::to_string(val)); // test value
    std::vector<uint64_t> dest5;
    ASSERT_TRUE(!param->AsUint64Vec(dest5));
    std::vector<int64_t> dest6;
    ASSERT_TRUE(!param->AsInt64Vec(dest6));
    std::vector<double> dest7;
    ASSERT_TRUE(!param->AsDoubleVec(dest7));
    std::vector<std::string> dest8;
    ASSERT_TRUE(!param->AsStringVec(dest8));
}

/**
 * @tc.name: EncodedParamTest006
 * @tc.desc: Test api interfaces of FloatingNumberEncodedArrayParam class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, EncodedParamTest006, testing::ext::TestSize.Level1)
{
    std::vector<double> vals = {1.0, -200.0, 300}; // test value
    auto encodedParam = std::make_shared<FloatingNumberEncodedArrayParam<double>>(TEST_KEY, vals);
    ASSERT_TRUE(encodedParam != nullptr);
    std::shared_ptr<EncodedParam> param = encodedParam;
    ASSERT_EQ(param->GetKey(), TEST_KEY);
    ASSERT_TRUE(param->Encode());
    ASSERT_TRUE(!param->GetRawData().IsEmpty());
    uint64_t dest1 = 0;
    ASSERT_TRUE(!param->AsUint64(dest1));
    int64_t dest2 = 0; // test value
    ASSERT_TRUE(!param->AsInt64(dest2));
    double dest3 = 0.0; // test value
    ASSERT_TRUE(!param->AsDouble(dest3));
    std::string dest4;  // test value
    ASSERT_TRUE(!param->AsString(dest4));
    std::vector<uint64_t> dest5;
    ASSERT_TRUE(!param->AsUint64Vec(dest5));
    std::vector<int64_t> dest6;
    ASSERT_TRUE(!param->AsInt64Vec(dest6));
    std::vector<double> dest7;
    ASSERT_TRUE(param->AsDoubleVec(dest7));
    ASSERT_TRUE(dest7.size() == vals.size());
    ASSERT_EQ(dest7[0], vals[0]); // compare by index
    ASSERT_EQ(dest7[1], vals[1]); // compare by index
    ASSERT_EQ(dest7[2], vals[2]); // compare by index
    std::vector<std::string> dest8;
    ASSERT_TRUE(!param->AsStringVec(dest8));
}

/**
 * @tc.name: EncodedParamTest007
 * @tc.desc: Test api interfaces of StringEncodedParam class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, EncodedParamTest007, testing::ext::TestSize.Level1)
{
    std::string val = "100.9"; // test value
    auto encodedParam = std::make_shared<StringEncodedParam>(TEST_KEY, val);
    ASSERT_TRUE(encodedParam != nullptr);
    std::shared_ptr<EncodedParam> param = encodedParam;
    ASSERT_EQ(param->GetKey(), TEST_KEY);
    ASSERT_TRUE(param->Encode());
    ASSERT_TRUE(!param->GetRawData().IsEmpty());
    uint64_t dest1 = 0;
    ASSERT_TRUE(!param->AsUint64(dest1));
    int64_t dest2 = 0; // test value
    ASSERT_TRUE(!param->AsInt64(dest2));
    double dest3 = 0.0; // test value
    ASSERT_TRUE(!param->AsDouble(dest3));
    std::string dest4; // test value
    ASSERT_TRUE(param->AsString(dest4));
    ASSERT_EQ(dest4, val);
    ASSERT_EQ(dest4, val); // test value
    std::vector<uint64_t> dest5;
    ASSERT_TRUE(!param->AsUint64Vec(dest5));
    std::vector<int64_t> dest6;
    ASSERT_TRUE(!param->AsInt64Vec(dest6));
    std::vector<double> dest7;
    ASSERT_TRUE(!param->AsDoubleVec(dest7));
    std::vector<std::string> dest8;
    ASSERT_TRUE(!param->AsStringVec(dest8));
}

/**
 * @tc.name: EncodedParamTest008
 * @tc.desc: Test api interfaces of StringEncodedArrayParam class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, EncodedParamTest008, testing::ext::TestSize.Level1)
{
    std::vector<std::string> vals = {"1.0", "-200.0", "300"}; // test value
    auto encodedParam = std::make_shared<StringEncodedArrayParam>(TEST_KEY, vals);
    ASSERT_TRUE(encodedParam != nullptr);
    std::shared_ptr<EncodedParam> param = encodedParam;
    ASSERT_EQ(param->GetKey(), TEST_KEY);
    ASSERT_TRUE(param->Encode());
    ASSERT_TRUE(!param->GetRawData().IsEmpty());
    uint64_t dest1 = 0;
    ASSERT_TRUE(!param->AsUint64(dest1));
    int64_t dest2 = 0; // test value
    ASSERT_TRUE(!param->AsInt64(dest2));
    double dest3 = 0.0; // test value
    ASSERT_TRUE(!param->AsDouble(dest3));
    std::string dest4;  // test value
    ASSERT_TRUE(!param->AsString(dest4));
    std::vector<uint64_t> dest5;
    ASSERT_TRUE(!param->AsUint64Vec(dest5));
    std::vector<int64_t> dest6;
    ASSERT_TRUE(!param->AsInt64Vec(dest6));
    std::vector<double> dest7;
    ASSERT_TRUE(!param->AsDoubleVec(dest7));
    std::vector<std::string> dest8;
    ASSERT_TRUE(param->AsStringVec(dest8));
    ASSERT_TRUE(dest8.size() == vals.size());
    ASSERT_EQ(dest8[0], vals[0]);
    ASSERT_EQ(dest8[1], vals[1]);
    ASSERT_EQ(dest8[2], vals[2]);
}

/**
 * @tc.name: RawDataBuilderJsonParserTest001
 * @tc.desc: Parse sysevent which contains floating point number with scientific notation format
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, RawDataBuilderJsonParserTest001, testing::ext::TestSize.Level1)
{
    std::string rawSysEventStr = "{\"domain_\":\"DEMO\",\"name_\":\"EVENT_NAME_A\",\"type_\":4,\
        \"PARAM_A\":3.4,\"UINT64_T\":18446744073709551610,\"DOUBLE_T\":3.34523e+05,\"INT64_T\":9223372036854775800,\
        \"PARAM_B\":[\"123\", \"456\", \"789\"],\"PARAM_C\":[]}";
    auto parser = std::make_shared<RawDataBuilderJsonParser>(rawSysEventStr);
    ASSERT_TRUE(parser != nullptr);
    auto eventBuilder = parser->Parse();
    ASSERT_TRUE(eventBuilder != nullptr);
    ASSERT_EQ(eventBuilder->GetDomain(), "DEMO");
    ASSERT_EQ(eventBuilder->GetName(), "EVENT_NAME_A");
    ASSERT_EQ(eventBuilder->GetEventType(), 4);
    uint64_t val = 0.0;
    eventBuilder->ParseValueByKey("INT64_T", val);
    ASSERT_EQ(val, 9223372036854775800);
}

/**
 * @tc.name: RawDataBuilderJsonParserTest002
 * @tc.desc: Construct RawDataBuilderJsonParser with empty string
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, RawDataBuilderJsonParserTest002, testing::ext::TestSize.Level1)
{
    auto parser = std::make_shared<RawDataBuilderJsonParser>("");
    ASSERT_TRUE(parser != nullptr);
    auto eventBuilder = parser->Parse();
    ASSERT_TRUE(eventBuilder != nullptr);
}

/**
 * @tc.name: RawDataBuilderJsonParserTest003
 * @tc.desc: Construct RawDataBuilderJsonParser with valid string
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, RawDataBuilderJsonParserTest003, testing::ext::TestSize.Level1)
{
    std::string rawSysEventStr = R"~({"domain_":"KERNEL_VENDOR", "name_":"POWER_KEY",
        "type_":1, "time_":1502603794820, "tz_":"+0800", "pid_":1751, "tid_":1751, "uid_":0,
        "id_":"17835276947892625495", "trace_flag_":3, "traceid_":"a92ab1ea12c7144",
        "spanid_":"0", "pspanid_":"0", "key1":"-300", "key2":[-300, 300],
        "key3":3.4, "key4":"[3.4,-3.4]", "info_":"", "level_":"CRITICAL", "seq_":972})~";
    auto parser = std::make_shared<RawDataBuilderJsonParser>(rawSysEventStr);
    ASSERT_TRUE(parser != nullptr);
    auto eventBuilder = parser->Parse();
    ASSERT_TRUE(eventBuilder != nullptr && eventBuilder->GetDomain() == "KERNEL_VENDOR" &&
        eventBuilder->GetName() == "POWER_KEY");
}

/**
 * @tc.name: EncodedAndThenDecodedTest001
 * @tc.desc: Encode uint64_t type and then decoded
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, EncodedAndThenDecodedTest001, testing::ext::TestSize.Level1)
{
    RawData data;
    size_t decodedOffset = 0;
    uint64_t origin = 100; // test value
    RawDataEncoder::UnsignedVarintEncoded(data, EncodeType::VARINT, origin);
    uint64_t dest = 0;
    RawDataDecoder::UnsignedVarintDecoded(data.GetData(), MAX_LEN, decodedOffset, dest);
    ASSERT_EQ(origin, dest);
}

/**
 * @tc.name: EncodedAndThenDecodedTest002
 * @tc.desc: Encode int64_t type and then decoded
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, EncodedAndThenDecodedTest002, testing::ext::TestSize.Level1)
{
    RawData data;
    size_t decodedOffset = 0;
    int64_t origin = -100; // test value
    RawDataEncoder::SignedVarintEncoded(data, EncodeType::VARINT, origin);
    int64_t dest = 0;
    RawDataDecoder::SignedVarintDecoded(data.GetData(), MAX_LEN, decodedOffset, dest);
    ASSERT_EQ(origin, dest);
}

/**
 * @tc.name: EncodedAndThenDecodedTest003
 * @tc.desc: Encode uint64_t type and then decoded
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, EncodedAndThenDecodedTest003, testing::ext::TestSize.Level1)
{
    RawData data;
    size_t decodedOffset = 0;
    double origin = 100.0; // test value
    RawDataEncoder::FloatingNumberEncoded(data, origin);
    double dest = 0.0;
    RawDataDecoder::FloatingNumberDecoded(data.GetData(), MAX_LEN, decodedOffset, dest);
    ASSERT_EQ(origin, dest);
}

/**
 * @tc.name: EncodedAndThenDecodedTest004
 * @tc.desc: Encode string type and then decoded
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, EncodedAndThenDecodedTest004, testing::ext::TestSize.Level1)
{
    RawData data;
    size_t decodedOffset = 0;
    std::string origin = "TEST_VALUE"; // test value
    RawDataEncoder::StringValueEncoded(data, origin);
    std::string dest;
    RawDataDecoder::StringValueDecoded(data.GetData(), MAX_LEN, decodedOffset, dest);
    ASSERT_EQ(origin, dest);
}

/**
 * @tc.name: EncodedAndThenDecodedTest005
 * @tc.desc: Encode ValueType type and then decoded
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, EncodedAndThenDecodedTest005, testing::ext::TestSize.Level1)
{
    RawData data;
    size_t decodedOffset = 0;
    RawDataEncoder::ValueTypeEncoded(data, false, ValueType::BOOL, CNT);
    struct ParamValueType dest;
    RawDataDecoder::ValueTypeDecoded(data.GetData(), MAX_LEN, decodedOffset, dest);
    ASSERT_EQ(dest.isArray, false);
    ASSERT_EQ(dest.valueType, 1); // enum ValueType::BOOL is 1
    ASSERT_EQ(dest.valueByteCnt, CNT); //
}

/**
 * @tc.name: DecodedParamTest001
 * @tc.desc: Test api interfaces of UnsignedVarintDecodedParam class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, DecodedParamTest001, testing::ext::TestSize.Level1)
{
    RawData data;
    size_t decodedOffset = 0;
    uint64_t val = 100; // test value
    RawDataEncoder::UnsignedVarintEncoded(data, EncodeType::VARINT, val);
    auto decodedParam = std::make_shared<UnsignedVarintDecodedParam>(data.GetData(), MAX_LEN, decodedOffset,
        TEST_KEY);
    ASSERT_TRUE(decodedParam != nullptr);
    std::shared_ptr<DecodedParam> param = decodedParam;
    ASSERT_EQ(param->GetKey(), TEST_KEY);
    ASSERT_TRUE(param->DecodeValue());
    uint64_t dest1 = 0;
    ASSERT_TRUE(param->AsUint64(dest1));
    ASSERT_EQ(dest1, val);
    int64_t dest2 = 0; // test value
    ASSERT_TRUE(!param->AsInt64(dest2));
    double dest3 = 0.0; // test value
    ASSERT_TRUE(!param->AsDouble(dest3));
    std::string dest4; // test value
    ASSERT_TRUE(!param->AsString(dest4));
    std::vector<uint64_t> dest5;
    ASSERT_TRUE(!param->AsUint64Vec(dest5));
    std::vector<int64_t> dest6;
    ASSERT_TRUE(!param->AsInt64Vec(dest6));
    std::vector<double> dest7;
    ASSERT_TRUE(!param->AsDoubleVec(dest7));
    std::vector<std::string> dest8;
    ASSERT_TRUE(!param->AsStringVec(dest8));
}

/**
 * @tc.name: DecodedParamTest002
 * @tc.desc: Test api interfaces of UnsignedVarintDecodedArrayParam class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, DecodedParamTest002, testing::ext::TestSize.Level1)
{
    RawData data;
    size_t decodedOffset = 0;
    std::vector<uint64_t> vals = {1, 2, 3}; // test value
    RawDataEncoder::UnsignedVarintEncoded(data, EncodeType::LENGTH_DELIMITED, vals.size());
    for (auto item : vals) {
        RawDataEncoder::UnsignedVarintEncoded(data, EncodeType::VARINT, item);
    }
    auto decodedParam = std::make_shared<UnsignedVarintDecodedArrayParam>(data.GetData(), MAX_LEN, decodedOffset,
        TEST_KEY);
    ASSERT_TRUE(decodedParam != nullptr);
    std::shared_ptr<DecodedParam> param = decodedParam;
    ASSERT_EQ(param->GetKey(), TEST_KEY);
    ASSERT_TRUE(param->DecodeValue());
    uint64_t dest1 = 0;
    ASSERT_TRUE(!param->AsUint64(dest1));
    int64_t dest2 = 0; // test value
    ASSERT_TRUE(!param->AsInt64(dest2));
    double dest3 = 0.0; // test value
    ASSERT_TRUE(!param->AsDouble(dest3));
    std::string dest4; // test value
    ASSERT_TRUE(!param->AsString(dest4));
    std::vector<uint64_t> dest5;
    ASSERT_TRUE(param->AsUint64Vec(dest5));
    ASSERT_TRUE(dest5.size() == vals.size());
    ASSERT_EQ(dest5[0], vals[0]);
    ASSERT_EQ(dest5[1], vals[1]);
    ASSERT_EQ(dest5[2], vals[2]);
    std::vector<int64_t> dest6;
    ASSERT_TRUE(!param->AsInt64Vec(dest6));
    std::vector<double> dest7;
    ASSERT_TRUE(!param->AsDoubleVec(dest7));
    std::vector<std::string> dest8;
    ASSERT_TRUE(!param->AsStringVec(dest8));
}

/**
 * @tc.name: DecodedParamTest003
 * @tc.desc: Test api interfaces of SignedVarintDecodedParam class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, DecodedParamTest003, testing::ext::TestSize.Level1)
{
    RawData data;
    size_t decodedOffset = 0;
    int64_t val = -100; // test value
    RawDataEncoder::SignedVarintEncoded(data, EncodeType::VARINT, val);
    auto decodedParam = std::make_shared<SignedVarintDecodedParam>(data.GetData(), MAX_LEN, decodedOffset,
        TEST_KEY);
    ASSERT_TRUE(decodedParam != nullptr);
    std::shared_ptr<DecodedParam> param = decodedParam;
    ASSERT_EQ(param->GetKey(), TEST_KEY);
    ASSERT_TRUE(param->DecodeValue());
    uint64_t dest1 = 0;
    ASSERT_TRUE(!param->AsUint64(dest1));
    int64_t dest2 = 0; // test value
    ASSERT_TRUE(param->AsInt64(dest2));
    ASSERT_EQ(dest2, val);
    double dest3 = 0.0; // test value
    ASSERT_TRUE(!param->AsDouble(dest3));
    std::string dest4; // test value
    ASSERT_TRUE(!param->AsString(dest4));
    std::vector<uint64_t> dest5;
    ASSERT_TRUE(!param->AsUint64Vec(dest5));
    std::vector<int64_t> dest6;
    ASSERT_TRUE(!param->AsInt64Vec(dest6));
    std::vector<double> dest7;
    ASSERT_TRUE(!param->AsDoubleVec(dest7));
    std::vector<std::string> dest8;
    ASSERT_TRUE(!param->AsStringVec(dest8));
}

/**
 * @tc.name: DecodedParamTest004
 * @tc.desc: Test api interfaces of SignedVarintDecodedArrayParam class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, DecodedParamTest004, testing::ext::TestSize.Level1)
{
    RawData data;
    size_t decodedOffset = 0;
    std::vector<int64_t> vals = {1, -2, 3}; // test value
    RawDataEncoder::UnsignedVarintEncoded(data, EncodeType::LENGTH_DELIMITED, vals.size());
    for (auto item : vals) {
        RawDataEncoder::SignedVarintEncoded(data, EncodeType::VARINT, item);
    }
    auto decodedParam = std::make_shared<SignedVarintDecodedArrayParam>(data.GetData(), MAX_LEN, decodedOffset,
        TEST_KEY);
    ASSERT_TRUE(decodedParam != nullptr);
    std::shared_ptr<DecodedParam> param = decodedParam;
    ASSERT_EQ(param->GetKey(), TEST_KEY);
    ASSERT_TRUE(param->DecodeValue());
    uint64_t dest1 = 0;
    ASSERT_TRUE(!param->AsUint64(dest1));
    int64_t dest2 = 0; // test value
    ASSERT_TRUE(!param->AsInt64(dest2));
    double dest3 = 0.0; // test value
    ASSERT_TRUE(!param->AsDouble(dest3));
    std::string dest4; // test value
    ASSERT_TRUE(!param->AsString(dest4));
    std::vector<uint64_t> dest5;
    ASSERT_TRUE(!param->AsUint64Vec(dest5));
    std::vector<int64_t> dest6;
    ASSERT_TRUE(param->AsInt64Vec(dest6));
    ASSERT_TRUE(dest6.size() == vals.size());
    ASSERT_EQ(dest6[0], vals[0]);
    ASSERT_EQ(dest6[1], vals[1]);
    ASSERT_EQ(dest6[2], vals[2]);
    std::vector<double> dest7;
    ASSERT_TRUE(!param->AsDoubleVec(dest7));
    std::vector<std::string> dest8;
    ASSERT_TRUE(!param->AsStringVec(dest8));
}

/**
 * @tc.name: DecodedParamTest005
 * @tc.desc: Test api interfaces of FloatingNumberDecodedParam class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, DecodedParamTest005, testing::ext::TestSize.Level1)
{
    RawData data;
    size_t decodedOffset = 0;
    double val = -100; // test value
    RawDataEncoder::FloatingNumberEncoded(data, val);
    auto decodedParam = std::make_shared<FloatingNumberDecodedParam>(data.GetData(), MAX_LEN, decodedOffset,
        TEST_KEY);
    ASSERT_TRUE(decodedParam != nullptr);
    std::shared_ptr<DecodedParam> param = decodedParam;
    ASSERT_EQ(param->GetKey(), TEST_KEY);
    ASSERT_TRUE(param->DecodeValue());
    uint64_t dest1 = 0;
    ASSERT_TRUE(!param->AsUint64(dest1));
    int64_t dest2 = 0; // test value
    ASSERT_TRUE(!param->AsInt64(dest2));
    double dest3 = 0.0; // test value
    ASSERT_TRUE(param->AsDouble(dest3));
    ASSERT_EQ(dest3, val);
    std::string dest4; // test value
    ASSERT_TRUE(!param->AsString(dest4));
    std::vector<uint64_t> dest5;
    ASSERT_TRUE(!param->AsUint64Vec(dest5));
    std::vector<int64_t> dest6;
    ASSERT_TRUE(!param->AsInt64Vec(dest6));
    std::vector<double> dest7;
    ASSERT_TRUE(!param->AsDoubleVec(dest7));
    std::vector<std::string> dest8;
    ASSERT_TRUE(!param->AsStringVec(dest8));
}

/**
 * @tc.name: DecodedParamTest006
 * @tc.desc: Test api interfaces of FloatingNumberDecodedArrayParam class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, DecodedParamTest006, testing::ext::TestSize.Level1)
{
    RawData data;
    size_t decodedOffset = 0;
    std::vector<double> vals = {1.2, -2.78, 3.5}; // test value
    RawDataEncoder::UnsignedVarintEncoded(data, EncodeType::LENGTH_DELIMITED, vals.size());
    for (auto item : vals) {
        RawDataEncoder::FloatingNumberEncoded(data, item);
    }
    auto decodedParam = std::make_shared<FloatingNumberDecodedArrayParam>(data.GetData(), MAX_LEN, decodedOffset,
        TEST_KEY);
    ASSERT_TRUE(decodedParam != nullptr);
    std::shared_ptr<DecodedParam> param = decodedParam;
    ASSERT_EQ(param->GetKey(), TEST_KEY);
    ASSERT_TRUE(param->DecodeValue());
    uint64_t dest1 = 0;
    ASSERT_TRUE(!param->AsUint64(dest1));
    int64_t dest2 = 0; // test value
    ASSERT_TRUE(!param->AsInt64(dest2));
    double dest3 = 0.0; // test value
    ASSERT_TRUE(!param->AsDouble(dest3));
    std::string dest4; // test value
    ASSERT_TRUE(!param->AsString(dest4));
    std::vector<uint64_t> dest5;
    ASSERT_TRUE(!param->AsUint64Vec(dest5));
    std::vector<int64_t> dest6;
    ASSERT_TRUE(!param->AsInt64Vec(dest6));
    std::vector<double> dest7;
    ASSERT_TRUE(param->AsDoubleVec(dest7));
    ASSERT_TRUE(dest7.size() == vals.size());
    ASSERT_EQ(dest7[0], vals[0]);
    ASSERT_EQ(dest7[1], vals[1]);
    ASSERT_EQ(dest7[2], vals[2]);
    std::vector<std::string> dest8;
    ASSERT_TRUE(!param->AsStringVec(dest8));
}

/**
 * @tc.name: DecodedParamTest007
 * @tc.desc: Test api interfaces of StringDecodedParam class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, DecodedParamTest007, testing::ext::TestSize.Level1)
{
    RawData data;
    size_t decodedOffset = 0;
    std::string val = "TEST_VALUE"; // test value
    RawDataEncoder::StringValueEncoded(data, val);
    auto decodedParam = std::make_shared<StringDecodedParam>(data.GetData(), MAX_LEN, decodedOffset, TEST_KEY);
    ASSERT_TRUE(decodedParam != nullptr);
    std::shared_ptr<DecodedParam> param = decodedParam;
    ASSERT_EQ(param->GetKey(), TEST_KEY);
    ASSERT_TRUE(param->DecodeValue());
    uint64_t dest1 = 0;
    ASSERT_TRUE(!param->AsUint64(dest1));
    int64_t dest2 = 0; // test value
    ASSERT_TRUE(!param->AsInt64(dest2));
    double dest3 = 0.0; // test value
    ASSERT_TRUE(!param->AsDouble(dest3));
    std::string dest4; // test value
    ASSERT_TRUE(param->AsString(dest4));
    ASSERT_EQ(dest4, val);
    std::vector<uint64_t> dest5;
    ASSERT_TRUE(!param->AsUint64Vec(dest5));
    std::vector<int64_t> dest6;
    ASSERT_TRUE(!param->AsInt64Vec(dest6));
    std::vector<double> dest7;
    ASSERT_TRUE(!param->AsDoubleVec(dest7));
    std::vector<std::string> dest8;
    ASSERT_TRUE(!param->AsStringVec(dest8));
}

/**
 * @tc.name: DecodedParamTest008
 * @tc.desc: Test api interfaces of StringDecodedArrayParam class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, DecodedParamTest008, testing::ext::TestSize.Level1)
{
        RawData data;
    size_t decodedOffset = 0;
    std::vector<std::string> vals = {"1.2", "-2.78", "3.5"}; // test value
    RawDataEncoder::UnsignedVarintEncoded(data, EncodeType::LENGTH_DELIMITED, vals.size());
    for (auto item : vals) {
        RawDataEncoder::StringValueEncoded(data, item);
    }
    auto decodedParam = std::make_shared<StringDecodedArrayParam>(data.GetData(), MAX_LEN, decodedOffset, TEST_KEY);
    ASSERT_TRUE(decodedParam != nullptr);
    std::shared_ptr<DecodedParam> param = decodedParam;
    ASSERT_EQ(param->GetKey(), TEST_KEY);
    ASSERT_TRUE(param->DecodeValue());
    uint64_t dest1 = 0;
    ASSERT_TRUE(!param->AsUint64(dest1));
    int64_t dest2 = 0; // test value
    ASSERT_TRUE(!param->AsInt64(dest2));
    double dest3 = 0.0; // test value
    ASSERT_TRUE(!param->AsDouble(dest3));
    std::string dest4; // test value
    ASSERT_TRUE(!param->AsString(dest4));
    std::vector<uint64_t> dest5;
    ASSERT_TRUE(!param->AsUint64Vec(dest5));
    std::vector<int64_t> dest6;
    ASSERT_TRUE(!param->AsInt64Vec(dest6));
    std::vector<double> dest7;
    ASSERT_TRUE(!param->AsDoubleVec(dest7));
    std::vector<std::string> dest8;
    ASSERT_TRUE(param->AsStringVec(dest8));
    ASSERT_TRUE(dest8.size() == vals.size());
    ASSERT_EQ(dest8[0], vals[0]);
    ASSERT_EQ(dest8[1], vals[1]);
    ASSERT_EQ(dest8[2], vals[2]);
}

/**
 * @tc.name: DecodedEventTest001
 * @tc.desc: Test api interfaces of DecodedEvent class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, DecodedEventTest001, testing::ext::TestSize.Level1)
{
    DecodedEvent event1(nullptr, 0);
    ASSERT_TRUE(!event1.IsValid());

    std::string rawSysEventStr = R"~({"domain_":"DEMO","name_":"EVENT_NAME_A","type_":4,
        "PARAM_A":3.4,"UINT64_T":18446744073709551610,"DOUBLE_T":3.34523e+05,"INT64_T":9223372036854775800,
        "PARAM_B":["123","456","789"],"PARAM_C":[]})~";
    auto parser = std::make_shared<RawDataBuilderJsonParser>(rawSysEventStr);
    ASSERT_TRUE(parser != nullptr);
    auto builder = parser->Parse();
    ASSERT_TRUE(builder != nullptr);
    auto rawData = builder->Build();
    ASSERT_TRUE(rawData != nullptr);
    DecodedEvent event2(rawData->GetData(), rawData->GetDataLength());
    ASSERT_TRUE(event2.IsValid());
}

/**
 * @tc.name: DecodedEventTest002
 * @tc.desc: Test api interfaces of DecodedEvent class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, DecodedEventTest002, testing::ext::TestSize.Level1)
{
    std::string rawSysEventStr = R"~({"domain_":"DEMO","name_":"EVENT_NAME_A","type_":4,)~";
    rawSysEventStr.append(R"~("PARAM_A":3.4,"UINT64_T":18446744073709551610,)~");
    rawSysEventStr.append(R"~("INT64_T":9223372036854775800,"PARAM_B":["123","456","789"],"PARAM_C":[]})~");
    auto parser = std::make_shared<RawDataBuilderJsonParser>(rawSysEventStr);
    ASSERT_TRUE(parser != nullptr);
    auto builder = parser->Parse();
    ASSERT_TRUE(builder != nullptr);
    auto rawData = builder->Build();
    ASSERT_TRUE(rawData != nullptr);
    DecodedEvent event(rawData->GetData(), rawData->GetDataLength());
    ASSERT_TRUE(event.IsValid());
    ASSERT_TRUE(event.GetRawData() != nullptr);
    std::string formattedStr = R"~({"domain_":"DEMO","name_":"EVENT_NAME_A","type_":4,)~";
    formattedStr.append(
        R"~("time_":0,"tz_":"-0100","pid_":0,"tid_":0,"uid_":0,"log_":0,"id_":"00000000000000000000",)~");
    formattedStr.append(R"~("PARAM_A":3.4,"UINT64_T":18446744073709551610,)~");
    formattedStr.append(R"~("INT64_T":9223372036854775800,"PARAM_B":["123","456","789"],"PARAM_C":[]})~");
    ASSERT_EQ(event.AsJsonStr(), formattedStr);
    auto header = event.GetHeader();
    ASSERT_EQ(static_cast<int>(header.type), 3); // test value
    auto traceInfo = event.GetTraceInfo();
    ASSERT_EQ(traceInfo.traceFlag, 0); // test value
    ASSERT_EQ(event.GetAllCustomizedValues().size(), 5); // test value
}

/**
 * @tc.name: RawDataBuilderTest001
 * @tc.desc: Test constructors of RawDataBuilder class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, RawDataBuilderTest001, testing::ext::TestSize.Level1)
{
    auto builder1 = std::make_shared<RawDataBuilder>();
    ASSERT_EQ(builder1->GetDomain(), "");

    std::string rawSysEventStr = R"~({"domain_":"DEMO","name_":"EVENT_NAME_A","type_":4,
        "PARAM_A":3.4,"UINT64_T":18446744073709551610,"DOUBLE_T":3.34523e+05,"INT64_T":9223372036854775800,
        "PARAM_B":["123","456","789"],"PARAM_C":[]})~";
    auto parser = std::make_shared<RawDataBuilderJsonParser>(rawSysEventStr);
    ASSERT_TRUE(parser != nullptr);
    auto builder2 = parser->Parse();
    RawDataBuilder builder3(builder2->Build());
    ASSERT_EQ(builder3.GetDomain(), builder2->GetDomain());

    RawDataBuilder builder4("DEMO", "EVENT_NAME", 4); // test value
    ASSERT_EQ(builder4.GetDomain(), builder2->GetDomain());
}

/**
 * @tc.name: RawDataBuilderTest002
 * @tc.desc: Test api interfaces of RawDataBuilder class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawEncodedTest, RawDataBuilderTest002, testing::ext::TestSize.Level1)
{
    std::string rawSysEventStr = R"~({"domain_":"DEMO","name_":"EVENT_NAME_A","type_":4,
        "PARAM_A":3.4,"UINT64_T":18446744073709551610,"DOUBLE_T":3.34523e+05,"INT64_T":9223372036854775800,
        "PARAM_B":["123","456","789"],"PARAM_C":[]})~";
    auto parser = std::make_shared<RawDataBuilderJsonParser>(rawSysEventStr);
    ASSERT_TRUE(parser != nullptr);
    auto builder = parser->Parse();
    ASSERT_TRUE(builder != nullptr);
    ASSERT_TRUE(builder->Build() != nullptr);
    auto param = builder->GetValue("DOUBLE_T");
    ASSERT_TRUE(param != nullptr);
    auto header = builder->GetHeader();
    ASSERT_EQ(static_cast<int>(header.type), 3); // test value
    auto traceInfo = builder->GetTraceInfo();
    ASSERT_EQ(traceInfo.traceFlag, 0); // test value
    ASSERT_EQ(builder->GetDomain(), "DEMO"); // test value
    ASSERT_EQ(builder->GetName(), "EVENT_NAME_A"); // test value
    ASSERT_EQ(builder->GetEventType(), 4); // test value
    ASSERT_EQ(builder->GetParamCnt(), 6); // test value
    ASSERT_TRUE(builder->IsBaseInfo("domain_")); // test value
    ASSERT_TRUE(!builder->IsBaseInfo("TEST_KEY")); // test value
}
} // namespace HiviewDFX
} // namespace OHOS