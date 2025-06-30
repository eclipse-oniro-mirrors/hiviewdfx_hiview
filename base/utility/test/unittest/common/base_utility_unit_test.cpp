/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "base_utility_unit_test.h"

#include <cmath>
#include <limits>
#include <vector>
#include <string>

#include "string_util.h"
#include "ffrt.h"
#include "ffrt_util.h"
#include "file_util.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
void BaseUtilityUnitTest::SetUpTestCase()
{
}

void BaseUtilityUnitTest::TearDownTestCase()
{
}

void BaseUtilityUnitTest::SetUp()
{
}

void BaseUtilityUnitTest::TearDown()
{
}

/**
 * @tc.name: BaseUtilityUnitTest001
 * @tc.desc: Test ConvertVectorToStr defined in namespace StringUtil
 * @tc.type: FUNC
 * @tc.require: issueI651IG
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest001, testing::ext::TestSize.Level3)
{
    std::vector<std::string> origins;
    origins.emplace_back("1");
    origins.emplace_back("2");
    origins.emplace_back("3");
    std::string split = ":";
    auto ret = StringUtil::ConvertVectorToStr(origins, split);
    ASSERT_EQ("1:2:3", ret);
}

/**
 * @tc.name: BaseUtilityUnitTest002
 * @tc.desc: Test ReplaceStr defined in namespace StringUtil
 * @tc.type: FUNC
 * @tc.require: issueI651IG
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest002, testing::ext::TestSize.Level3)
{
    std::string str = "src123src";
    std::string emptySrc;
    std::string src = "src";
    std::string src1 = "src9";
    std::string dest = "dest";
    auto ret = StringUtil::ReplaceStr(str, emptySrc, dest);
    ASSERT_EQ(ret, str);
    ret = StringUtil::ReplaceStr(str, src, dest);
    ASSERT_EQ(ret, "dest123dest");
    ret = StringUtil::ReplaceStr(str, src1, dest);
    ASSERT_EQ(ret, "src123src");
}

/**
 * @tc.name: BaseUtilityUnitTest003
 * @tc.desc: Test StrToInt defined in namespace StringUtil
 * @tc.type: FUNC
 * @tc.require: issueI651IG
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest003, testing::ext::TestSize.Level3)
{
    auto intVal1 = -1;
    auto intVal2 = 234;
    auto ret = StringUtil::StrToInt(std::to_string(intVal1));
    ASSERT_EQ(intVal1, ret);
    ret = StringUtil::StrToInt(std::to_string(intVal2));
    ASSERT_EQ(intVal2, ret);
}

/**
 * @tc.name: BaseUtilityUnitTest004
 * @tc.desc: Test DexToHexString defined in namespace StringUtil
 * @tc.type: FUNC
 * @tc.require: issueI651IG
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest004, testing::ext::TestSize.Level3)
{
    auto intVal1 = 1;
    auto intVal2 = 11;
    auto ret = StringUtil::DexToHexString(intVal1, true);
    ASSERT_EQ("1", ret);
    ret = StringUtil::DexToHexString(intVal1, false);
    ASSERT_EQ("1", ret);
    ret = StringUtil::DexToHexString(intVal2, true);
    ASSERT_EQ("B", ret);
    ret = StringUtil::DexToHexString(intVal2, false);
    ASSERT_EQ("b", ret);
}

/**
 * @tc.name: BaseUtilityUnitTest005
 * @tc.desc: Test GetKeyValueByString defined in namespace StringUtil
 * @tc.type: FUNC
 * @tc.require: issueI651IG
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest005, testing::ext::TestSize.Level3)
{
    size_t start = 0;
    std::string origin1 = "key:value;";
    auto ret = StringUtil::GetKeyValueByString(start, origin1);
    ASSERT_EQ("key", ret.first);
    ASSERT_EQ("value", ret.second.first);
    std::string origin2 = "key::value;";
    ret = StringUtil::GetKeyValueByString(start, origin2);
    ASSERT_EQ(";", ret.first);
    ASSERT_EQ("", ret.second.first);
    std::string origin3 = "key:value;;";
    ret = StringUtil::GetKeyValueByString(start, origin3);
    ASSERT_EQ("", ret.first);
    ASSERT_EQ("", ret.second.first);
}

/**
 * @tc.name: BaseUtilityUnitTest006
 * @tc.desc: Test IsValidFloatNum defined in namespace StringUtil
 * @tc.type: FUNC
 * @tc.require: issueI651IG
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest006, testing::ext::TestSize.Level3)
{
    auto ret = StringUtil::IsValidFloatNum("123+456");
    ASSERT_TRUE(!ret);
    ret = StringUtil::IsValidFloatNum("123.45.6");
    ASSERT_TRUE(!ret);
    ret = StringUtil::IsValidFloatNum("123.");
    ASSERT_TRUE(!ret);
    ret = StringUtil::IsValidFloatNum(".123");
    ASSERT_TRUE(!ret);
    ret = StringUtil::IsValidFloatNum("123.45");
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: BaseUtilityUnitTest007
 * @tc.desc: Test GetLeft/RightSubstr defined in namespace StringUtil
 * @tc.type: FUNC
 * @tc.require: issueI651IG
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest007, testing::ext::TestSize.Level3)
{
    std::string origin = "left:right";
    auto ret = StringUtil::GetLeftSubstr(origin, ":");
    ASSERT_EQ("left", ret);
    ret = StringUtil::GetLeftSubstr(origin, "v");
    ASSERT_EQ("left:right", ret);
    ret = StringUtil::GetRightSubstr(origin, ":");
    ASSERT_EQ("right", ret);
    StringUtil::GetRightSubstr(origin, "v");
    ASSERT_EQ("right", ret);
}

/**
 * @tc.name: BaseUtilityUnitTest008
 * @tc.desc: Test GetRleft/RrightSubstr defined in namespace StringUtil
 * @tc.type: FUNC
 * @tc.require: issueI651IG
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest008, testing::ext::TestSize.Level3)
{
    std::string origin = "left:right";
    auto ret = StringUtil::GetRleftSubstr(origin, ":");
    ASSERT_EQ("left", ret);
    ret = StringUtil::GetRleftSubstr(origin, "v");
    ASSERT_EQ("left:right", ret);
    ret = StringUtil::GetRrightSubstr(origin, ":");
    ASSERT_EQ("right", ret);
    StringUtil::GetRrightSubstr(origin, "v");
    ASSERT_EQ("right", ret);
}

/**
 * @tc.name: BaseUtilityUnitTest009
 * @tc.desc: Test EraseString defined in namespace StringUtil
 * @tc.type: FUNC
 * @tc.require: issueI651IG
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest009, testing::ext::TestSize.Level3)
{
    std::string input = "123abc123def123g";
    std::string toErase = "123";
    auto ret = StringUtil::EraseString(input, toErase);
    ASSERT_EQ("abcdefg", ret);
}

/**
 * @tc.name: BaseUtilityUnitTest010
 * @tc.desc: Test GetMidSubstr defined in namespace StringUtil
 * @tc.type: FUNC
 * @tc.require: issueI651IG
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest010, testing::ext::TestSize.Level3)
{
    std::string input = "123abc678";
    std::string begin1 = "1234";
    std::string begin2 = "123";
    std::string end1 = "5678";
    std::string end2 = "678";
    auto ret = StringUtil::GetMidSubstr(input, begin1, end1);
    ASSERT_EQ("", ret);
    ret = StringUtil::GetMidSubstr(input, begin2, end1);
    ASSERT_EQ("", ret);
    ret = StringUtil::GetMidSubstr(input, begin2, end2);
    ASSERT_EQ("abc", ret);
}

/**
 * @tc.name: BaseUtilityUnitTest011
 * @tc.desc: Test VectorToString defined in namespace StringUtil
 * @tc.type: FUNC
 * @tc.require: issueI651IG
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest011, testing::ext::TestSize.Level3)
{
    std::vector<std::string> origin = {
        "123", "", "456"
    };
    auto ret = StringUtil::VectorToString(origin, true, ":");
    ASSERT_EQ("456:123:", ret);
    ret = StringUtil::VectorToString(origin, false, ":");
    ASSERT_EQ("123:456:", ret);
}

/**
 * @tc.name: BaseUtilityUnitTest012
 * @tc.desc: Test StringToUl/Double defined in namespace StringUtil
 * @tc.type: FUNC
 * @tc.require: issueI651IG
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest012, testing::ext::TestSize.Level3)
{
    auto ret1 = StringUtil::StringToUl(std::to_string(ULONG_MAX));
    ASSERT_EQ(0, ret1);
    auto ulVal1 = 123;
    ret1 = StringUtil::StringToUl(std::to_string(ulVal1));
    ASSERT_EQ(ulVal1, ret1);
    double standard = 0;
    auto ret2 = StringUtil::StringToDouble("");
    ASSERT_TRUE(fabs(ret2 - standard) < std::numeric_limits<double>::epsilon());
    ret2 = StringUtil::StringToDouble("abc");
    ASSERT_TRUE(fabs(ret2 - standard) < std::numeric_limits<double>::epsilon());
    double origin = 3.3;
    ret2 = StringUtil::StringToDouble(std::to_string(origin));
    ASSERT_TRUE(fabs(ret2 - origin) < std::numeric_limits<double>::epsilon());
}

/**
 * @tc.name: BaseUtilityUnitTest013
 * @tc.desc: Test FindMatchSubString defined in namespace StringUtil
 * @tc.type: FUNC
 * @tc.require: issueI651IG
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest013, testing::ext::TestSize.Level3)
{
    std::string target = "123456abcdefh";
    std::string begin1 = "xyz";
    std::string end1 = "dfh";
    int offset = 0;
    auto ret = StringUtil::FindMatchSubString(target, begin1, offset, end1);
    ASSERT_EQ("", ret);
    int offset1 = 100;
    std::string begin2 = "123";
    ret = StringUtil::FindMatchSubString(target, begin2, offset, end1);
    ASSERT_EQ("123456abc", ret);
    std::string end2 = "opq";
    ret = StringUtil::FindMatchSubString(target, begin2, offset, end2);
    ASSERT_EQ("123456abcdefh", ret);
    ret = StringUtil::FindMatchSubString(target, begin1, offset1, end1);
    ASSERT_EQ("", ret);
}

/**
 * @tc.name: BaseUtilityUnitTest014
 * @tc.desc: Test Escape/UnescapeJsonStringValue defined in namespace StringUtil
 * @tc.type: FUNC
 * @tc.require: issueI651IG
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest014, testing::ext::TestSize.Level3)
{
    std::string origin1 = "abcd1234456\\\"\b\f\n\r\t";
    auto escapedStr = StringUtil::EscapeJsonStringValue(origin1);
    ASSERT_EQ("abcd1234456\\\\\\\"\\b\\f\\n\\r\\t", escapedStr);
    std::string origin2 = "/abcd1234456\\\"\b\f\n\r\t";
    auto unescapedStr = StringUtil::UnescapeJsonStringValue(origin2);
    ASSERT_EQ("/abcd1234456\"\b\f\n\r\t", unescapedStr);
}

/**
 * @tc.name: BaseUtilityUnitTest015
 * @tc.desc: Test SplitStr defined in namespace StringUtil
 * @tc.type: FUNC
 * @tc.require: issueI651IG
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest015, testing::ext::TestSize.Level3)
{
    std::string origin = "1234:5678:abcd";
    auto ret1 = StringUtil::SplitStr(origin, ':');
    size_t expectSize = 3;
    ASSERT_TRUE(ret1.size() == expectSize);
    auto element = ret1.front();
    ASSERT_EQ("1234", element);
    ret1.pop_front();
    element = ret1.front();
    ASSERT_EQ("5678", element);
    ret1.pop_front();
    element = ret1.front();
    ASSERT_EQ("abcd", element);
    ret1.pop_front();
    std::vector<std::string> ret2;
    StringUtil::SplitStr(origin, ":", ret2);
    ASSERT_TRUE(ret2.size() == expectSize);
}

/**
 * @tc.name: BaseUtilityUnitTest016
 * @tc.desc: Test HideDeviceId defined in namespace StringUtil
 * @tc.type: FUNC
 * @tc.require: issueIAR8QU
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest016, testing::ext::TestSize.Level3)
{
    std::string originStr1 = "TEST_VAL_ABBBBB123BBBDASDDASDASDASDASDASDASDSADASDSDSADSASAD333444422_V";
    auto ret = StringUtil::HideDeviceIdInfo(originStr1);
    ASSERT_EQ(ret, "TEST_VAL_******_V");
    originStr1 = "TEST_VAL_ABBBBB123BBBDASDDASDASDASDASDASDASDSADASDSDSADSASAD333444422ABCDE_V";
    ret = StringUtil::HideDeviceIdInfo(originStr1);
    ASSERT_EQ(ret, "TEST_VAL_******_V");
    originStr1 = "TEST_VAL_ABBBBB123BBBDASDDASDASDASDASDASDASDSADASDSDSADSASAD33344442_V";
    ret = StringUtil::HideDeviceIdInfo(originStr1);
    ASSERT_EQ(ret, "TEST_VAL_ABBBBB123BBBDASDDASDASDASDASDASDASDSADASDSDSADSASAD33344442_V");
    originStr1 = "TEST_VAL_ABBBBB123BBBDASDDASDASDASDASDASDASDSADASDSDSADSASAD333444422ABCDEF_V";
    ret = StringUtil::HideDeviceIdInfo(originStr1);
    ASSERT_EQ(ret, "TEST_VAL_ABBBBB123BBBDASDDASDASDASDASDASDASDSADASDSDSADSASAD333444422ABCDEF_V");
}

/**
 * @tc.name: BaseUtilityUnitTest017
 * @tc.desc: Test IsValidRegex defined in namespace StringUtil
 * @tc.type: FUNC
 * @tc.require: issueIAWMX3
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest017, testing::ext::TestSize.Level3)
{
    ASSERT_TRUE(!StringUtil::IsValidRegex("[a-z]+("));
}

/**
 * @tc.name: BaseUtilityUnitTest018
 * @tc.desc: Test GetUserId defined in namespace FileUtil
 * @tc.type: FUNC
 * @tc.require: issueIC8IZD
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest018, testing::ext::TestSize.Level3)
{
    int userId = FileUtil::GetUserId(20020193);
    ASSERT_EQ(userId, 100);
    userId = FileUtil::GetUserId(1201);
    ASSERT_EQ(userId, 0);
}

/**
 * @tc.name: BaseUtilityUnitTest019
 * @tc.desc: Test GetSandBoxLogPath defined in namespace FileUtil
 * @tc.type: FUNC
 * @tc.require: issueIC8IZD
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest019, testing::ext::TestSize.Level3)
{
    std::string sandBox = FileUtil::GetSandBoxLogPath(20020193, "com.ohos.xxx", "resourcelimit");
    ASSERT_EQ(sandBox, "/data/app/el2/100/log/com.ohos.xxx/resourcelimit");
    sandBox = FileUtil::GetSandBoxLogPath(20020193, "com.ohos.xxx", "watchdog");
    ASSERT_EQ(sandBox, "/data/app/el2/100/log/com.ohos.xxx/watchdog");
    sandBox = FileUtil::GetSandBoxLogPath(20020193, "com.ohos.xxx", "hiappevent");
    ASSERT_EQ(sandBox, "/data/app/el2/100/log/com.ohos.xxx/hiappevent");
}

/**
 * @tc.name: BaseUtilityUnitTest020
 * @tc.desc: Test GetSandBoxBasePath defined in namespace FileUtil
 * @tc.type: FUNC
 * @tc.require: issueIC8IZD
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest020, testing::ext::TestSize.Level3)
{
    std::string sandBox = FileUtil::GetSandBoxBasePath(20020193, "com.ohos.xxx");
    ASSERT_EQ(sandBox, "/data/app/el2/100/base/com.ohos.xxx/cache/hiappevent");
}

/**
 * @tc.name: BaseUtilityUnitTest021
 * @tc.desc: Test GetDeviceValidSize defined in namespace FileUtil
 * @tc.type: FUNC
 * @tc.require: issueIC8IZD
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest021, testing::ext::TestSize.Level3)
{
    double size = FileUtil::GetDeviceValidSize("/data");
    ASSERT_TRUE(size > 0);
    size = FileUtil::GetDeviceValidSize("/test");
    ASSERT_EQ(size, 0);
}

/**
 * @tc.name: BaseUtilityUnitTest022
 * @tc.desc: Test Sleep defined in namespace FileUtil
 * @tc.type: FUNC
 * @tc.require: issueICIL48
 */
HWTEST_F(BaseUtilityUnitTest, BaseUtilityUnitTest022, testing::ext::TestSize.Level3)
{
    ffrt::submit([] () {
        auto ts1 = static_cast<int64_t>(TimeUtil::GetSteadyClockTimeMs());
        int64_t WAIT_TIME = 11; // 11 seconds is a test duration to sleep
        FfrtUtil::Sleep(WAIT_TIME); // sleep 11 seconds
        auto ts2 = static_cast<int64_t>(TimeUtil::GetSteadyClockTimeMs());
        ASSERT_GE(ts2 - WAIT_TIME * TimeUtil::SEC_TO_MILLISEC, ts1);
    }, {}, {}, ffrt::task_attr().name("base_util_ut_022").qos(ffrt::qos_default));
}
} // namespace HiviewDFX
} // namespace OHOS
