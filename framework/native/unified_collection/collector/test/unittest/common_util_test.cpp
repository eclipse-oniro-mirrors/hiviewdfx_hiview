/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "common_util.h"
#include "time_util.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;

class CommonUtilTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

/**
 * @tc.name: CommonUtilTest001
 * @tc.desc: used to test common util in UCollect
 * @tc.type: FUNC
*/
HWTEST_F(CommonUtilTest, CommonUtilTest001, TestSize.Level1)
{
    std::string testStr;
    std::string type;
    int64_t value = 0;
    ASSERT_EQ(CommonUtil::ParseTypeAndValue(testStr, type, value), false);
    testStr = "key: 1";
    ASSERT_EQ(CommonUtil::ParseTypeAndValue(testStr, type, value), true);
    ASSERT_EQ(type, "key");
    ASSERT_EQ(value, 1);
    testStr = "key: 1 kB";
    ASSERT_EQ(CommonUtil::ParseTypeAndValue(testStr, type, value), true);
    ASSERT_EQ(type, "key");
    ASSERT_EQ(value, 1);
}

/**
 * @tc.name: CommonUtilTest003
 * @tc.desc: used to test common util in UCollect
 * @tc.type: FUNC
*/
HWTEST_F(CommonUtilTest, CommonUtilTest003, TestSize.Level1)
{
    int32_t ret = CommonUtil::ReadNodeWithOnlyNumber("invalid_path");
    ASSERT_EQ(ret, 0);
    ret = CommonUtil::ReadNodeWithOnlyNumber("/proc/1/oom_score_adj");
    ASSERT_NE(ret, 0);
}
