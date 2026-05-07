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

class VersionConfigParserTest : public testing::Test {
protected:
    void SetUp() override {
        // You can initialize the resources required for testing here.
    }

    void TearDown() override {
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
    Json::Value jsonValue = true;
    VersionConfigParser parser;
    uint8_t controlTag = parser.ParsePreserveCollectConfig(jsonValue);
    ASSERT_EQ(controlTag, BETA_COLLECT);
}

/**
 * @tc.name: ParsePreserveCollectConfigTest002
 * @tc.desc: Test ParsePreserveCollectConfig with object value
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ParsePreserveCollectConfigTest002, testing::ext::TestSize.Level0)
{
    Json::Value jsonValue;
    jsonValue["preserve"]["beta"] = true;
    jsonValue["collect"]["beta"] = true;
    VersionConfigParser parser;
    uint8_t controlTag = parser.ParsePreserveCollectConfig(jsonValue);
    ASSERT_EQ(controlTag, BETA_COLLECT | BETA_PRESERVE);
}

/**
 * @tc.name: ShouldCollectTest001
 * @tc.desc: Test ShouldCollect with BETA_COLLECT and beta version
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ShouldCollectTest001, testing::ext::TestSize.Level0)
{
    Parameter::SetParameter("const.hiview.isBeta", "true");
    VersionConfigParser parser;
    ASSERT_TRUE(parser.ShouldCollect(BETA_COLLECT));
    ASSERT_FALSE(parser.ShouldCollect(COMM_COLLECT));
}

/**
 * @tc.name: ShouldCollectTest002
 * @tc.desc: Test ShouldCollect with COMM_COLLECT and commercial version
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ShouldCollectTest002, testing::ext::TestSize.Level0)
{
    Parameter::SetParameter("const.hiview.isBeta", "false");
    VersionConfigParser parser;
    ASSERT_TRUE(parser.ShouldCollect(COMM_COLLECT));
    ASSERT_FALSE(parser.ShouldCollect(BETA_COLLECT));
}

/**
 * @tc.name: ShouldPreserveTest001
 * @tc.desc: Test ShouldPreserve with BETA_PRESERVE and beta version
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ShouldPreserveTest001, testing::ext::TestSize.Level0)
{
    Parameter::SetParameter("const.hiview.isBeta", "true");
    VersionConfigParser parser;
    ASSERT_TRUE(parser.ShouldPreserve(BETA_PRESERVE));
    ASSERT_FALSE(parser.ShouldPreserve(COMM_PRESERVE));
}

/**
 * @tc.name: ShouldPreserveTest002
 * @tc.desc: Test ShouldPreserve with COMM_PRESERVE and commercial version
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ShouldPreserveTest002, testing::ext::TestSize.Level0)
{
    Parameter::SetParameter("const.hiview.isBeta", "false");
    VersionConfigParser parser;
    ASSERT_TRUE(parser.ShouldPreserve(COMM_PRESERVE));
    ASSERT_FALSE(parser.ShouldPreserve(BETA_PRESERVE));
}

} // namespace HiviewDFX
} // namespace OHOS
