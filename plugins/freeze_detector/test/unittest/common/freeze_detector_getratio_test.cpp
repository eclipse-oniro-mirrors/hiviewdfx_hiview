/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "freeze_detector_getratio_test.h"
 
#include <gtest/gtest.h>
#include "parameter.h"
#include "parameters.h"
#include "freeze_common.h"
#include "get_ratio_utils.h"
 
using namespace testing::ext;
using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace HiviewDFX {
    constexpr float FLOAT_EPSILON = 0.01f;
    constexpr const char* DFX_ABILITYMS_TIMEOUT_RATIO = "persist.sys.abilityms.timeout_unit_time_ratio";
    constexpr const char* DFX_APPFREEZE_TIMEOUT_RATIO = "const.sys.dfx.appfreeze.timeout_unit_time_ratio";
 
void FreezeDetectorGetratiotest::SetUp()
{
    /**
     * @tc.setup: create work directories
     */
    printf("SetUp.\n");
}
void FreezeDetectorGetratiotest::SetUpTestCase()
{
    /**
     * @tc.setup: all first
     */
    printf("SetUpTestCase.\n");
}
 
void FreezeDetectorGetratiotest::TearDownTestCase()
{
    /**
     * @tc.setup: all end
     */
    printf("TearDownTestCase.\n");
}
 
void FreezeDetectorGetratiotest::TearDown()
{
    /**
     * @tc.teardown: destroy the event loop we have created
     */
    printf("TearDown.\n");
}
 
/**
 * @tc.name: FreezeDetectorGetratiotest_IsNumeric_001
 * @tc.desc: Test IsNumeric
 * @tc.type: FUNC
 */
HWTEST_F(FreezeDetectorGetratiotest, FreezeDetectorGetratiotest_IsNumeric_001, TestSize.Level1)
{
    bool ret = FreezeGetRatio::GetInstance()->IsNumeric("1500");
    EXPECT_TRUE(ret);
}
 
/**
 * @tc.name: FreezeDetectorGetratiotest_IsNumeric_002
 * @tc.desc: Test IsNumeric
 * @tc.type: FUNC
 */
HWTEST_F(FreezeDetectorGetratiotest, FreezeDetectorGetratiotest_IsNumeric_002, TestSize.Level1)
{
    bool ret = FreezeGetRatio::GetInstance()->IsNumeric("1a2b");
    EXPECT_FALSE(ret);
}
 
/**
 * @tc.name: FreezeDetectorGetratiotest_IsNumeric_003
 * @tc.desc: Test IsNumeric
 * @tc.type: FUNC
 */
HWTEST_F(FreezeDetectorGetratiotest, FreezeDetectorGetratiotest_IsNumeric_003, TestSize.Level1)
{
    bool ret = FreezeGetRatio::GetInstance()->IsNumeric("abc");
    EXPECT_FALSE(ret);
}
 
/**
 * @tc.name: FreezeDetectorGetratiotest_IsNumeric_004
 * @tc.desc: Test IsNumeric
 * @tc.type: FUNC
 */
HWTEST_F(FreezeDetectorGetratiotest, FreezeDetectorGetratiotest_IsNumeric_004, TestSize.Level1)
{
    bool ret = FreezeGetRatio::GetInstance()->IsNumeric("");
    EXPECT_FALSE(ret);
}
 
/**
 * @tc.name: FreezeDetectorGetratiotest_getRatio_001
 * @tc.desc: Test getRatio
 * @tc.type: FUNC
 */
HWTEST_F(FreezeDetectorGetratiotest, FreezeDetectorGetratiotest_getRatio_001, TestSize.Level1)
{
    float result = FreezeGetRatio::GetInstance()->GetAppfreezeTimeoutRatio();
    EXPECT_TRUE(std::abs(result - 1.0f) < FLOAT_EPSILON);
    OHOS::system::SetParameter(DFX_APPFREEZE_TIMEOUT_RATIO, "");
    result = FreezeGetRatio::GetInstance()->GetAppfreezeTimeoutRatio();
    EXPECT_TRUE(std::abs(result - 1.0f) < FLOAT_EPSILON);
}
 
/**
 * @tc.name: FreezeDetectorGetratiotest_getRatio_002
 * @tc.desc: Test getRatio
 * @tc.type: FUNC
 */
HWTEST_F(FreezeDetectorGetratiotest, FreezeDetectorGetratiotest_getRatio_002, TestSize.Level1)
{
    OHOS::system::SetParameter(DFX_APPFREEZE_TIMEOUT_RATIO, "1500");
    float result = FreezeGetRatio::GetInstance()->GetAppfreezeTimeoutRatio();
    EXPECT_TRUE(std::abs(result - 1.5f) < FLOAT_EPSILON);
}
 
/**
 * @tc.name: FreezeDetectorGetratiotest_getRatio_003
 * @tc.desc: Test getRatio
 * @tc.type: FUNC
 */
HWTEST_F(FreezeDetectorGetratiotest, FreezeDetectorGetratiotest_getRatio_003, TestSize.Level1)
{
    OHOS::system::SetParameter(DFX_APPFREEZE_TIMEOUT_RATIO, "1");
    float result = FreezeGetRatio::GetInstance()->GetAppfreezeTimeoutRatio();
    EXPECT_TRUE(std::abs(result - 1.0f) < FLOAT_EPSILON);
}
 
/**
 * @tc.name: FreezeDetectorGetratiotest_getRatio_004
 * @tc.desc: Test getRatio
 * @tc.type: FUNC
 */
HWTEST_F(FreezeDetectorGetratiotest, FreezeDetectorGetratiotest_getRatio_004, TestSize.Level1)
{
    float result = FreezeGetRatio::GetInstance()->GetAbilitymsTimeoutRatio();
    EXPECT_TRUE(std::abs(result - 1.0f) < FLOAT_EPSILON);
    OHOS::system::SetParameter(DFX_ABILITYMS_TIMEOUT_RATIO, "");
    result = FreezeGetRatio::GetInstance()->GetAbilitymsTimeoutRatio();
    EXPECT_TRUE(std::abs(result - 1.0f) < FLOAT_EPSILON);
}
 
/**
 * @tc.name: FreezeDetectorGetratiotest_getRatio_005
 * @tc.desc: Test getRatio
 * @tc.type: FUNC
 */
HWTEST_F(FreezeDetectorGetratiotest, FreezeDetectorGetratiotest_getRatio_005, TestSize.Level1)
{
    OHOS::system::SetParameter(DFX_ABILITYMS_TIMEOUT_RATIO, "2");
    float result = FreezeGetRatio::GetInstance()->GetAbilitymsTimeoutRatio();
    EXPECT_TRUE(std::abs(result - 2.0f) < FLOAT_EPSILON);
}
 
/**
 * @tc.name: FreezeDetectorGetratiotest_getRatio_006
 * @tc.desc: Test getRatio
 * @tc.type: FUNC
 */
HWTEST_F(FreezeDetectorGetratiotest, FreezeDetectorGetratiotest_getRatio_006, TestSize.Level1)
{
    OHOS::system::SetParameter(DFX_ABILITYMS_TIMEOUT_RATIO, "0.001");
    float result = FreezeGetRatio::GetInstance()->GetAbilitymsTimeoutRatio();
    EXPECT_TRUE(std::abs(result - 1.0f) < FLOAT_EPSILON);
}
}
}