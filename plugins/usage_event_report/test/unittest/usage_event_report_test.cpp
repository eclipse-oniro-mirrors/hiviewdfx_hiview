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
#include <gtest/gtest.h>

#include "hiview_shutdown_callback.h"
#include "usage_event_report.h"

using namespace testing::ext;

namespace OHOS {
namespace HiviewDFX {
class TestHiviewContext : public HiviewContext {
public:
    std::string GetHiViewDirectory(DirectoryType type) override
    {
        return "/data/test/hiview/usage_event_report";
    }
};

class UsageEventReportTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: UsageEventReportTest001
 * @tc.desc: check the plugin loading function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(UsageEventReportTest, UsageEventReportTest001, TestSize.Level1)
{
    UsageEventReport plugin;
    TestHiviewContext context;
    plugin.SetHiviewContext(&context);
    // load plugin
    plugin.OnLoad();
    EXPECT_TRUE(plugin.IsRunning());

    // start task
    plugin.TimeOut();
    EXPECT_TRUE(plugin.IsRunning());

    // unload plugin
    plugin.OnUnload();
    EXPECT_FALSE(plugin.IsRunning());
}

/**
 * @tc.name: UsageEventReportTest002
 * @tc.desc: check functions to process events.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(UsageEventReportTest, UsageEventReportTest002, TestSize.Level1)
{
    UsageEventReport plugin;
    // load plugin
    plugin.OnLoad();
    EXPECT_TRUE(plugin.IsRunning());

    std::shared_ptr<Event> event = nullptr;
    plugin.OnEvent(event);
    event = plugin.GetEvent(Event::SYS_EVENT);
    ASSERT_NE(event, nullptr);
    ASSERT_TRUE(plugin.OnEvent(event));

    plugin.SaveEventToDb();
    auto callback = new (std::nothrow) HiViewShutdownCallback();
    callback->OnAsyncShutdown();
    // unload plugin
    plugin.OnUnload();
    EXPECT_FALSE(plugin.IsRunning());
}
} // namespace HiviewDFX
} // namespace OHOS
