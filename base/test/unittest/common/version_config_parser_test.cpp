/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
inline constexpr uint8_t BETA_COLLECT = 0b0001;
inline constexpr uint8_t COMM_COLLECT = 0b0010;
inline constexpr uint8_t BETA_PRESERVE = 0b0100;
inline constexpr uint8_t COMM_PRESERVE = 0b1000;

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
 * @tc.name: ParseConfigTest001
 * @tc.desc: Test ParseConfig with bool value
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ParseConfigTest001, testing::ext::TestSize.Level0)
{
    Json::Value jsonValue;
    jsonValue["preserve"] = true;
    jsonValue["collect"] = false;
    
    VersionConfigParser parser(jsonValue);
    ASSERT_TRUE(parser.ShouldPreserve());
    ASSERT_FALSE(parser.ShouldCollect());
}

/**
 * @tc.name: ParseConfigTest002
 * @tc.desc: Test ParseConfig with uint value
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ParseConfigTest002, testing::ext::TestSize.Level0)
{
    Json::Value jsonValue;
    jsonValue["preserve"] = 1;
    jsonValue["collect"] = 2;
    
    VersionConfigParser parser(jsonValue);
    if (Parameter::IsBetaVersion()) {
        ASSERT_TRUE(parser.ShouldPreserve());
        ASSERT_FALSE(parser.ShouldCollect());
    } else {
        ASSERT_TRUE(parser.ShouldPreserve());
        ASSERT_TRUE(parser.ShouldCollect());
    }
}

/**
 * @tc.name: ParseConfigTest003
 * @tc.desc: Test ParseConfig with missing fields
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ParseConfigTest003, testing::ext::TestSize.Level0)
{
    Json::Value jsonValue;
    jsonValue["invalid"] = "";
    VersionConfigParser parser(jsonValue);
    ASSERT_TRUE(parser.ShouldPreserve());
    ASSERT_FALSE(parser.ShouldCollect());
}

/**
 * @tc.name: ParseConfigTest004
 * @tc.desc: Test ParseConfig with invalid values
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ParseConfigTest004, testing::ext::TestSize.Level0)
{
    Json::Value jsonValue;
    jsonValue["preserve"] = "invalid";
    jsonValue["collect"] = -1;

    VersionConfigParser parser(jsonValue);
    ASSERT_TRUE(parser.ShouldPreserve());
    ASSERT_FALSE(parser.ShouldCollect());
}

/**
 * @tc.name: ParseConfigTest006
 * @tc.desc: Test ParseConfig with mixed types
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ParseConfigTest006, testing::ext::TestSize.Level0)
{
    Json::Value jsonValue;
    jsonValue["preserve"] = true;
    jsonValue["collect"] = 1;
    
    VersionConfigParser parser(jsonValue);
    ASSERT_TRUE(parser.ShouldPreserve());
    ASSERT_TRUE(parser.ShouldCollect());
}

/**
 * @tc.name: ShouldCollectTest004
 * @tc.desc: Test ShouldCollect with all possible values
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ShouldCollectTest004, testing::ext::TestSize.Level0)
{
    // Test all possible values for collect
    for (int i = 0; i <= 3; ++i) {
        Json::Value jsonValue;
        jsonValue["collect"] = i;
        VersionConfigParser parser(jsonValue);
        
        if (Parameter::IsBetaVersion()) {
            if (i == 1 || i == 3) {
                ASSERT_TRUE(parser.ShouldCollect());
            } else {
                ASSERT_FALSE(parser.ShouldCollect());
            }
        } else {
            if (i == 1 || i == 2) {
                ASSERT_TRUE(parser.ShouldCollect());
            } else {
                ASSERT_FALSE(parser.ShouldCollect());
            }
        }
    }
}

/**
 * @tc.name: ShouldPreserveTest004
 * @tc.desc: Test ShouldPreserve with all possible values
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ShouldPreserveTest004, testing::ext::TestSize.Level0)
{
    // Test all possible values for preserve
    for (int i = 0; i <= 3; ++i) {
        Json::Value jsonValue;
        jsonValue["preserve"] = i;
        VersionConfigParser parser(jsonValue);
        
        if (Parameter::IsBetaVersion()) {
            if (i == 1 || i == 3) {
                ASSERT_TRUE(parser.ShouldPreserve());
            } else {
                ASSERT_FALSE(parser.ShouldPreserve());
            }
        } else {
            if (i == 1 || i == 2) {
                ASSERT_TRUE(parser.ShouldPreserve());
            } else {
                ASSERT_FALSE(parser.ShouldPreserve());
            }
        }
    }
}
} // namespace HiviewDFX
} // namespace OHOS
