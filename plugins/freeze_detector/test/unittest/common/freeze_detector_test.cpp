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
#include "freeze_detector_test.h"

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
std::shared_ptr<SysEvent> makeEvent(const std::string& name,
                                    const std::string& domain,
                                    const std::string& eventName,
                                    const std::string& packageName,
                                    const std::string& logPath)
{
    auto time = TimeUtil::GetMilliseconds();
    std::string str = "******************\n";
    str += "this is test " + eventName + " at " + std::to_string(time) + "\n";
    str += "******************\n";
    FileUtil::SaveStringToFile(logPath, str);

    auto jsonStr = "{\"domain_\":\"" + domain + "\"}";
    auto sysEvent = std::make_shared<SysEvent>(name, nullptr, jsonStr);
    sysEvent->SetEventValue("name_", eventName);
    sysEvent->SetEventValue("type_", 1);
    sysEvent->SetEventValue("time_", time);
    sysEvent->SetEventValue("pid_", getpid());
    sysEvent->SetEventValue("tid_", gettid());
    sysEvent->SetEventValue("uid_", getuid());
    sysEvent->SetEventValue("tz_", TimeUtil::GetTimeZone());
    sysEvent->SetEventValue("PID", getpid());
    sysEvent->SetEventValue("UID", getuid());
    sysEvent->SetEventValue("MSG", "test " + eventName + " event");
    sysEvent->SetEventValue("PACKAGE_NAME", packageName);
    sysEvent->SetEventValue("PROCESS_NAME", packageName);

    std::string tmpStr = R"~(logPath:)~" + logPath;
    sysEvent->SetEventValue("info_", tmpStr);
    return sysEvent;
}

bool GetFreezeDectorTestFile(const std::string& domain, const std::string& eventName, const std::string& packageName,
    uint64_t time)
{
    auto freezeCommon = std::make_shared<FreezeCommon>();
    if (!freezeCommon->Init()) {
        printf("failed to parse rule file.");
        return false;
    }
    std::string type = freezeCommon->IsApplicationEvent(domain, eventName) ? "appfreeze" :
        (freezeCommon->IsSystemEvent(domain, eventName) ? "sysfreeze" : "syswarning");

    int count = 0;
    std::string decLogPath = "";
    while (count < 10) { // 10: 最大等待10s
        sleep(1);
        std::vector<std::string> files;
        FileUtil::GetDirFiles("/data/log/faultlog/freeze/", files);
        ++count;
        for (auto& i : files) {
            if (i.find(packageName) == std::string::npos || i.find(type) == std::string::npos) {
                printf("File name not match, packageName: %s, type: %s\n", packageName.c_str(), type.c_str());
                continue;
            }

            std::string content;
            FileUtil::LoadStringFromFile(i, content);
            if (content.find(packageName) == std::string::npos || content.find(eventName) == std::string::npos ||
                content.find(domain) == std::string::npos || content.find(std::to_string(time)) == std::string::npos) {
                printf("File content not match, packageName: %s, eventName: %s, domain: %s, time: %s\n",
                    packageName.c_str(), eventName.c_str(), domain.c_str(), std::to_string(time).c_str());
                FileUtil::RemoveFile(i);
                continue;
            }
            decLogPath = i;
            break;
        }

        if (decLogPath != "") {
            break;
        }
    }

    if (decLogPath == "") {
        printf("Not find files.\n");
        return false;
    }

    FileUtil::RemoveFile(decLogPath);
    return true;
}
}

void FreezeDetectorTest::SetUp()
{
    /**
     * @tc.setup: create work directories
     */
    printf("SetUp.\n");
}
void FreezeDetectorTest::SetUpTestCase()
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

void FreezeDetectorTest::TearDownTestCase()
{
    /**
     * @tc.setup: all end
     */
    printf("TearDownTestCase.\n");
}

void FreezeDetectorTest::TearDown()
{
    /**
     * @tc.teardown: destroy the event loop we have created
     */
    printf("TearDown.\n");
}

/**
 * @tc.name: FreezeDetectorProxyTest001
 * @tc.desc: FreezeDetector proxy
 * @tc.type: FUNC
 * @tc.require: AR000H3T5D
 */
HWTEST_F(FreezeDetectorTest, FreezeDetectorProxyTest001, TestSize.Level3)
{
    HiviewPlatform &platform = HiviewPlatform::GetInstance();
    std::shared_ptr<Plugin> plugin = platform.GetPluginByName("FreezeDetectorPlugin");
    if (plugin == nullptr) {
        printf("Get FreezeDetectorPlugin, failed\n");
        FAIL();
    }

    std::shared_ptr<PluginProxy> pluginProxy = std::static_pointer_cast<PluginProxy>(plugin);

    if (pluginProxy->HoldInstance()) {
        printf("FreezeDetectorPlugin, HoldInstance is true\n");
        FAIL();
    }

    auto jsonStr = "{\"domain_\":\"TEST_INPUT\"}";
    auto sysEvent = std::make_shared<SysEvent>("FreezeDetectorProxyTest001", nullptr, jsonStr);
    sysEvent->SetEventValue("name_", "TEST_INPUT");
    sysEvent->SetEventValue("type_", 1);
    sysEvent->SetEventValue("time_", TimeUtil::GetMilliseconds());
    sysEvent->SetEventValue("pid_", getpid());
    sysEvent->SetEventValue("tid_", gettid());
    sysEvent->SetEventValue("uid_", getuid());
    sysEvent->SetEventValue("tz_", TimeUtil::GetTimeZone());
    sysEvent->SetEventValue("PID", getpid());
    sysEvent->SetEventValue("UID", getuid());
    sysEvent->SetEventValue("MSG", "test TEST_INPUT event");
    sysEvent->SetEventValue("PACKAGE_NAME", "FreezeDetectorProxyTest001");
    sysEvent->SetEventValue("PROCESS_NAME", "FreezeDetectorProxyTest001");
    sysEvent->SetEventValue("info_", "");

    std::shared_ptr<OHOS::HiviewDFX::Event> event = std::static_pointer_cast<Event>(sysEvent);
    plugin->OnEventListeningCallback(*(event.get()));

    sleep(1);
    if (!pluginProxy->HoldInstance()) {
        printf("FreezeDetectorPlugin, HoldInstance is false\n");
        FAIL();
    }
}

/**
 * @tc.name: FreezeDetectorTest001
 * @tc.desc: FreezeDetector send APPLICATION_BLOCK_INPUT
 * @tc.type: FUNC
 * @tc.require: AR000H3T5D
 */
HWTEST_F(FreezeDetectorTest, FreezeDetectorTest001, TestSize.Level0)
{
    HiviewPlatform &platform = HiviewPlatform::GetInstance();
    std::shared_ptr<Plugin> plugin = platform.GetPluginByName("FreezeDetectorPlugin");
    if (plugin == nullptr) {
        printf("Get FreezeDetectorPlugin, failed\n");
        FAIL();
    }
    ASSERT_TRUE(plugin != nullptr);

    /*
        {"domain_":"MULTIMODALINPUT","name_":"APPLICATION_BLOCK_INPUT","type_":1,"time_":1504016751820,
            "tz_":"+0000","pid_":253,"tid_":1220,
            "uid_":6696,"PID":1886,"UID":20010031,"PACKAGE_NAME":"com.ohos.example.openapi9",
            "PROCESS_NAME":"com.ohos.example.openapi9",
            "MSG":"User input does not respond","level_":"CRITICAL","id_":"53545984916101040510",
            "info_":"isResolved,eventId:0,logPath:/data/log/eventlog/APPLICATION_BLOCK_INPUT-1886-20170829142551.log"}
    */
    std::string logPath = "/data/test/test_data/LOG001.log";
    FileUtil::CreateFile(logPath);
    bool fileExist = FileUtil::FileExists(logPath);
    if (!fileExist) {
        printf("CreateFile file, failed\n");
        FAIL();
    }
    ASSERT_TRUE(fileExist);

    auto sysEvent = makeEvent("FreezeDectorTest001", "AAFWK", "APP_INPUT_BLOCK", "FreezeDectorTest", logPath);
    if (sysEvent == nullptr) {
        printf("GetFreezeDectorTest001File, failed\n");
        FAIL();
    }
    ASSERT_TRUE(sysEvent != nullptr);

    uint64_t time = sysEvent->GetEventIntValue("time_");
    std::shared_ptr<OHOS::HiviewDFX::Event> event = std::static_pointer_cast<Event>(sysEvent);
    event->eventName_ = sysEvent->GetEventValue("name_");
    plugin->OnEventListeningCallback(*(event.get()));

    sleep(10);
    bool getFreezeTestFile = GetFreezeDectorTestFile("AAFWK", "APP_INPUT_BLOCK", "FreezeDectorTest", time);
    printf("GetFreezeDectorTest002File result: %d\n", getFreezeTestFile);
}

/**
 * @tc.name: FreezeDetectorTestTest002
 * @tc.desc: FreezeDetector send LIFECYCLE_TIMEOUT
 * @tc.type: FUNC
 * @tc.require: AR000H3T5D
 */
HWTEST_F(FreezeDetectorTest, FreezeDetectorTest002, TestSize.Level3)
{
    HiviewPlatform &platform = HiviewPlatform::GetInstance();
    std::shared_ptr<Plugin> plugin = platform.GetPluginByName("FreezeDetectorPlugin");
    if (plugin == nullptr) {
        printf("Get FreezeDetectorPlugin, failed");
        FAIL();
    }
    ASSERT_TRUE(plugin != nullptr);

    /*
        {"domain_":"AAFWK","name_":"LIFECYCLE_TIMEOUT","type_":1,"time_":1504095513772,"tz_":"+0000",
            "pid_":444,"tid_":1290,
            "uid_":5523,"UID":20010031,"PID":2070,"PACKAGE_NAME":"com.ohos.example.openapi9",
            "PROCESS_NAME":"com.ohos.example.openapi9",
            "MSG":"ability:MainAbility background timeout","level_":"CRITICAL",
            "tag_":"STABILITY","id_":"13720438474024340534",
            "info_":"logPath:/data/log/eventlog/LIFECYCLE_TIMEOUT-2070-20170830121833.log,"}
    */
    std::string logPath = "/data/test/test_data/LOG002.log";
    FileUtil::CreateFile(logPath);
    bool fileExist = FileUtil::FileExists(logPath);
    if (!fileExist) {
        printf("CreateFile file, failed\n");
        FAIL();
    }
    ASSERT_TRUE(fileExist);

    auto sysEvent = makeEvent("FreezeDectorTest002", "GRAPHIC", "NO_DRAW", "FreezeDectorTest", logPath);
    if (sysEvent == nullptr) {
        printf("GetFreezeDectorTest002File, failed\n");
        FAIL();
    }
    ASSERT_TRUE(sysEvent != nullptr);

    uint64_t time = sysEvent->GetEventIntValue("time_");
    std::shared_ptr<OHOS::HiviewDFX::Event> event = std::static_pointer_cast<Event>(sysEvent);
    event->eventName_ = sysEvent->GetEventValue("name_");
    plugin->OnEventListeningCallback(*(event.get()));

    sleep(10);
    bool getFreezeTestFile = GetFreezeDectorTestFile("GRAPHIC", "NO_DRAW", "FreezeDectorTest", time);
    printf("GetFreezeDectorTest002File result: %d\n", getFreezeTestFile);
}

/**
 * @tc.name: FreezeDetectorTest003
 * @tc.desc: FreezeDetector send LIFECYCLE_TIMEOUT
 * @tc.type: FUNC
 * @tc.require: AR000H3T5D
 */
HWTEST_F(FreezeDetectorTest, FreezeDetectorTest003, TestSize.Level3)
{
    HiviewPlatform &platform = HiviewPlatform::GetInstance();
    std::shared_ptr<Plugin> plugin = platform.GetPluginByName("FreezeDetectorPlugin");
    if (plugin == nullptr) {
        printf("Get FreezeDetectorPlugin, failed");
        FAIL();
    }
    ASSERT_TRUE(plugin != nullptr);

    std::string logPath = "/data/test/test_data/LOG003_1.log";
    FileUtil::CreateFile(logPath);
    bool fileExist = FileUtil::FileExists(logPath);
    if (!fileExist) {
        printf("CreateFile file, failed\n");
        FAIL();
    }
    ASSERT_TRUE(fileExist);

    auto sysEvent = makeEvent("FreezeDectorTest003", "ACE", "UI_BLOCK_6S", "FreezeDectorTest", logPath);
    std::shared_ptr<OHOS::HiviewDFX::Event> event = std::static_pointer_cast<Event>(sysEvent);
    plugin->OnEventListeningCallback(*(event.get()));
    sleep(10);
    ASSERT_EQ(plugin->GetName(), "FreezeDetectorPlugin");
}

/**
 * @tc.name: FreezeDetectorTest003
 * @tc.desc: FreezeDetector send LIFECYCLE_TIMEOUT
 * @tc.type: FUNC
 * @tc.require: AR000H3T5D
 */
HWTEST_F(FreezeDetectorTest, FreezeDetectorTest004, TestSize.Level3)
{
    HiviewPlatform &platform = HiviewPlatform::GetInstance();
    std::shared_ptr<Plugin> plugin = platform.GetPluginByName("FreezeDetectorPlugin");
    if (plugin == nullptr) {
        printf("Get FreezeDetectorPlugin, failed");
        FAIL();
    }
    ASSERT_TRUE(plugin != nullptr);

    /*
        {"domain_":"AAFWK","name_":"LIFECYCLE_TIMEOUT","type_":1,"time_":1504095513772,"tz_":"+0000",
            "pid_":444,"tid_":1290,
            "uid_":5523,"UID":20010031,"PID":2070,"PACKAGE_NAME":"com.ohos.example.openapi9",
            "PROCESS_NAME":"com.ohos.example.openapi9",
            "MSG":"ability:MainAbility background timeout","level_":"CRITICAL",
            "tag_":"STABILITY","id_":"13720438474024340534",
            "info_":"logPath:/data/log/eventlog/LIFECYCLE_TIMEOUT-2070-20170830121833.log,"}
    */
    std::string logPath = "/data/test/test_data/LOG004.log";
    FileUtil::CreateFile(logPath);
    bool fileExist = FileUtil::FileExists(logPath);
    if (!fileExist) {
        printf("CreateFile file, failed\n");
        FAIL();
    }
    ASSERT_TRUE(fileExist);

    auto sysEvent = makeEvent("FreezeDectorTest004", "AAFWK", "UI_BLOCK_3S", "FreezeDectorTest", logPath);
    std::shared_ptr<OHOS::HiviewDFX::Event> event = std::static_pointer_cast<Event>(sysEvent);
    plugin->OnEventListeningCallback(*(event.get()));

    sleep(10);
    ASSERT_EQ(plugin->GetName(), "FreezeDetectorPlugin");
}

/**
 * @tc.name: FreezeRuleTest001
 * @tc.desc: FreezeRule
 * @tc.type: FUNC
 * @tc.require: AR000H3T5D
 */
HWTEST_F(FreezeDetectorTest, FreezeRuleTest001, TestSize.Level0)
{
    auto freezeRuleCluster = std::make_shared<FreezeRuleCluster>();
    bool freezeRuleFlag = freezeRuleCluster->ParseRuleFile("/data/test/test_data/freeze_rules.xml");
    ASSERT_TRUE(freezeRuleFlag);

    std::map<std::string, std::pair<std::string, bool>> appPairs = freezeRuleCluster->GetApplicationPairs();
    if (appPairs.find("THREAD_BLOCK_6S") != appPairs.end()) {
        auto tmp = appPairs["THREAD_BLOCK_6S"];
        if (tmp.first != "AAFWK") {
            printf("THREAD_BLOCK_6S tmp.first != AAFWK.");
            FAIL();
        }
        ASSERT_EQ(tmp.first, "AAFWK");
        if (!tmp.second) {
            printf("THREAD_BLOCK_6S tmp.second == false.");
            FAIL();
        }
        ASSERT_TRUE(tmp.second);
    } else {
        printf("THREAD_BLOCK_6S not find.");
        FAIL();
    }

    if (appPairs.find("UI_BLOCK_3S") != appPairs.end()) {
        auto tmp = appPairs["UI_BLOCK_3S"];
        if (tmp.first != "ACE") {
            printf("UI_BLOCK_3S tmp.first != AAFWK.");
            FAIL();
        }
        ASSERT_EQ(tmp.first, "ACE");
        if (tmp.second) {
            printf("UI_BLOCK_3S tmp.second == true.");
            FAIL();
        }
        ASSERT_TRUE(!tmp.second);
    } else {
        printf("UI_BLOCK_3S not find.");
        FAIL();
    }
}

/**
 * @tc.name: FreezeRuleTest002
 * @tc.desc: FreezeRule
 * @tc.type: FUNC
 * @tc.require: AR000H3T5D
 */
HWTEST_F(FreezeDetectorTest, FreezeRuleTest002, TestSize.Level3)
{
    auto freezeRuleCluster = std::make_shared<FreezeRuleCluster>();
    bool freezeRuleFlag = freezeRuleCluster->ParseRuleFile("/data/test/test_data/freeze_rules.xml");
    ASSERT_TRUE(freezeRuleFlag);

    std::map<std::string, std::pair<std::string, bool>> systemPairs = freezeRuleCluster->GetSystemPairs();
    if (systemPairs.find("SCREEN_ON") != systemPairs.end()) {
        auto tmp = systemPairs["SCREEN_ON"];
        if (tmp.first != "KERNEL_VENDOR") {
            printf("SCREEN_ON tmp.first != AAFWK.");
            FAIL();
        }
        ASSERT_EQ(tmp.first, "KERNEL_VENDOR");
        if (!tmp.second) {
            printf("SCREEN_ON tmp.second == false.");
            FAIL();
        }
        ASSERT_TRUE(tmp.second);
    } else {
        printf("SCREEN_ON not find.");
        FAIL();
    }

    if (systemPairs.find("HUNGTASK") != systemPairs.end()) {
        auto tmp = systemPairs["HUNGTASK"];
        if (tmp.first != "KERNEL_VENDOR") {
            printf("HUNGTASK tmp.first != AAFWK.");
            FAIL();
        }
        ASSERT_EQ(tmp.first, "KERNEL_VENDOR");
        if (!tmp.second) {
            printf("HUNGTASK tmp.second == false.");
            FAIL();
        }
        ASSERT_TRUE(tmp.second);
    } else {
        printf("HUNGTASK not find.");
        FAIL();
    }
}

/**
 * @tc.name: FreezeCommonTest001
 * @tc.desc: FreezeCommon
 * @tc.type: FUNC
 * @tc.require: AR000H3T5D
 */
HWTEST_F(FreezeDetectorTest, FreezeCommonTest001, TestSize.Level3)
{
    auto freezeCommon = std::make_shared<FreezeCommon>();
    bool initResult = freezeCommon->Init();
    if (!initResult) {
        printf("failed to parse rule file.");
        ASSERT_TRUE(initResult);
    }

    bool isFreezeEvent = freezeCommon->IsFreezeEvent("AAFWK", "LIFECYCLE_TIMEOUT");
    if (!isFreezeEvent) {
        printf("IsFreezeEvent \"AAFWK\", \"LIFECYCLE_TIMEOUT\" not find.");
        FAIL();
    }
    ASSERT_TRUE(isFreezeEvent);

    isFreezeEvent = freezeCommon->IsFreezeEvent("KERNEL_VENDOR", "SCREEN_ON");
    if (!isFreezeEvent) {
        printf("IsFreezeEvent \"KERNEL_VENDOR\", \"SCREEN_ON\" not find.");
        FAIL();
    }
    ASSERT_TRUE(isFreezeEvent);

    isFreezeEvent = freezeCommon->IsFreezeEvent("MULTIMODALINPUT", "NO_DRAW");
    if (isFreezeEvent) {
        printf("IsFreezeEvent, \"NO_DRAW\" is error.");
        FAIL();
    }
    ASSERT_TRUE(!isFreezeEvent);

    bool isApplicationEvent = freezeCommon->IsApplicationEvent("AAFWK", "THREAD_BLOCK_3S");
    if (!isApplicationEvent) {
        printf("\"AAFWK\", \"THREAD_BLOCK_3S\" not ApplicationEvent.");
        FAIL();
    }
    ASSERT_TRUE(isApplicationEvent);

    isApplicationEvent = freezeCommon->IsApplicationEvent("KERNEL_VENDOR", "HUNGTASK");
    if (isApplicationEvent) {
        printf("\"KERNEL_VENDOR\", \"HUNGTASK\" is error.");
        FAIL();
    }
    ASSERT_TRUE(!isApplicationEvent);

    bool isSystemEvent = freezeCommon->IsSystemEvent("AAFWK", "THREAD_BLOCK_3S");
    if (isSystemEvent) {
        printf("\"AAFWK\", \"THREAD_BLOCK_3S\" is error.");
        FAIL();
    }
    ASSERT_TRUE(!isSystemEvent);

    isSystemEvent = freezeCommon->IsSystemEvent("KERNEL_VENDOR", "HUNGTASK");
    if (!isSystemEvent) {
        printf("\"KERNEL_VENDOR\", \"HUNGTASK\" not SystemEvent.");
        FAIL();
    }
    ASSERT_TRUE(isSystemEvent);
}
}
}
