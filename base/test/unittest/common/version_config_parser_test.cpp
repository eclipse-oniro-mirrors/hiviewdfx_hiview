/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#include <gtest/gtest.h>
#include "version_config_parser.h"
#include "parameter_ex.h"

using namespace OHOS::HiviewDFX;

namespace OHOS {
namespace HiviewDFX {
inline constexpr uint8_t BETA_COLLECT = 0b0001;  // Beta collection
inline constexpr uint8_t COMM_COLLECT = 0b0010;  // Commercial collection
inline constexpr uint8_t BETA_PRESERVE = 0b0100; // Beta Preserve
inline constexpr uint8_t COMM_PRESERVE = 0b1000; // Commercial Preserve

class ParameterMock {
public:

    static bool IsBetaVersion()
    {
        return isBeta_;
    }

private:
    static bool isBeta_;
};

bool ParameterMock::isBeta_ = false;

class VersionConfigParserTest : public testing::Test {
protected:
    void SetUp() override
    {
        // You can initialize the resources required for testing here.
    }

    void TearDown() override
    {
        // You can release the resources needed for testing here.
    }
};

/**
 * @tc.name: ParsePreserveCollectConfigTest001
 * @tc.desc: Test ParsePreserveCollectConfig with bool value
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ParsePreserveCollectConfigTest001, testing::ext::TestSize.Level0)
{
    Json::Value jsonValue;
    jsonValue["preserve"] = true; // Both beta and commercial preserve
    jsonValue["collect"] = true; // Both beta and commercial collect
    VersionConfigParser parser(jsonValue);
    ASSERT_TRUE(parser.ShouldCollect()); // Indirect test of ParsePreserveCollectConfig
    ASSERT_TRUE(parser.ShouldPreserve()); // Indirect test of ParsePreserveCollectConfig
}

/**
 * @tc.name: ParsePreserveCollectConfigTest002
 * @tc.desc: Test ParsePreserveCollectConfig with uint value
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ParsePreserveCollectConfigTest002, testing::ext::TestSize.Level0)
{
    Json::Value jsonValue;
    jsonValue["preserve"] = 1;
    jsonValue["collect"] = 1;

    VersionConfigParser parser(jsonValue);
    ASSERT_TRUE(parser.ShouldCollect()); // Indirect test of ParsePreserveCollectConfig
    ASSERT_TRUE(parser.ShouldPreserve()); // Indirect test of ParsePreserveCollectConfig
}

/**
 * @tc.name: ShouldCollectTest001
 * @tc.desc: Test ShouldCollect with beta version and uint value
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ShouldCollectTest001, testing::ext::TestSize.Level0)
{
    // Test uint input for beta version
    Json::Value jsonValue;
    jsonValue["preserve"] = 2;  // COMMERCIAL_ONLY

    VersionConfigParser parser(jsonValue);
    if (Parameter::IsBetaVersion()) {
        ASSERT_FALSE(parser.ShouldCollect()); //  no collection for beta version
    } else {
        ASSERT_TRUE(parser.ShouldCollect()); // collection for commercial version
    }
}

/**
 * @tc.name: ShouldCollectTest002
 * @tc.desc: Test ShouldCollect with commercial version and uint value
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ShouldCollectTest002, testing::ext::TestSize.Level0)
{
    // Test uint input for commercial version
    Json::Value jsonValue;
    jsonValue["collect"] = 1;   // collection for beta version
    VersionConfigParser parser(jsonValue);

    if (Parameter::IsBetaVersion()) {
        ASSERT_TRUE(parser.ShouldCollect()); //  collection for beta version
    } else {
        ASSERT_FALSE(parser.ShouldCollect()); // No collection for commercial version
    }
}

/**
 * @tc.name: ShouldCollectTest003
 * @tc.desc: Test ShouldCollect with bool value
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ShouldCollectTest003, testing::ext::TestSize.Level0)
{
    // Test bool input
    Json::Value jsonValue;
    jsonValue["collect"] = true; // Enable collection
    VersionConfigParser parser1(jsonValue);

    if (Parameter::IsBetaVersion()) {
        ASSERT_TRUE(parser1.ShouldCollect()); // Enable collection for beta version
    } else {
        ASSERT_TRUE(parser1.ShouldCollect()); // Enable collection for commercial version
    }

    jsonValue["collect"] = false; // Disable collection
    VersionConfigParser parser2(jsonValue);

    if (Parameter::IsBetaVersion()) {
        ASSERT_FALSE(parser2.ShouldCollect()); // No collection for beta version
    } else {
        ASSERT_FALSE(parser2.ShouldCollect()); // No collection for commercial version
    }
}

/**
 * @tc.name: ShouldPreserveTest001
 * @tc.desc: Test ShouldPreserve with beta version and uint value
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ShouldPreserveTest001, testing::ext::TestSize.Level0)
{
    // Test uint input for beta version
    Json::Value jsonValue;
    jsonValue["preserve"] = 3; // Enable preserve for beta version
    VersionConfigParser parser(jsonValue);

    if (Parameter::IsBetaVersion()) {
        ASSERT_TRUE(parser.ShouldPreserve()); // Enable preserve for beta version
    } else {
        ASSERT_FALSE(parser.ShouldPreserve()); // No preserve for commercial version
    }
}

/**
 * @tc.name: ShouldPreserveTest002
 * @tc.desc: Test ShouldPreserve with commercial version and uint value
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ShouldPreserveTest002, testing::ext::TestSize.Level0)
{
    // Test uint input for commercial version
    Json::Value jsonValue;
    jsonValue["preserve"] = 2; // Enable preserve for commercial version
    VersionConfigParser parser(jsonValue);

    if (Parameter::IsBetaVersion()) {
        ASSERT_FALSE(parser.ShouldPreserve()); // No preserve for beta version
    } else {
        ASSERT_TRUE(parser.ShouldPreserve()); // Enable preserve for commercial version
    }
}

/**
 * @tc.name: ShouldPreserveTest003
 * @tc.desc: Test ShouldPreserve with bool value
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ShouldPreserveTest003, testing::ext::TestSize.Level0)
{
    // Test bool input
    Json::Value jsonValue;
    jsonValue["preserve"] = true; // Enable preserve
    VersionConfigParser parser1(jsonValue);
    ASSERT_TRUE(parser1.ShouldPreserve()); // Enable preserve for beta version

    jsonValue["preserve"] = false; // Disable preserve
    VersionConfigParser parser2(jsonValue);
    ASSERT_FALSE(parser2.ShouldPreserve()); // No preserve for beta version
}

/**
 * @tc.name: ShouldCollectTest004
 * @tc.desc: Test ShouldCollect with uint value 0/1/2/3
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ShouldCollectTest004, testing::ext::TestSize.Level0)
{
    // Test uint input for beta version
    Json::Value jsonValue;
    jsonValue["collect"] = 3;
    VersionConfigParser parser(jsonValue);

    if (Parameter::IsBetaVersion()) {
        ASSERT_TRUE(parser.ShouldCollect()); // Enable collection for beta version
    } else {
        ASSERT_FALSE(parser.ShouldCollect()); // no  collection for commercial version
    }

    jsonValue["collect"] = 0; // No collection for beta and commercial version
    VersionConfigParser parser2(jsonValue);

    if (Parameter::IsBetaVersion()) {
        ASSERT_FALSE(parser2.ShouldCollect()); // No collection for beta version
    } else {
        ASSERT_FALSE(parser2.ShouldCollect()); // No collection for commercial version
    }
}

/**
 * @tc.name: ShouldPreserveTest004
 * @tc.desc: Test ShouldPreserve with uint value 0/1/2/3
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ShouldPreserveTest004, testing::ext::TestSize.Level0)
{
    // Test uint input for beta version
    Json::Value jsonValue;
    jsonValue["collect"] = 1;
    VersionConfigParser parser(jsonValue);

    if (Parameter::IsBetaVersion()) {
        ASSERT_FALSE(parser.ShouldPreserve()); // Enable preserve for beta version
    } else {
        ASSERT_TRUE(parser.ShouldPreserve()); // no Enable preserve for commercial version
    }

    jsonValue["collect"] = 1; // No preserve for beta and commercial version
    VersionConfigParser parser2(jsonValue);

    if (Parameter::IsBetaVersion()) {
        ASSERT_FALSE(parser2.ShouldPreserve());
    } else {
        ASSERT_TRUE(parser2.ShouldPreserve());
    }
}
} // namespace HiviewDFX
} // namespace OHOS
