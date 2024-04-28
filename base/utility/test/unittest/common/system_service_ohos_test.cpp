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

#include "system_service_ohos_test.h"

#include "parameter_ex.h"

using namespace std;

namespace OHOS {
namespace HiviewDFX {
void SystemServiceOhosTest::SetUpTestCase() {}

void SystemServiceOhosTest::TearDownTestCase() {}

void SystemServiceOhosTest::SetUp() {}

void SystemServiceOhosTest::TearDown() {}

/**
 * @tc.name: SystemServiceTest001
 * @tc.desc: Check beta version
 * @tc.type: FUNC
 * @tc.require: issueI64Q4L
 */
HWTEST_F(SystemServiceOhosTest, SystemServiceTest001, testing::ext::TestSize.Level3)
{
    auto ret = Parameter::IsBetaVersion();
    auto userType = Parameter::GetString(KEY_HIVIEW_VERSION_TYPE, "unknown");
    ASSERT_TRUE(ret == (userType.find("beta") != std::string::npos));
}

/**
 * @tc.name: SystemServiceTest002
 * @tc.desc: Test getter
 * @tc.type: FUNC
 * @tc.require: issueI64QXL
 */
HWTEST_F(SystemServiceOhosTest, SystemServiceTest003, testing::ext::TestSize.Level3)
{
    auto ret1 = Parameter::GetUnsignedInteger("TEST_KEY1", 200);
    ASSERT_TRUE(ret1 == 200);
    auto ret2 = Parameter::GetBoolean("TEST_KEY2", true);
    ASSERT_TRUE(ret2);
    auto ret3 = Parameter::SetProperty("TEST_KEY3", "1234");
    ASSERT_TRUE(ret3);
    auto ret4 = Parameter::GetString("TEST_KEY3", "");
    ASSERT_TRUE(ret4 == "1234");
    auto ret5 = Parameter::GetInteger("TEST_KEY3", 0);
    ASSERT_TRUE(ret5 == 1234);
}
}
}