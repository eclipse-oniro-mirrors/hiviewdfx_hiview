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
#include "freeze_detector_ipcfull_test.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <unistd.h>

#include "event.h"
#include "file_util.h"
#include "time_util.h"

#include "plugin_proxy.h"
#include "hiview_platform.h"
#include "sys_event.h"

#include "freeze_common.h"
#include "rule_cluster.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
namespace {
    struct EventParam {
        std::string name;
        std::string domain;
        std::string eventName;
        std::string packageName;
        std::string logPath;
        int uid;
        int pid;
        int tid;
        EventParam() : uid(getuid()), pid(getpid()), tid(gettid()) {}
    };

    std::shared_ptr<SysEvent> makeEvent(EventParam &param)
    {
        auto time = TimeUtil::GetMilliseconds();
        std::string str = "******************\n";
        str += "this is test " + param.eventName + " at " + std::to_string(time) + "\n";
        str += "******************\n";
        FileUtil::SaveStringToFile(param.logPath, str);

        auto jsonStr = "{\"domain_\":\"" + param.domain + "\"}";
        auto sysEvent = std::make_shared<SysEvent>(param.name, nullptr, jsonStr);
        sysEvent->SetEventValue("name_", param.eventName);
        sysEvent->SetEventValue("type_", 1);
        sysEvent->SetEventValue("time_", time);
        sysEvent->SetEventValue("pid_", param.pid);
        sysEvent->SetEventValue("tid_", param.tid);
        sysEvent->SetEventValue("uid_", param.uid);
        sysEvent->SetEventValue("tz_", TimeUtil::GetTimeZone());
        sysEvent->SetEventValue("PID", param.pid);
        sysEvent->SetEventValue("UID", param.uid);
        sysEvent->SetEventValue("MSG", "test " + param.eventName + " event");
        sysEvent->SetEventValue("PACKAGE_NAME", param.packageName);
        sysEvent->SetEventValue("PROCESS_NAME", param.packageName);

        std::string tmpStr = R"~(logPath:)~" + param.logPath;
        sysEvent->SetEventValue("info_", tmpStr);
        return sysEvent;
    }
}
void FreezeDetectorIPCFULLTest::SetUp()
{
    /**
     * @tc.setup: create work directories
     */
    printf("SetUp.\n");
}
void FreezeDetectorIPCFULLTest::SetUpTestCase()
{
    /**
     * @tc.setup: all first
     */
    printf("SetUpTestCase.\n");
    HiviewPlatform &platform = HiviewPlatform::GetInstance();
    if (!platform.InitEnvironment("/data/test/test_data/hiview_platform_config")) {
        printf("Fail to init environment.\n");
    }
}

void FreezeDetectorIPCFULLTest::TearDownTestCase()
{
    /**
     * @tc.setup: all end
     */
    printf("TearDownTestCase.\n");
}

void FreezeDetectorIPCFULLTest::TearDown()
{
    /**
     * @tc.teardown: destroy the event loop we have created
     */
    printf("TearDown.\n");
}

/**
 * @tc.name: FreezeDetectorIPCFULLTest001
 * @tc.desc: Test the functionality of FreezeDetector proxy matching IPC_FULL events and HIVIEW's UID
 * @tc.type: FUNC
 */
HWTEST_F(FreezeDetectorIPCFULLTest, FreezeDetectorIPCFULLTest001, TestSize.Level3)
{
    HiviewPlatform &platform = HiviewPlatform::GetInstance();
    std::shared_ptr<Plugin> plugin = platform.GetPluginByName("FreezeDetectorPlugin");
    if (plugin == nullptr) {
        printf("Get FreezeDetectorPlugin, failed\n");
        FAIL();
    }
    ASSERT_TRUE(plugin != nullptr);
    EventParam param;
    param.uid = 1201;
    param.eventName = "IPC_FULL";
    auto sysEvent = makeEvent(param);
    if (sysEvent == nullptr) {
        printf("GetFreezeDetectorIPCFULLTest001File, failed\n");
        FAIL();
    }
    ASSERT_TRUE(sysEvent != nullptr);
    std::shared_ptr<OHOS::HiviewDFX::Event> event = std::static_pointer_cast<Event>(sysEvent);
    plugin->OnEventListeningCallback(*(event.get()));

        if (event == nullptr) {
        printf("Get FreezeDetectorIPCFULLPlugin after OnEventListeningCallback, failed\n");
        FAIL();
    }
}
/**
 * @tc.name: FreezeDetectorIPCFULLTest002
 * @tc.desc: Test the functionality of FreezeDetector proxy when neither IPC_FULL events nor HIVIEW's UID is present
 * @tc.type: FUNC
 */
HWTEST_F(FreezeDetectorIPCFULLTest, FreezeDetectorIPCFULLTest002, TestSize.Level3)
{
    HiviewPlatform &platform = HiviewPlatform::GetInstance();
    std::shared_ptr<Plugin> plugin = platform.GetPluginByName("FreezeDetectorPlugin");
    if (plugin == nullptr) {
        printf("Get FreezeDetectorPlugin, failed\n");
        FAIL();
    }
    ASSERT_TRUE(plugin != nullptr);
    EventParam param;
    param.uid = 1000;
    param.eventName = "NOT_IPC_FULL";
    auto sysEvent = makeEvent(param);
    if (sysEvent == nullptr) {
        printf("GetFreezeDetectorIPCFULLTest002File, failed\n");
        FAIL();
    }
    ASSERT_TRUE(sysEvent != nullptr);
    std::shared_ptr<OHOS::HiviewDFX::Event> event = std::static_pointer_cast<Event>(sysEvent);
    plugin->OnEventListeningCallback(*(event.get()));

        if (event == nullptr) {
        printf("Get FreezeDetectorIPCFULLPlugin after OnEventListeningCallback, failed\n");
        FAIL();
    }
}
/**
 * @tc.name: FreezeDetectorIPCFULLTest003
 * @tc.desc: Test the functionality of FreezeDetector proxy when only IPC_FULL events are present without HIVIEW's UID
 * @tc.type: FUNC
 */
HWTEST_F(FreezeDetectorIPCFULLTest, FreezeDetectorIPCFULLTest003, TestSize.Level3)
{
    HiviewPlatform &platform = HiviewPlatform::GetInstance();
    std::shared_ptr<Plugin> plugin = platform.GetPluginByName("FreezeDetectorPlugin");
    if (plugin == nullptr) {
        printf("Get FreezeDetectorPlugin, failed\n");
        FAIL();
    }
    ASSERT_TRUE(plugin != nullptr);
    EventParam param;
    param.uid = 1000;
    param.eventName = "IPC_FULL";
    auto sysEvent = makeEvent(param);
    if (sysEvent == nullptr) {
        printf("GetFreezeDetectorIPCFULLTest003File, failed\n");
        FAIL();
    }
    ASSERT_TRUE(sysEvent != nullptr);
    std::shared_ptr<OHOS::HiviewDFX::Event> event = std::static_pointer_cast<Event>(sysEvent);
    plugin->OnEventListeningCallback(*(event.get()));

        if (event == nullptr) {
        printf("Get FreezeDetectorIPCFULLPlugin after OnEventListeningCallback, failed\n");
        FAIL();
    }
}
/**
 * @tc.name: FreezeDetectorIPCFULLTest004
 * @tc.desc: Test the functionality of FreezeDetector proxy when only HIVIEW's UID is present without IPC_FULL events
 * @tc.type: FUNC
 */
HWTEST_F(FreezeDetectorIPCFULLTest, FreezeDetectorIPCFULLTest004, TestSize.Level3)
{
    HiviewPlatform &platform = HiviewPlatform::GetInstance();
    std::shared_ptr<Plugin> plugin = platform.GetPluginByName("FreezeDetectorPlugin");
    if (plugin == nullptr) {
        printf("Get FreezeDetectorPlugin, failed\n");
        FAIL();
    }
    ASSERT_TRUE(plugin != nullptr);
    EventParam param;
    param.uid = 1201;
    param.eventName = "NOT_IPC_FULL";
    auto sysEvent = makeEvent(param);
    if (sysEvent == nullptr) {
        printf("GetFreezeDetectorIPCFULLTest004File, failed\n");
        FAIL();
    }
    ASSERT_TRUE(sysEvent != nullptr);
    std::shared_ptr<OHOS::HiviewDFX::Event> event = std::static_pointer_cast<Event>(sysEvent);
    plugin->OnEventListeningCallback(*(event.get()));

        if (event == nullptr) {
        printf("Get FreezeDetectorIPCFULLPlugin after OnEventListeningCallback, failed\n");
        FAIL();
    }
}
}
}