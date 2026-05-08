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
    static void SetParameter(const std::string& key, const std::string& value)
    {
        // Mock implementation for testing
        if (key == "const.hiview.isBeta") {
            isBeta_ = value == "true";
        }
    }

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
    Json::Value jsonValue = true;
    VersionConfigParser parser(jsonValue);
    ASSERT_TRUE(parser.ShouldCollect()); // Indirect test of ParsePreserveCollectConfig
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
    VersionConfigParser parser(jsonValue);
    ASSERT_TRUE(parser.ShouldCollect()); // Indirect test of ParsePreserveCollectConfig
}

/**
 * @tc.name: ShouldCollectTest001
 * @tc.desc: Test ShouldCollect with BETA_COLLECT and beta version
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ShouldCollectTest001, testing::ext::TestSize.Level0)
{
    ParameterMock::SetParameter("const.hiview.isBeta", "true");
    Json::Value jsonValue;
    jsonValue["collect"]["beta"] = true;
    VersionConfigParser parser(jsonValue);
    ASSERT_TRUE(parser.ShouldCollect());
}

/**
 * @tc.name: ShouldCollectTest002
 * @tc.desc: Test ShouldCollect with COMM_COLLECT and commercial version
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ShouldCollectTest002, testing::ext::TestSize.Level0)
{
    ParameterMock::SetParameter("const.hiview.isBeta", "false");
    Json::Value jsonValue;
    jsonValue["collect"]["commercial"] = true;
    VersionConfigParser parser(jsonValue);
    ASSERT_TRUE(parser.ShouldCollect());
}

/**
 * @tc.name: ShouldPreserveTest001
 * @tc.desc: Test ShouldPreserve with BETA_PRESERVE and beta version
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ShouldPreserveTest001, testing::ext::TestSize.Level0)
{
    ParameterMock::SetParameter("const.hiview.isBeta", "true");
    Json::Value jsonValue;
    jsonValue["preserve"]["beta"] = true;
    VersionConfigParser parser(jsonValue);
    ASSERT_TRUE(parser.ShouldPreserve());
}

/**
 * @tc.name: ShouldPreserveTest002
 * @tc.desc: Test ShouldPreserve with COMM_PRESERVE and commercial version
 * @tc.type: FUNC
 */
HWTEST_F(VersionConfigParserTest, ShouldPreserveTest002, testing::ext::TestSize.Level0)
{
    ParameterMock::SetParameter("const.hiview.isBeta", "false");
    Json::Value jsonValue;
    jsonValue["preserve"]["commercial"] = true;
    VersionConfigParser parser(jsonValue);
    ASSERT_TRUE(parser.ShouldPreserve());
}
} // namespace HiviewDFX
} // namespace OHOS
