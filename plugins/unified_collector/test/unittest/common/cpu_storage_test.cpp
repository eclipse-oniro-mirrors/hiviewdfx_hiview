/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "cpu_storage_test.h"

#include "gmock/gmock-matchers.h"
#include "power_status_manager.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;

void CpuStorageTest::SetUpTestCase()
{
}

void CpuStorageTest::TearDownTestCase()
{
}

void CpuStorageTest::SetUp()
{
    platform.GetPluginMap();
}

void CpuStorageTest::TearDown()
{
}

/**
 * @tc.name: CpuStorageTest001
 * @tc.desc: CpuStorage test PowerStatusManager
 * @tc.type: FUNC
 * @tc.require: issueI5NULM
 */
#ifdef POWER_MANAGER_ENABLE
HWTEST_F(CpuStorageTest, CpuStorageTest001, TestSize.Level3)
{
    int32_t powerState = UCollectUtil::PowerStatusManager::GetInstance().GetPowerState();
    ASSERT_THAT(powerState, testing::AnyOf(UCollectUtil::SCREEN_OFF, UCollectUtil::SCREEN_ON));
    UCollectUtil::PowerStatusManager::GetInstance().SetPowerState(UCollectUtil::SCREEN_ON);
    int32_t powerState2 = UCollectUtil::PowerStatusManager::GetInstance().GetPowerState();
    ASSERT_EQ(powerState2, UCollectUtil::SCREEN_ON);
    UCollectUtil::PowerStatusManager::GetInstance().SetPowerState(UCollectUtil::SCREEN_OFF);
    int32_t powerState3 = UCollectUtil::PowerStatusManager::GetInstance().GetPowerState();
    ASSERT_EQ(powerState3, UCollectUtil::SCREEN_OFF);
}
#endif
