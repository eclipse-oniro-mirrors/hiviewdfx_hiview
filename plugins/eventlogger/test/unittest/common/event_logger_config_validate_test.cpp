/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "event_field_validator.h"
#include "sys_event.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;

namespace {
constexpr int32_t FOUNDATION_UID = 5523;
constexpr int32_t GRAPHICS_UID = 1003;
constexpr int32_t HIVIEW_UID = 1201;
constexpr int32_t ROOT_UID = 0;
constexpr int32_t POWERMGR_UID = 5528;
constexpr int32_t INPUT_UID = 6696;
constexpr int32_t APP_UID = 20010029;
constexpr int32_t APP_PID = 3328;
constexpr int32_t SYS_PID = 100;
const std::string APP_EXTRA = "\"PID\":3328,\"UID\":20010029";

std::shared_ptr<SysEvent> BuildEvent(const std::string& domain, const std::string& eventName,
    int32_t pid, int32_t uid, const std::string& extra = "")
{
    std::string json = R"~({"domain_":")~" + domain + R"~(",)~";
    json += "\"name_\":\"" + eventName + "\",\"type_\":1,\"time_\":1620271291188,";
    json += "\"pid_\":" + std::to_string(pid) + ",\"tid_\":" + std::to_string(pid);
    json += ",\"uid_\":" + std::to_string(uid);
    if (!extra.empty()) {
        json += "," + extra;
    }
    json += "}";
    auto evt = std::make_shared<SysEvent>(eventName, nullptr, json);
    evt->eventName_ = eventName;
    evt->domain_ = domain;
    return evt;
}
}

class EventLoggerConfigValidateTest : public testing::Test {
public:
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: EventLoggerConfigValidateTest001
 * @tc.desc: wholeDomainAllowed domains (ACE/FFRT/GRAPHICS_GAME/WEBVIEW) trusted sender passes
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest001, TestSize.Level1)
{
    std::vector<std::pair<std::string, std::string>> events = {
        {"ACE", "UI_BLOCK_6S"}, {"ACE", "UI_BLOCK_3S"}, {"ACE", "UIEXTENSION_TRANSPARENT_DETECTED"},
        {"FFRT", "SERIAL_TASK_TIMEOUT"}, {"FFRT", "TASK_DEADLOCK"}, {"FFRT", "CONGESTION"},
        {"GRAPHICS_GAME", "GAME_FREEZE_DUMPSTACK"},
        {"WEBVIEW", "PROCESS_FREEZE_WARNING"}, {"WEBVIEW", "RENDER_JS_FREEZE"},
    };
    for (auto& [domain, name] : events) {
        auto event = BuildEvent(domain, name, SYS_PID, HIVIEW_UID);
        ASSERT_TRUE(event != nullptr) << name;
        EXPECT_TRUE(EventFieldValidator::ValidateEvent(event)) << name;
    }
}

/**
 * @tc.name: EventLoggerConfigValidateTest002
 * @tc.desc: wholeDomainAllowed domains untrusted sender with valid fields passes
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest002, TestSize.Level1)
{
    std::vector<std::pair<std::string, std::string>> events = {
        {"ACE", "UI_BLOCK_6S"}, {"ACE", "UI_BLOCK_3S"}, {"ACE", "UIEXTENSION_TRANSPARENT_DETECTED"},
        {"FFRT", "SERIAL_TASK_TIMEOUT"}, {"FFRT", "TASK_DEADLOCK"}, {"FFRT", "CONGESTION"},
        {"GRAPHICS_GAME", "GAME_FREEZE_DUMPSTACK"},
        {"WEBVIEW", "PROCESS_FREEZE_WARNING"}, {"WEBVIEW", "RENDER_JS_FREEZE"},
    };
    for (auto& [domain, name] : events) {
        auto event = BuildEvent(domain, name, APP_PID, APP_UID, APP_EXTRA);
        ASSERT_TRUE(event != nullptr) << name;
        EXPECT_TRUE(EventFieldValidator::ValidateEvent(event)) << name;
    }
}

/**
 * @tc.name: EventLoggerConfigValidateTest003
 * @tc.desc: wholeDomainAllowed domains untrusted sender with invalid fields is rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest003, TestSize.Level1)
{
    auto badPid = BuildEvent("ACE", "UI_BLOCK_6S", APP_PID, APP_UID, "\"PID\":1");
    ASSERT_TRUE(badPid != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(badPid));
    auto badUid = BuildEvent("FFRT", "CONGESTION", APP_PID, APP_UID, "\"PID\":3328,\"UID\":5523");
    ASSERT_TRUE(badUid != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(badUid));
    auto badName = BuildEvent("WEBVIEW", "PROCESS_FREEZE_WARNING", APP_PID, APP_UID,
        APP_EXTRA + ",\"PACKAGE_NAME\":\"../etc\"");
    ASSERT_TRUE(badName != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(badName));
    auto badNum = BuildEvent("GRAPHICS_GAME", "GAME_FREEZE_DUMPSTACK", APP_PID, APP_UID,
        APP_EXTRA + ",\"INPUT_ID\":\"abc\"");
    ASSERT_TRUE(badNum != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(badNum));
}

/**
 * @tc.name: EventLoggerConfigValidateTest004
 * @tc.desc: unregistered domains (FORM_MANAGER/SYSTEM_NAV_UE/HMOS_SVC_BROKER/HIGPU) trusted sender passes
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest004, TestSize.Level1)
{
    std::vector<std::pair<std::string, std::string>> events = {
        {"FORM_MANAGER", "FORM_BLOCK_CALLSTACK"},
        {"SYSTEM_NAV_UE", "GESTURE_NAVIGATION_BACK"},
        {"HMOS_SVC_BROKER", "RETRY_EXCEPTION"},
        {"HIGPU", "DMD_HIGPU_JOB_HANG"}, {"HIGPU", "DMD_HIGPU_PAGE_FAULT"}, {"HIGPU", "DMD_HIGPU_JOB_FAIL"},
    };
    for (auto& [domain, name] : events) {
        auto event = BuildEvent(domain, name, SYS_PID, HIVIEW_UID);
        ASSERT_TRUE(event != nullptr) << name;
        EXPECT_TRUE(EventFieldValidator::ValidateEvent(event)) << name;
    }
}

/**
 * @tc.name: EventLoggerConfigValidateTest005
 * @tc.desc: unregistered domains untrusted sender with valid fields passes
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest005, TestSize.Level1)
{
    std::vector<std::pair<std::string, std::string>> events = {
        {"FORM_MANAGER", "FORM_BLOCK_CALLSTACK"},
        {"SYSTEM_NAV_UE", "GESTURE_NAVIGATION_BACK"},
        {"HMOS_SVC_BROKER", "RETRY_EXCEPTION"},
        {"HIGPU", "DMD_HIGPU_JOB_HANG"}, {"HIGPU", "DMD_HIGPU_PAGE_FAULT"}, {"HIGPU", "DMD_HIGPU_JOB_FAIL"},
    };
    for (auto& [domain, name] : events) {
        auto event = BuildEvent(domain, name, APP_PID, APP_UID, APP_EXTRA);
        ASSERT_TRUE(event != nullptr) << name;
        EXPECT_TRUE(EventFieldValidator::ValidateEvent(event)) << name;
    }
}

/**
 * @tc.name: EventLoggerConfigValidateTest006
 * @tc.desc: unregistered domains untrusted sender with invalid fields is rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest006, TestSize.Level1)
{
    auto badPid = BuildEvent("FORM_MANAGER", "FORM_BLOCK_CALLSTACK", APP_PID, APP_UID, "\"PID\":1");
    ASSERT_TRUE(badPid != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(badPid));
    auto badName = BuildEvent("HIGPU", "DMD_HIGPU_JOB_HANG", APP_PID, APP_UID,
        APP_EXTRA + ",\"PROCESS_NAME\":\";id\"");
    ASSERT_TRUE(badName != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(badName));
    auto badPath = BuildEvent("HMOS_SVC_BROKER", "RETRY_EXCEPTION", APP_PID, APP_UID,
        APP_EXTRA + ",\"FREEZE_INFO_PATH\":\"/etc/passwd\"");
    ASSERT_TRUE(badPath != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(badPath));
    auto badAruid = BuildEvent("SYSTEM_NAV_UE", "GESTURE_NAVIGATION_BACK", APP_PID, APP_UID,
        APP_EXTRA + ",\"APP_RUNNING_UNIQUE_ID\":\"abc\"");
    ASSERT_TRUE(badAruid != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(badAruid));
}

/**
 * @tc.name: EventLoggerConfigValidateTest007
 * @tc.desc: AAFWK domain events with FOUNDATION_UID trusted sender pass
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest007, TestSize.Level1)
{
    std::vector<std::string> events = {
        "THREAD_BLOCK_6S", "THREAD_BLOCK_3S", "LIFECYCLE_TIMEOUT", "LIFECYCLE_HALF_TIMEOUT",
        "LIFECYCLE_TIMEOUT_WARNING", "LIFECYCLE_HALF_TIMEOUT_WARNING", "APP_INPUT_BLOCK",
        "BUSSINESS_THREAD_BLOCK_6S", "BUSSINESS_THREAD_BLOCK_3S", "FREEZE_HALF_HIVIEW_LOG",
        "BUSINESS_INPUT_BLOCK",
    };
    for (auto& name : events) {
        auto event = BuildEvent("AAFWK", name, SYS_PID, FOUNDATION_UID);
        ASSERT_TRUE(event != nullptr) << name;
        EXPECT_TRUE(EventFieldValidator::ValidateEvent(event)) << name;
    }
}

/**
 * @tc.name: EventLoggerConfigValidateTest008
 * @tc.desc: AAFWK domain allowed events with untrusted sender pass
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest008, TestSize.Level1)
{
    std::vector<std::string> events = {
        "BUSSINESS_THREAD_BLOCK_3S", "BUSSINESS_THREAD_BLOCK_6S",
        "BUSINESS_INPUT_BLOCK", "FREEZE_HALF_HIVIEW_LOG",
    };
    for (auto& name : events) {
        auto event = BuildEvent("AAFWK", name, APP_PID, APP_UID, APP_EXTRA);
        ASSERT_TRUE(event != nullptr) << name;
        EXPECT_TRUE(EventFieldValidator::ValidateEvent(event)) << name;
    }
}

/**
 * @tc.name: EventLoggerConfigValidateTest009
 * @tc.desc: AAFWK domain non-allowed events with untrusted sender are rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest009, TestSize.Level1)
{
    std::vector<std::string> events = {
        "THREAD_BLOCK_6S", "THREAD_BLOCK_3S", "LIFECYCLE_TIMEOUT", "LIFECYCLE_HALF_TIMEOUT",
        "LIFECYCLE_TIMEOUT_WARNING", "LIFECYCLE_HALF_TIMEOUT_WARNING", "APP_INPUT_BLOCK",
    };
    for (auto& name : events) {
        auto event = BuildEvent("AAFWK", name, APP_PID, APP_UID, APP_EXTRA);
        ASSERT_TRUE(event != nullptr) << name;
        EXPECT_FALSE(EventFieldValidator::ValidateEvent(event)) << name;
    }
}

/**
 * @tc.name: EventLoggerConfigValidateTest010
 * @tc.desc: AAFWK domain non-FOUNDATION trusted sender is rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest010, TestSize.Level1)
{
    auto event = BuildEvent("AAFWK", "THREAD_BLOCK_6S", SYS_PID, HIVIEW_UID);
    ASSERT_TRUE(event != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(event));
    auto event2 = BuildEvent("AAFWK", "LIFECYCLE_TIMEOUT", SYS_PID, 3000);
    ASSERT_TRUE(event2 != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(event2));
}

/**
 * @tc.name: EventLoggerConfigValidateTest011
 * @tc.desc: AAFWK domain allowed events untrusted sender with invalid fields is rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest011, TestSize.Level1)
{
    auto badPid = BuildEvent("AAFWK", "BUSSINESS_THREAD_BLOCK_3S", APP_PID, APP_UID, "\"PID\":1");
    ASSERT_TRUE(badPid != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(badPid));
    auto badName = BuildEvent("AAFWK", "BUSSINESS_THREAD_BLOCK_6S", APP_PID, APP_UID,
        APP_EXTRA + ",\"MODULE_NAME\":\"../etc\"");
    ASSERT_TRUE(badName != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(badName));
    auto badNum = BuildEvent("AAFWK", "BUSINESS_INPUT_BLOCK", APP_PID, APP_UID,
        APP_EXTRA + ",\"REMOTE_PID\":\"xyz\"");
    ASSERT_TRUE(badNum != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(badNum));
}

/**
 * @tc.name: EventLoggerConfigValidateTest012
 * @tc.desc: FRAMEWORK domain events with trusted sender pass
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest012, TestSize.Level1)
{
    std::vector<std::string> events = {
        "SERVICE_BLOCK", "SERVICE_WARNING", "SERVICE_TIMEOUT", "SERVICE_TIMEOUT_WARNING",
        "IPC_FULL_WARNING", "IPC_FULL", "HIT_EMPTY_WARNING", "APP_HICOLLIE",
        "FREQUENT_CLICK_WARNING", "USER_PANIC_WARNING", "BINDER_BUFFER_FULL_WARNING",
        "BINDER_BUFFER_FULL", "CES_SUBSCRIBER_OVER_LIMIT", "SAMGR_SERVICE_BLOCK",
    };
    for (auto& name : events) {
        auto event = BuildEvent("FRAMEWORK", name, SYS_PID, 3000);
        ASSERT_TRUE(event != nullptr) << name;
        EXPECT_TRUE(EventFieldValidator::ValidateEvent(event)) << name;
    }
}

/**
 * @tc.name: EventLoggerConfigValidateTest013
 * @tc.desc: FRAMEWORK domain allowed events with untrusted sender pass
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest013, TestSize.Level1)
{
    auto e1 = BuildEvent("FRAMEWORK", "APP_HICOLLIE", APP_PID, APP_UID, APP_EXTRA);
    ASSERT_TRUE(e1 != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(e1));
    auto e2 = BuildEvent("FRAMEWORK", "HIT_EMPTY_WARNING", APP_PID, APP_UID, APP_EXTRA);
    ASSERT_TRUE(e2 != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(e2));
}

/**
 * @tc.name: EventLoggerConfigValidateTest014
 * @tc.desc: FRAMEWORK domain non-allowed events with untrusted sender are rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest014, TestSize.Level1)
{
    std::vector<std::string> events = {
        "SERVICE_BLOCK", "SERVICE_WARNING", "SERVICE_TIMEOUT", "SERVICE_TIMEOUT_WARNING",
        "IPC_FULL_WARNING", "IPC_FULL", "FREQUENT_CLICK_WARNING", "USER_PANIC_WARNING",
        "BINDER_BUFFER_FULL_WARNING", "BINDER_BUFFER_FULL", "CES_SUBSCRIBER_OVER_LIMIT",
        "SAMGR_SERVICE_BLOCK",
    };
    for (auto& name : events) {
        auto event = BuildEvent("FRAMEWORK", name, APP_PID, APP_UID, APP_EXTRA);
        ASSERT_TRUE(event != nullptr) << name;
        EXPECT_FALSE(EventFieldValidator::ValidateEvent(event)) << name;
    }
}

/**
 * @tc.name: EventLoggerConfigValidateTest015
 * @tc.desc: FRAMEWORK domain allowed events untrusted sender with invalid fields is rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest015, TestSize.Level1)
{
    auto badPid = BuildEvent("FRAMEWORK", "APP_HICOLLIE", APP_PID, APP_UID, "\"PID\":1");
    ASSERT_TRUE(badPid != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(badPid));
    auto badName = BuildEvent("FRAMEWORK", "HIT_EMPTY_WARNING", APP_PID, APP_UID,
        APP_EXTRA + ",\"PNAMEID\":\";rm\"");
    ASSERT_TRUE(badName != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(badName));
}

/**
 * @tc.name: EventLoggerConfigValidateTest016
 * @tc.desc: GRAPHIC domain events with GRAPHICS_UID trusted sender pass
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest016, TestSize.Level1)
{
    auto e1 = BuildEvent("GRAPHIC", "RS_VULKAN_ERROR", SYS_PID, GRAPHICS_UID);
    ASSERT_TRUE(e1 != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(e1));
    auto e2 = BuildEvent("GRAPHIC", "RS_RENDER_EXCEPTION", SYS_PID, GRAPHICS_UID);
    ASSERT_TRUE(e2 != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(e2));
}

/**
 * @tc.name: EventLoggerConfigValidateTest017
 * @tc.desc: GRAPHIC domain events with non-GRAPHICS_UID or untrusted sender are rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest017, TestSize.Level1)
{
    auto nonGraphicsTrusted = BuildEvent("GRAPHIC", "RS_VULKAN_ERROR", SYS_PID, FOUNDATION_UID);
    ASSERT_TRUE(nonGraphicsTrusted != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(nonGraphicsTrusted));
    auto untrusted = BuildEvent("GRAPHIC", "RS_RENDER_EXCEPTION", APP_PID, APP_UID, APP_EXTRA);
    ASSERT_TRUE(untrusted != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(untrusted));
    auto hiviewTrusted = BuildEvent("GRAPHIC", "RS_VULKAN_ERROR", SYS_PID, HIVIEW_UID);
    ASSERT_TRUE(hiviewTrusted != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(hiviewTrusted));
}

/**
 * @tc.name: EventLoggerConfigValidateTest018
 * @tc.desc: KERNEL_VENDOR domain events with ROOT_UID and HIVIEW_UID trusted sender pass
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest018, TestSize.Level1)
{
    std::vector<std::string> events = {
        "SCREEN_ON", "SCREEN_OFF", "HUNGTASK", "COM_LONG_PRESS",
        "DMD_TP_I2C", "DMD_TP_HP", "DMD_LCD_DDR", "DMD_EMMC_TUNING",
        "DMD_EXT4", "DMD_F2FS_UNLINK", "DMD_UFS_FASTBOOT", "DMD_FSCK_F2FS",
    };
    for (auto& name : events) {
        auto rootEvt = BuildEvent("KERNEL_VENDOR", name, 1, ROOT_UID);
        ASSERT_TRUE(rootEvt != nullptr) << name;
        EXPECT_TRUE(EventFieldValidator::ValidateEvent(rootEvt)) << name;
        auto hiviewEvt = BuildEvent("KERNEL_VENDOR", name, SYS_PID, HIVIEW_UID);
        ASSERT_TRUE(hiviewEvt != nullptr) << name;
        EXPECT_TRUE(EventFieldValidator::ValidateEvent(hiviewEvt)) << name;
    }
}

/**
 * @tc.name: EventLoggerConfigValidateTest019
 * @tc.desc: KERNEL_VENDOR domain events with non-allowed trusted or untrusted sender are rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest019, TestSize.Level1)
{
    std::vector<std::string> events = {"SCREEN_ON", "HUNGTASK", "COM_LONG_PRESS", "DMD_TP_I2C"};
    for (auto& name : events) {
        auto trusted = BuildEvent("KERNEL_VENDOR", name, SYS_PID, FOUNDATION_UID);
        ASSERT_TRUE(trusted != nullptr) << name;
        EXPECT_FALSE(EventFieldValidator::ValidateEvent(trusted)) << name;
        auto untrusted = BuildEvent("KERNEL_VENDOR", name, APP_PID, APP_UID, APP_EXTRA);
        ASSERT_TRUE(untrusted != nullptr) << name;
        EXPECT_FALSE(EventFieldValidator::ValidateEvent(untrusted)) << name;
    }
}

/**
 * @tc.name: EventLoggerConfigValidateTest020
 * @tc.desc: MULTIMODALINPUT domain event with INPUT_UID passes, other senders are rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest020, TestSize.Level1)
{
    auto success = BuildEvent("MULTIMODALINPUT", "INPUT_EVENT_SOCKET_TIMEOUT", SYS_PID, INPUT_UID);
    ASSERT_TRUE(success != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(success));
    auto wrongTrusted = BuildEvent("MULTIMODALINPUT", "INPUT_EVENT_SOCKET_TIMEOUT", SYS_PID, FOUNDATION_UID);
    ASSERT_TRUE(wrongTrusted != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(wrongTrusted));
    auto untrusted = BuildEvent("MULTIMODALINPUT", "INPUT_EVENT_SOCKET_TIMEOUT", APP_PID, APP_UID, APP_EXTRA);
    ASSERT_TRUE(untrusted != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(untrusted));
}

/**
 * @tc.name: EventLoggerConfigValidateTest021
 * @tc.desc: POWER domain event with POWERMGR_UID passes, other senders are rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest021, TestSize.Level1)
{
    auto success = BuildEvent("POWER", "SCREEN_ON_TIMEOUT", SYS_PID, POWERMGR_UID);
    ASSERT_TRUE(success != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(success));
    auto wrongTrusted = BuildEvent("POWER", "SCREEN_ON_TIMEOUT", SYS_PID, FOUNDATION_UID);
    ASSERT_TRUE(wrongTrusted != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(wrongTrusted));
    auto untrusted = BuildEvent("POWER", "SCREEN_ON_TIMEOUT", APP_PID, APP_UID, APP_EXTRA);
    ASSERT_TRUE(untrusted != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(untrusted));
}

/**
 * @tc.name: EventLoggerConfigValidateTest022
 * @tc.desc: RELIABILITY domain event with HIVIEW/FOUNDATION passes, other senders are rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest022, TestSize.Level1)
{
    auto hiviewEvt = BuildEvent("RELIABILITY", "APP_FREEZE_BETACLUB", SYS_PID, HIVIEW_UID);
    ASSERT_TRUE(hiviewEvt != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(hiviewEvt));
    auto foundEvt = BuildEvent("RELIABILITY", "APP_FREEZE_BETACLUB", SYS_PID, FOUNDATION_UID);
    ASSERT_TRUE(foundEvt != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(foundEvt));
    auto wrongTrusted = BuildEvent("RELIABILITY", "APP_FREEZE_BETACLUB", SYS_PID, POWERMGR_UID);
    ASSERT_TRUE(wrongTrusted != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(wrongTrusted));
    auto untrusted = BuildEvent("RELIABILITY", "APP_FREEZE_BETACLUB", APP_PID, APP_UID, APP_EXTRA);
    ASSERT_TRUE(untrusted != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(untrusted));
}

/**
 * @tc.name: EventLoggerConfigValidateTest023
 * @tc.desc: THP domain event with trusted sender passes, untrusted sender is rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest023, TestSize.Level1)
{
    auto trusted = BuildEvent("THP", "DMDW_THP_DAE_PROCESS_TIMEOUT", SYS_PID, 3000);
    ASSERT_TRUE(trusted != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(trusted));
    auto untrusted = BuildEvent("THP", "DMDW_THP_DAE_PROCESS_TIMEOUT", APP_PID, APP_UID, APP_EXTRA);
    ASSERT_TRUE(untrusted != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(untrusted));
}

/**
 * @tc.name: EventLoggerConfigValidateTest024
 * @tc.desc: WINDOWMANAGER domain events with FOUNDATION_UID trusted sender pass
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest024, TestSize.Level1)
{
    std::vector<std::string> events = {
        "NO_FOCUS_WINDOW", "GET_DISPLAY_SNAPSHOT", "CREATE_VIRTUAL_SCREEN",
        "REPEAT_SET_UI_NODE_ID", "WINDOW_EXCEPTION_DETECTION", "WINDOW_STATE_ERROR",
        "WINDOW_FROZEN_DETECTION",
    };
    for (auto& name : events) {
        auto event = BuildEvent("WINDOWMANAGER", name, SYS_PID, FOUNDATION_UID);
        ASSERT_TRUE(event != nullptr) << name;
        EXPECT_TRUE(EventFieldValidator::ValidateEvent(event)) << name;
    }
}

/**
 * @tc.name: EventLoggerConfigValidateTest025
 * @tc.desc: WINDOWMANAGER domain events with non-FOUNDATION trusted or untrusted sender are rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest025, TestSize.Level1)
{
    std::vector<std::string> events = {
        "NO_FOCUS_WINDOW", "GET_DISPLAY_SNAPSHOT", "CREATE_VIRTUAL_SCREEN",
        "REPEAT_SET_UI_NODE_ID", "WINDOW_EXCEPTION_DETECTION", "WINDOW_STATE_ERROR",
        "WINDOW_FROZEN_DETECTION",
    };
    for (auto& name : events) {
        auto wrongTrusted = BuildEvent("WINDOWMANAGER", name, SYS_PID, HIVIEW_UID);
        ASSERT_TRUE(wrongTrusted != nullptr) << name;
        EXPECT_FALSE(EventFieldValidator::ValidateEvent(wrongTrusted)) << name;
    }
    for (auto& name : events) {
        auto untrusted = BuildEvent("WINDOWMANAGER", name, APP_PID, APP_UID, APP_EXTRA);
        ASSERT_TRUE(untrusted != nullptr) << name;
        EXPECT_FALSE(EventFieldValidator::ValidateEvent(untrusted)) << name;
    }
}

/**
 * @tc.name: EventLoggerConfigValidateTest026
 * @tc.desc: WINDOWMANAGER domain events with INPUT_UID or untrusted sender are rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest026, TestSize.Level1)
{
    auto wrongTrusted = BuildEvent("WINDOWMANAGER", "NO_FOCUS_WINDOW", SYS_PID, INPUT_UID);
    ASSERT_TRUE(wrongTrusted != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(wrongTrusted));
    auto untrusted = BuildEvent("WINDOWMANAGER", "WINDOW_STATE_ERROR", APP_PID, APP_UID, APP_EXTRA);
    ASSERT_TRUE(untrusted != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(untrusted));
}

/**
 * @tc.name: EventLoggerConfigValidateTest027
 * @tc.desc: untrusted sender with MAIN_STACK as absolute path is rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest027, TestSize.Level1)
{
    auto badMainStack = BuildEvent("ACE", "UI_BLOCK_6S", APP_PID, APP_UID,
        APP_EXTRA + ",\"MAIN_STACK\":\"/data/log/some_stack\"");
    ASSERT_TRUE(badMainStack != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(badMainStack));
    auto validMainStack = BuildEvent("ACE", "UI_BLOCK_3S", APP_PID, APP_UID,
        APP_EXTRA + ",\"MAIN_STACK\":\"#00 pc 0001234\"");
    ASSERT_TRUE(validMainStack != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(validMainStack));
}

/**
 * @tc.name: EventLoggerConfigValidateTest029
 * @tc.desc: untrusted sender with oversized small/large content fields is rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest029, TestSize.Level1)
{
    std::string oversized(9000, 'a');
    auto badHitrace = BuildEvent("FFRT", "CONGESTION", APP_PID, APP_UID,
        APP_EXTRA + ",\"HITRACE_ID\":\"" + oversized + "\"");
    ASSERT_TRUE(badHitrace != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(badHitrace));
    std::string oversizedMsg(280 * 1024, 'b');
    auto badMsg = BuildEvent("WEBVIEW", "RENDER_JS_FREEZE", APP_PID, APP_UID,
        APP_EXTRA + ",\"MSG\":\"" + oversizedMsg + "\"");
    ASSERT_TRUE(badMsg != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(badMsg));
}

/**
 * @tc.name: EventLoggerConfigValidateTest030
 * @tc.desc: trusted sender bypasses untrusted field validation (PID/name mismatch allowed)
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest030, TestSize.Level1)
{
    auto badPidTrusted = BuildEvent("AAFWK", "THREAD_BLOCK_6S", SYS_PID, FOUNDATION_UID,
        "\"PID\":99999,\"UID\":99999");
    ASSERT_TRUE(badPidTrusted != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(badPidTrusted));
    auto badNameTrusted = BuildEvent("FRAMEWORK", "SERVICE_BLOCK", SYS_PID, 3000,
        "\"PACKAGE_NAME\":\"../etc\"");
    ASSERT_TRUE(badNameTrusted != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(badNameTrusted));
}

/**
 * @tc.name: EventLoggerConfigValidateTest031
 * @tc.desc: trusted sender with MAIN_STACK as absolute path is still rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest031, TestSize.Level1)
{
    auto badStack = BuildEvent("AAFWK", "THREAD_BLOCK_6S", SYS_PID, FOUNDATION_UID,
        "\"MAIN_STACK\":\"/data/log/some_stack\"");
    ASSERT_TRUE(badStack != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(badStack));
}

/**
 * @tc.name: EventLoggerConfigValidateTest032
 * @tc.desc: ValidateEvent rejects nullptr event
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest032, TestSize.Level1)
{
    std::shared_ptr<SysEvent> nullEvent = nullptr;
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(nullEvent));
}

/**
 * @tc.name: EventLoggerConfigValidateTest033
 * @tc.desc: KERNEL_VENDOR remaining events with untrusted sender are rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventLoggerConfigValidateTest, EventLoggerConfigValidateTest033, TestSize.Level1)
{
    std::vector<std::string> events = {
        "SCREEN_OFF", "DMD_TP_HP", "DMD_LCD_DDR", "DMD_EMMC_TUNING",
        "DMD_EXT4", "DMD_F2FS_UNLINK", "DMD_UFS_FASTBOOT", "DMD_FSCK_F2FS",
    };
    for (auto& name : events) {
        auto untrusted = BuildEvent("KERNEL_VENDOR", name, APP_PID, APP_UID, APP_EXTRA);
        ASSERT_TRUE(untrusted != nullptr) << name;
        EXPECT_FALSE(EventFieldValidator::ValidateEvent(untrusted)) << name;
    }
}
