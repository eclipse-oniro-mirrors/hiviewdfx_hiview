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
#include "plugin_test.h"

#include "plugin.h"
#include "pipeline.h"

namespace OHOS {
namespace HiviewDFX {
void PluginTest::SetUpTestCase()
{
}

void PluginTest::TearDownTestCase()
{
}

void PluginTest::SetUp()
{
}

void PluginTest::TearDown()
{
}

/**
 * @tc.name: PluginTest001
 * @tc.desc: Test the api of Plugin.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PluginTest, PluginTest001, testing::ext::TestSize.Level0)
{
    /**
     * @tc.steps: step1. create Plugin object.
     * @tc.steps: step2. invoke the function of the plugin object.
     */
    printf("PluginTest001 start\n");
    Plugin plugin;
    ASSERT_FALSE(plugin.HasLoaded());

    plugin.OnLoad();
    auto event = plugin.GetEvent(Event::SYS_EVENT);
    ASSERT_NE(event, nullptr);
    ASSERT_TRUE(plugin.OnEvent(event));

    /* default null function test */
    // Dump
    plugin.Dump(0, {});
    // OnEventListeningCallback
    plugin.OnEventListeningCallback(*(event.get()));
    // OnConfigUpdate
    std::string localCfgPath = "test/local/config";
    std::string cloudCfgPath = "test/cloud/config";
    plugin.OnConfigUpdate(localCfgPath, cloudCfgPath);

    // delay to handle event
    auto pipelineEvent = std::make_shared<PipelineEvent>(*(event.get()));
    ASSERT_FALSE(pipelineEvent->hasPending_);
    auto workLoop = std::make_shared<EventLoop>("testLoop");
    plugin.BindWorkLoop(workLoop);
    ASSERT_NE(plugin.GetWorkLoop(), nullptr);
    plugin.DelayProcessEvent(pipelineEvent, 1); // 1s delay
    ASSERT_TRUE(pipelineEvent->hasPending_);
    sleep(3);

    // udpate the active time
    auto lastActiveTime1 = plugin.GetLastActiveTime();
    ASSERT_GT(lastActiveTime1, 0);
    plugin.UpdateTimeByDelay(1000); // delay 1s
    auto lastActiveTime2 = plugin.GetLastActiveTime();
    ASSERT_GT(lastActiveTime2, lastActiveTime1);

    // set/get function
    std::string version = "1.0";
    plugin.SetVersion(version);
    ASSERT_EQ(version, plugin.GetVersion());

    // bundle test
    ASSERT_FALSE(plugin.IsBundlePlugin());
    std::string bundleName = "testBundle";
    plugin.SetBundleName(bundleName);
    ASSERT_TRUE(plugin.IsBundlePlugin());

    ASSERT_EQ(plugin.GetUseCount(), 0);
    plugin.AddUseCount();
    ASSERT_EQ(plugin.GetUseCount(), 1);
    plugin.SubUseCount();
    ASSERT_EQ(plugin.GetUseCount(), 0);

    plugin.OnUnload();
    printf("PluginTest001 end\n");
}

/**
 * @tc.name: PluginTest002
 * @tc.desc: Test the api of Plugin.
 * @tc.type: FUNC
 */
HWTEST_F(PluginTest, PluginTest002, testing::ext::TestSize.Level0)
{
    /**
     * @tc.steps: step1. create Plugin object.
     * @tc.steps: step2. invoke the function of the plugin object.
     */
    printf("PluginTest002 start\n");
    auto plugin = std::make_shared<Plugin>();
    plugin->OnLoad();

    // OnEventProxy
    ASSERT_FALSE(plugin->OnEventProxy(nullptr));
    auto event = plugin->GetEvent(Event::SYS_EVENT);
    ASSERT_TRUE(plugin->OnEventProxy(event));

    // DelayProcessEvent
    plugin->DelayProcessEvent(nullptr, 0);
    plugin->DelayProcessEvent(event, 0);
    auto workLoop = std::make_shared<EventLoop>("testLoop");
    plugin->BindWorkLoop(workLoop);
    auto pipelineEvent = std::make_shared<PipelineEvent>(*(event.get()));
    plugin->DelayProcessEvent(nullptr, 0);
    plugin->DelayProcessEvent(event, 0);
    ASSERT_NE(plugin->GetLastActiveTime(), 0);

    // AddDispatchInfo
    plugin->AddDispatchInfo({});
    HiviewContext context;
    plugin->SetHiviewContext(&context);
    ASSERT_NE(plugin->GetHiviewContext(), nullptr);
    plugin->AddDispatchInfo({});

    plugin->OnUnload();
    printf("PluginTest002 end\n");
}

/**
 * @tc.name: PluginTest003
 * @tc.desc: Test the api of Plugin.
 * @tc.type: FUNC
 */
HWTEST_F(PluginTest, PluginTest003, testing::ext::TestSize.Level3)
{
    auto plugin = std::make_shared<Plugin>();
    plugin->SetName("test_plugin");
    ASSERT_EQ("test_plugin", plugin->GetName());
    ASSERT_EQ("test_plugin", plugin->GetHandlerInfo());
}

/**
 * @tc.name: HiviewContextTest001
 * @tc.desc: Test the api of HiviewContext.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PluginTest, HiviewContextTest001, testing::ext::TestSize.Level0)
{
    /**
     * @tc.steps: step1. create HiviewContext object.
     * @tc.steps: step2. invoke the function of the HiviewContext object.
     */
    printf("HiviewContextTest001 start\n");
    HiviewContext context;

    /* default null function test */
    // PostUnorderedEvent
    context.PostUnorderedEvent(nullptr, nullptr);
    // RegisterUnorderedEventListener
    std::weak_ptr<EventListener> eventListenerPtr;
    context.RegisterUnorderedEventListener(eventListenerPtr);
    // PostAsyncEventToTarget
    context.PostAsyncEventToTarget(nullptr, "", nullptr);
    // RequestUnloadPlugin
    context.RequestUnloadPlugin(nullptr);
    // AppendPluginToPipeline
    context.AppendPluginToPipeline("", "");
    // RequestLoadBundle
    context.RequestLoadBundle("");
    // RequestUnloadBundle
    context.RequestUnloadBundle("");
    // AddListenerInfo
    context.AddListenerInfo(0, "", {}, {});
    context.AddListenerInfo(0, "");
    // AddDispatchInfo
    std::weak_ptr<Plugin> pluginPtr;
    context.AddDispatchInfo(pluginPtr, {}, {}, {}, {});

    /* default null function test with return value */
    // PostSyncEventToTarget
    ASSERT_TRUE(context.PostSyncEventToTarget(nullptr, "", nullptr));
    // GetSharedWorkLoop
    ASSERT_EQ(context.GetSharedWorkLoop(), nullptr);
    // GetPipelineSequenceByName
    ASSERT_TRUE(context.GetPipelineSequenceByName("").empty());
    // IsReady
    ASSERT_FALSE(context.IsReady());
    // GetHiViewDirectory
    ASSERT_TRUE(context.GetHiViewDirectory(HiviewContext::DirectoryType::CONFIG_DIRECTORY).empty());
    ASSERT_TRUE(context.GetHiViewDirectory(HiviewContext::DirectoryType::WORK_DIRECTORY).empty());
    ASSERT_TRUE(context.GetHiViewDirectory(HiviewContext::DirectoryType::PERSIST_DIR).empty());
    // GetHiviewProperty
    ASSERT_EQ(context.GetHiviewProperty("test_key", "test_value"), "test_value");
    // SetHiviewProperty
    ASSERT_TRUE(context.SetHiviewProperty("test_key", "test_value", true));
    // InstancePluginByProxy
    ASSERT_EQ(context.InstancePluginByProxy(nullptr), nullptr);
    // GetPluginByName
    ASSERT_EQ(context.GetPluginByName(""), nullptr);
    // GetListenerInfo
    ASSERT_TRUE(context.GetListenerInfo(0, "", "").empty());
    // GetDisPatcherInfo
    ASSERT_TRUE(context.GetDisPatcherInfo(0, "", "", "").empty());

    printf("HiviewContextTest001 end\n");
}
} // namespace HiviewDFX
} // namespace OHOS
