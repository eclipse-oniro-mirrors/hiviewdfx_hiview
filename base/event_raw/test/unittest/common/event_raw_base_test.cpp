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

#include "event_raw_base_test.h"

#include "base/raw_data_base_def.h"
#include "base/raw_data.h"
#include "base/value_param.h"

namespace OHOS {
namespace HiviewDFX {
using namespace EventRaw;
namespace {
constexpr size_t CAP = 10;
constexpr size_t CAP2 = 20;
}

void EventRawBaseTest::SetUpTestCase()
{
}

void EventRawBaseTest::TearDownTestCase()
{
}

void EventRawBaseTest::SetUp()
{
    platform.GetPluginMap();
}

void EventRawBaseTest::TearDown()
{
}

/**
 * @tc.name: ParseTimeZone001
 * @tc.desc: Test ParseTimeZone function
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawBaseTest, ParseTimeZone001, testing::ext::TestSize.Level1)
{
    auto offsetZone = ParseTimeZone("+0800");
    ASSERT_EQ(offsetZone, 27); // test value
    offsetZone = ParseTimeZone("+1110");
    ASSERT_EQ(offsetZone, 14); // an invalid time zone, test value
}

/**
 * @tc.name: ParseTimeZone002
 * @tc.desc: Test ParseTimeZone function
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawBaseTest, ParseTimeZone002, testing::ext::TestSize.Level1)
{
    auto offsetFormat = ParseTimeZone(27); // a valid index
    ASSERT_EQ(offsetFormat, "+0800");
    offsetFormat = ParseTimeZone(14); // a valid index
    ASSERT_EQ(offsetFormat, "+0000");
    offsetFormat = ParseTimeZone(100); // an invalid index
    ASSERT_EQ(offsetFormat, "+0000");
}

/**
 * @tc.name: GetValidDataMinimumByteCount001
 * @tc.desc: Test GetValidDataMinimumByteCount function
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawBaseTest, GetValidDataMinimumByteCount001, testing::ext::TestSize.Level1)
{
    auto limit = sizeof(int32_t) + sizeof(struct HiSysEventHeader);
    ASSERT_EQ(limit, GetValidDataMinimumByteCount());
}

/**
 * @tc.name: TransNumToHexStr001
 * @tc.desc: Test TransNumToHexStr function
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawBaseTest, TransNumToHexStr001, testing::ext::TestSize.Level1)
{
    int8_t num1 = 10; // 10 is a test value
    ASSERT_EQ("\n", TransNumToHexStr(num1));
    int16_t num2 = 300; // 300 is a test value
    ASSERT_EQ("12c", TransNumToHexStr(num2));
    int32_t num3 = 3000;
    ASSERT_EQ("bb8", TransNumToHexStr(num3));
    int64_t num4 = 3000000;
    ASSERT_EQ("2dc6c0", TransNumToHexStr(num4));
    int32_t num5 = 10;
    ASSERT_EQ("a", TransNumToHexStr(num5));
}

/**
 * @tc.name: RawDataTest001
 * @tc.desc: Test api interfaces of RawData class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawBaseTest, RawDataTest001, testing::ext::TestSize.Level1)
{
    auto data = std::make_shared<RawData>();
    ASSERT_TRUE(data != nullptr);
    ASSERT_TRUE(data->IsEmpty());
    ASSERT_EQ(data->GetDataLength(), 0);

    uint8_t* tmpData = new(std::nothrow) uint8_t[CAP];
    data->Append(tmpData, CAP);
    ASSERT_EQ(data->GetDataLength(), CAP);
    data->Update(tmpData, CAP, 0);
    ASSERT_EQ(data->GetDataLength(), CAP);
    data->Reset();
    ASSERT_TRUE(data->IsEmpty());
    delete[] tmpData;
}

/**
 * @tc.name: RawDataTest002
 * @tc.desc: Test api interfaces of RawData class
 * @tc.type: FUNC
 * @tc.require: issueI7X274
 */
HWTEST_F(EventRawBaseTest, RawDataTest002, testing::ext::TestSize.Level1)
{
    uint8_t* tmpData = new(std::nothrow) uint8_t[CAP];
    RawData data1;
    ASSERT_TRUE(data1.IsEmpty());
    ASSERT_EQ(data1.GetDataLength(), 0);
    data1.Append(tmpData, CAP);
    RawData data2 = data1;
    ASSERT_EQ(data2.GetDataLength(), CAP);
    RawData data3(tmpData, CAP);
    ASSERT_TRUE(!data3.IsEmpty());
    ASSERT_EQ(data3.GetDataLength(), CAP);
    delete[] tmpData;
}

/**
 * @tc.name: RawDataTest003
 * @tc.desc: Test assign apis of RawData class
 * @tc.type: FUNC
 * @tc.require: issueIBT9BB
 */
HWTEST_F(EventRawBaseTest, RawDataTest003, testing::ext::TestSize.Level1)
{
    uint8_t* tmpData = nullptr;
    RawData srcData(tmpData, 0);
    ASSERT_EQ(srcData.GetDataLength(), 0);
    RawData destData(srcData);
    ASSERT_EQ(destData.GetDataLength(), 0);
    RawData normalData;
    uint8_t* tmpData1 = new(std::nothrow) uint8_t[CAP];
    normalData.Append(tmpData1, CAP);
    ASSERT_EQ(normalData.GetDataLength(), CAP);
    destData = const_cast<const RawData&>(normalData);
    ASSERT_EQ(destData.GetDataLength(), CAP);
    destData = const_cast<const RawData&>(srcData);
    ASSERT_EQ(destData.GetDataLength(), CAP);

    uint8_t* tmpData2 = new(std::nothrow) uint8_t[CAP2];
    RawData normalData2;
    normalData2.Append(tmpData2, CAP);
    normalData = const_cast<const RawData&>(normalData2);
    ASSERT_EQ(destData.GetDataLength(), normalData2.GetDataLength());
    srcData = const_cast<const RawData&>(normalData2);
    ASSERT_EQ(srcData.GetDataLength(), normalData2.GetDataLength());
}
} // namespace HiviewDFX
} // namespace OHOS