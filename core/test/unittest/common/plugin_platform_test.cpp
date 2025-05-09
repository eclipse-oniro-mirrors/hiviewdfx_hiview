/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
#include "plugin_platform_test.h"

#include <fstream>
#include <iostream>
#include <set>
#include <unistd.h>

#include "event.h"
#include "file_util.h"

#include "platform_test_result_listener.h"
using namespace testing::ext;
using namespace OHOS::HiviewDFX;

void PluginPlatformTest::SetUp()
{
    /**
     * @tc.setup: create work directories
     */
    printf("SetUp.\n");
    FileUtil::ForceCreateDirectory("/data/test/faultlog");
    platform.GetPluginMap();
}

/**
 * @tc.name: PluginPlatformPluginPendLoadTest001
 * @tc.desc: test pend loading plugin feature
 * @tc.type: FUNC
 * @tc.require: AR000DPTT4
 */
HWTEST_F(PluginPlatformTest, PluginPlatformPluginPendLoadTest001, TestSize.Level3)
{
    /**
     * @tc.steps: step1. init plugin platform
     */
    printf("PluginPlatformTest2.\n");
    OHOS::HiviewDFX::HiviewPlatform platform;
    if (!platform.InitEnvironment("/data/test/test_data/hiview_platform_config1")) {
        printf("Fail to init environment. \n");
    }

    sleep(1);
    /**
     * @tc.steps: step2. check current loaded plugin number
     */
    ASSERT_EQ(true, platform.IsReady());
    auto& pluginList = platform.GetPluginMap();
    auto size = pluginList.size();
    ASSERT_EQ(size, 4ul);

    sleep(5);
    /**
     * @tc.steps: step3. check final loaded plugin number
     */
    auto& pluginList2 = platform.GetPluginMap();
    auto size2 = pluginList2.size();
    ASSERT_EQ(size2, 5ul);
}


/**
 * @tc.name: PluginPlatformServiceStartTest001
 * @tc.desc: start fault detect service
 * @tc.type: FUNC
 * @tc.require: AR000DPTT4
 */
HWTEST_F(PluginPlatformTest, PluginPlatformServiceStartTest001, TestSize.Level3)
{
    /**
     * @tc.steps: step1. init plugin platform
     */
    printf("PluginPlatformTest2.\n");
    OHOS::HiviewDFX::HiviewPlatform platform;
    printf("PluginPlatformServiceStartTest001. called\n");
    if (!platform.InitEnvironment("/data/test/test_data/hiview_platform_config")) {
        printf("Fail to init environment. \n");
    }
    sleep(1);
    printf("PluginPlatformServiceStartTest001. end\n");
    ASSERT_EQ(true, platform.IsReady());
}

#ifndef TEST_LOCAL_SRC
/**
 * @tc.name: PluginPlatformDynamicPluginUnloadTest001
 * @tc.desc: test pend loading plugin feature
 * @tc.type: FUNC
 * @tc.require: AR000DPTT4
 */
HWTEST_F(PluginPlatformTest, PluginPlatformDynamicPluginUnloadTest001, TestSize.Level3)
{
    /**
     * @tc.steps: step1. init plugin platform
     */
    printf("PluginPlatformTest2.\n");
    OHOS::HiviewDFX::HiviewPlatform platform;
    ASSERT_TRUE(platform.InitEnvironment("/data/test/test_data/hiview_platform_config"));
    ASSERT_EQ(access("/system/lib64/libdynamicloadpluginexample.z.so", F_OK), 0);

    sleep(1);
    ASSERT_EQ(true, platform.IsReady());
    auto& pluginList = platform.GetPluginMap();
    auto size = pluginList.size();
    ASSERT_EQ(size, 6ul);

    std::shared_ptr<Plugin> plugin = nullptr;
    auto pos = pluginList.find("EventProcessorExample1");
    if (pos == pluginList.end()) {
        FAIL();
    } else {
        plugin = pos->second;
    }
    ASSERT_NE(plugin, nullptr);

    auto event = plugin->GetEvent(Event::MessageType::FAULT_EVENT);
    event->eventId_ = 901000002;
    bool ret = platform.PostSyncEventToTarget(plugin, "DynamicLoadPluginExample", event);
    ASSERT_EQ(ret, true);
    auto str = event->GetValue("DynamicLoadPluginExample");
    printf("event %p  str:%s \n", event.get(), str.c_str());
    ASSERT_EQ(str, "Done");
    auto unloadEvent = plugin->GetEvent(Event::MessageType::PLUGIN_MAINTENANCE);
    unloadEvent->SetValue("DynamicLoadPluginExample", "Unload");
    platform.PostUnorderedEvent(plugin, unloadEvent);
    sleep(3);
    size = pluginList.size();
    ASSERT_EQ(size, 5ul);
}
#endif
