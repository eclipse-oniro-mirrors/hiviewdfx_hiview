/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "event_export_mgr_test.h"

#include "setting_observer_manager.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr char PARAM_NAME[] = "PARAM_NAME";
constexpr char DEFAULT_VAL[] = "DEFAULT_VAL";
}

void EventExportMgrTest::SetUpTestCase()
{
}

void EventExportMgrTest::TearDownTestCase()
{
}

void EventExportMgrTest::SetUp()
{
}

void EventExportMgrTest::TearDown()
{
}

/**
 * @tc.name: EventExportMgrTest001
 * @tc.desc: Test apis of SettingObserverManager
 * @tc.type: FUNC
 * @tc.require: issueI9GHI8
 */
HWTEST_F(EventExportMgrTest, EventExportMgrTest001, testing::ext::TestSize.Level3)
{
    SettingObserver::ObserverCallback callback =
        [] (const std::string& paramKey) {
            // do nothing
        };
    ASSERT_TRUE(SettingObserverManager::GetInstance()->RegisterObserver(PARAM_NAME, callback));
    // observer exit
    ASSERT_TRUE(SettingObserverManager::GetInstance()->RegisterObserver(PARAM_NAME, callback));
    ASSERT_TRUE(SettingObserverManager::GetInstance()->UnregisterObserver(PARAM_NAME));
    // observer not found
    ASSERT_TRUE(SettingObserverManager::GetInstance()->UnregisterObserver(PARAM_NAME));
    auto value = SettingObserverManager::GetInstance()->GetStringValue(PARAM_NAME, DEFAULT_VAL);
    ASSERT_EQ(value, DEFAULT_VAL);
}
} // namespace HiviewDFX
} // namespace OHOS