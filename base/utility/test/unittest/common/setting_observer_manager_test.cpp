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
#include "setting_observer_manager_test.h"

#include <string>

#include "setting_observer_manager.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;

namespace {
constexpr int FAILED_TO_OPT_DATA_SHARE = -2;
}

/**
 * @tc.name: SettingObserverOnChangeTest001
 * @tc.desc: test SettingObserver OnChange invokes callback with correct key
 * @tc.type: FUNC
 */
HWTEST_F(SettingObserverManagerTest, SettingObserverOnChangeTest001, TestSize.Level3)
{
    std::string receivedKey;
    auto observer = OHOS::sptr<SettingObserver>(new SettingObserver("test_key",
        [&receivedKey](const std::string& key) { receivedKey = key; }));
    observer->OnChange();
    ASSERT_EQ(receivedKey, "test_key");
}

/**
 * @tc.name: SettingObserverManagerGetStringValueTest001
 * @tc.desc: test GetStringValue returns default value when DataShare unavailable
 * @tc.type: FUNC
 */
HWTEST_F(SettingObserverManagerTest, SettingObserverManagerGetStringValueTest001, TestSize.Level3)
{
    auto val = SettingObserverManager::GetInstance()->GetStringValue("nonexistent_key", "default_val");
    ASSERT_EQ(val, "default_val");

    val = SettingObserverManager::GetInstance()->GetStringValue("", "default_val");
    ASSERT_EQ(val, "default_val");
}

/**
 * @tc.name: SettingObserverManagerRegisterObserverTest001
 * @tc.desc: test RegisterObserver
 * @tc.type: FUNC
 */
HWTEST_F(SettingObserverManagerTest, SettingObserverManagerRegisterObserverTest001, TestSize.Level3)
{
    bool ret = SettingObserverManager::GetInstance()->RegisterObserver("test_register_key",
        [](const std::string&) {});
    ASSERT_TRUE(ret);

    ret = SettingObserverManager::GetInstance()->UnregisterObserver("test_register_key");
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: SettingObserverManagerSetStringValueTest001
 * @tc.desc: test SetStringValue fails
 * @tc.type: FUNC
 */
HWTEST_F(SettingObserverManagerTest, SettingObserverManagerSetStringValueTest001, TestSize.Level3)
{
    int ret = SettingObserverManager::GetInstance()->SetStringValue("test_set_key", "test_value");
    ASSERT_EQ(ret, FAILED_TO_OPT_DATA_SHARE);
}
