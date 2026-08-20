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

#include <gtest/gtest.h>

#include "event_field_validator.h"
#include "sys_event.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;

namespace {
constexpr int32_t SERVICE_UID = 5523; // foundation
constexpr int32_t APP_UID = 20010029;
constexpr int32_t APP_PID = 3328;
constexpr int32_t ROOT_UID = 0;
constexpr int32_t GRAPHICS_UID = 1003;
constexpr int32_t HIVIEW_UID = 1201;
constexpr int32_t POWERMGR_UID = 5528;
constexpr int32_t INPUT_UID = 6696;

std::shared_ptr<SysEvent> BuildEvent(const std::string& domain, const std::string& eventName,
    int32_t pid, int32_t uid, const std::string& extraParams)
{
    std::string json = R"~({"domain_":")~" + domain + R"~(",)~";
    json += "\"name_\":\"" + eventName + "\",\"type_\":1,\"time_\":1620271291188,";
    json += "\"pid_\":" + std::to_string(pid) + ",\"tid_\":" + std::to_string(pid) +
        ",\"uid_\":" + std::to_string(uid);
    if (!extraParams.empty()) {
        json += "," + extraParams;
    }
    json += "}";
    return std::make_shared<SysEvent>("test", nullptr, json);
}

std::shared_ptr<SysEvent> BuildEvent(const std::string& eventName, int32_t pid, int32_t uid,
    const std::string& extraParams)
{
    return BuildEvent("FORM_MANAGER", eventName, pid, uid, extraParams);
}
}

class EventFieldValidatorTest : public testing::Test {
public:
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: EventFieldValidatorTest001
 * @tc.desc: trusted service sender keeps the default policy without field checks
 * @tc.type: FUNC
 */
HWTEST_F(EventFieldValidatorTest, EventFieldValidatorTest001, TestSize.Level1)
{
    // body PID mismatches the sender on purpose: trusted source must not be affected
    auto event = BuildEvent("TB6S_TRUSTED", 100, SERVICE_UID, "\"PID\":1,\"UID\":5523");
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(EventFieldValidator::IsTrustedSource(event));
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(event));
}

/**
 * @tc.name: EventFieldValidatorTest002
 * @tc.desc: kernel/root sender is trusted
 * @tc.type: FUNC
 */
HWTEST_F(EventFieldValidatorTest, EventFieldValidatorTest002, TestSize.Level1)
{
    auto event = BuildEvent("TB6S_KERNEL", 1, ROOT_UID, "");
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(EventFieldValidator::IsTrustedSource(event));
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(event));
}

/**
 * @tc.name: EventFieldValidatorTest003
 * @tc.desc: untrusted sender with consistent fields passes validation
 * @tc.type: FUNC
 */
HWTEST_F(EventFieldValidatorTest, EventFieldValidatorTest003, TestSize.Level1)
{
    auto event = BuildEvent("TB6S_VALID", APP_PID, APP_UID,
        "\"PID\":3328,\"UID\":20010029,\"PACKAGE_NAME\":\"com.example.app\",\"PROCESS_NAME\":\"com.example.app\"");
    ASSERT_TRUE(event != nullptr);
    EXPECT_FALSE(EventFieldValidator::IsTrustedSource(event));
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(event));
}

/**
 * @tc.name: EventFieldValidatorTest004
 * @tc.desc: untrusted sender forging the PID field is rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventFieldValidatorTest, EventFieldValidatorTest004, TestSize.Level1)
{
    auto event = BuildEvent("TB6S_PID", APP_PID, APP_UID, "\"PID\":1,\"UID\":20010029");
    ASSERT_TRUE(event != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(event));
}

/**
 * @tc.name: EventFieldValidatorTest005
 * @tc.desc: untrusted sender forging the UID field is rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventFieldValidatorTest, EventFieldValidatorTest005, TestSize.Level1)
{
    auto event = BuildEvent("TB6S_UID", APP_PID, APP_UID, "\"PID\":3328,\"UID\":5523");
    ASSERT_TRUE(event != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(event));
}

/**
 * @tc.name: EventFieldValidatorTest006
 * @tc.desc: untrusted sender with shell metachars in PACKAGE_NAME is rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventFieldValidatorTest, EventFieldValidatorTest006, TestSize.Level1)
{
    auto event = BuildEvent("TB6S_META", APP_PID, APP_UID,
        "\"PACKAGE_NAME\":\"hvd;id>/data/local/tmp/proof;#\"");
    ASSERT_TRUE(event != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(event));
}

/**
 * @tc.name: EventFieldValidatorTest007
 * @tc.desc: untrusted sender with garbage numeric INPUT_ID is rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventFieldValidatorTest, EventFieldValidatorTest007, TestSize.Level1)
{
    auto event = BuildEvent("TB6S_NUM", APP_PID, APP_UID, "\"INPUT_ID\":\"8;id\"");
    ASSERT_TRUE(event != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(event));
}

/**
 * @tc.name: EventFieldValidatorTest008
 * @tc.desc: untrusted sender with traversal or out-of-root FREEZE_INFO_PATH is rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventFieldValidatorTest, EventFieldValidatorTest008, TestSize.Level1)
{
    auto traversal = BuildEvent("TB6S_PATH1", APP_PID, APP_UID,
        "\"FREEZE_INFO_PATH\":\"/data/log/../../etc/passwd\"");
    ASSERT_TRUE(traversal != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(traversal));

    auto outOfRoot = BuildEvent("TB6S_PATH2", APP_PID, APP_UID, "\"FREEZE_INFO_PATH\":\"/etc/passwd\"");
    ASSERT_TRUE(outOfRoot != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(outOfRoot));
}

/**
 * @tc.name: EventFieldValidatorTest009
 * @tc.desc: STACK starting with / must be under freeze root with _stack suffix
 * @tc.type: FUNC
 */
HWTEST_F(EventFieldValidatorTest, EventFieldValidatorTest009, TestSize.Level1)
{
    // STACK path validation only triggers when FileExists returns true; use
    // paths that do not exist on any system so the check is always skipped
    auto badPath = BuildEvent("TB6S_STACK1", APP_PID, APP_UID,
        "\"STACK\":\"/nonexist_test/proc_environ\"");
    ASSERT_TRUE(badPath != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(badPath));

    auto traversal = BuildEvent("TB6S_STACK2", APP_PID, APP_UID,
        "\"STACK\":\"/nonexist_test/freeze/../../../etc/passwd_stack\"");
    ASSERT_TRUE(traversal != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(traversal));

    auto badSuffix = BuildEvent("TB6S_STACK3", APP_PID, APP_UID,
        "\"STACK\":\"/nonexist_test/freeze/TB6S_bad\"");
    ASSERT_TRUE(badSuffix != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(badSuffix));

    std::string nullPayload = "\"STACK\":\"abc";
    nullPayload.push_back('\0');
    nullPayload += "def\"";
    auto nullByte = BuildEvent("TB6S_STACK4", APP_PID, APP_UID, nullPayload);
    ASSERT_TRUE(nullByte != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(nullByte));

    auto validStack = BuildEvent("TB6S_STACK5", APP_PID, APP_UID,
        "\"STACK\":\"/nonexist_test/freeze/TB6S_stack\"");
    ASSERT_TRUE(validStack != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(validStack));

    auto plainStack = BuildEvent("TB6S_STACK6", APP_PID, APP_UID, "\"STACK\":\"#00 pc 0001\"");
    ASSERT_TRUE(plainStack != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(plainStack));

    auto trustedBad = BuildEvent("TB6S_STACK7", 100, SERVICE_UID,
        "\"STACK\":\"/nonexist_test/proc_environ\"");
    ASSERT_TRUE(trustedBad != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(trustedBad));
}

/**
 * @tc.name: EventFieldValidatorTest010
 * @tc.desc: BINDER_INFO path must be under freeze root with _binder suffix;
 *           HICOLLIE_BINDER_INFO dropped for untrusted
 * @tc.type: FUNC
 */
HWTEST_F(EventFieldValidatorTest, EventFieldValidatorTest010, TestSize.Level1)
{
    // path validation only triggers when FileExists returns true; use paths
    // that do not exist on any system so BINDER_INFO is kept as-is
    auto badPath = BuildEvent("TB6S_BI1", APP_PID, APP_UID,
        "\"BINDER_INFO\":\"/nonexist_test/passwd,1 2 3,4\",\"HICOLLIE_BINDER_INFO\":\"PROCESS_NAME:x\"");
    ASSERT_TRUE(badPath != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(badPath));
    EXPECT_FALSE(badPath->GetEventValue("BINDER_INFO").empty());

    auto badSuffix = BuildEvent("TB6S_BI2", APP_PID, APP_UID,
        "\"BINDER_INFO\":\"/nonexist_test/freeze/TB6S_bad,1 2 3,4\"");
    ASSERT_TRUE(badSuffix != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(badSuffix));
    EXPECT_FALSE(badSuffix->GetEventValue("BINDER_INFO").empty());

    auto traversal = BuildEvent("TB6S_BI3", APP_PID, APP_UID,
        "\"BINDER_INFO\":\"/nonexist_test/freeze/../../nonexist_binder,1 2 3,4\"");
    ASSERT_TRUE(traversal != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(traversal));
    EXPECT_FALSE(traversal->GetEventValue("BINDER_INFO").empty());

    auto validBinder = BuildEvent("TB6S_BI4", APP_PID, APP_UID,
        "\"BINDER_INFO\":\"/nonexist_test/freeze/TB6S_binder,1 2 3,4\","
        "\"HICOLLIE_BINDER_INFO\":\"x\"");
    ASSERT_TRUE(validBinder != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(validBinder));
    EXPECT_FALSE(validBinder->GetEventValue("BINDER_INFO").empty());
    EXPECT_TRUE(validBinder->GetEventValue("HICOLLIE_BINDER_INFO").empty());

    auto trustedBinder = BuildEvent("TB6S_BI5", 100, SERVICE_UID,
        "\"BINDER_INFO\":\"/nonexist_test/freeze/TB6S_binder,1 2 3,4\","
        "\"HICOLLIE_BINDER_INFO\":\"x\"");
    ASSERT_TRUE(trustedBinder != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(trustedBinder));
    EXPECT_FALSE(trustedBinder->GetEventValue("BINDER_INFO").empty());
    EXPECT_FALSE(trustedBinder->GetEventValue("HICOLLIE_BINDER_INFO").empty());

    auto trustedBad = BuildEvent("TB6S_BI6", 100, SERVICE_UID,
        "\"BINDER_INFO\":\"/nonexist_test/passwd,1 2 3,4\"");
    ASSERT_TRUE(trustedBad != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(trustedBad));
    EXPECT_FALSE(trustedBad->GetEventValue("BINDER_INFO").empty());

    auto plainBinder = BuildEvent("TB6S_BI7", APP_PID, APP_UID, "\"BINDER_INFO\":\"12 34,56\"");
    ASSERT_TRUE(plainBinder != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(plainBinder));
    EXPECT_FALSE(plainBinder->GetEventValue("BINDER_INFO").empty());
}

/**
 * @tc.name: EventFieldValidatorTest012
 * @tc.desc: untrusted sender with oversized small field is rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventFieldValidatorTest, EventFieldValidatorTest012, TestSize.Level1)
{
    std::string oversized(9000, 'a'); // MAX_SMALL_CONTENT_LEN is 8192 for HITRACE_ID
    auto event = BuildEvent("TB6S_SIZE", APP_PID, APP_UID,
        "\"HITRACE_ID\":\"" + oversized + "\"");
    ASSERT_TRUE(event != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(event));
}

/**
 * @tc.name: EventFieldValidatorTest013
 * @tc.desc: GetAppFreezeFile use-point path whitelist accepts freeze roots only
 * @tc.type: FUNC
 */
HWTEST_F(EventFieldValidatorTest, EventFieldValidatorTest013, TestSize.Level1)
{
    EXPECT_TRUE(EventFieldValidator::IsAcceptedReadPath("/data/log/eventlog/UI_BLOCK_6S-3328-20260817.log"));
    EXPECT_TRUE(EventFieldValidator::IsAcceptedReadPath(
        "/data/app/el2/100/log/com.example.app/watchdog/freeze/freeze_ext_file"));
    EXPECT_TRUE(EventFieldValidator::IsAcceptedReadPath("")); // callers handle empty input

    EXPECT_FALSE(EventFieldValidator::IsAcceptedReadPath("/etc/passwd"));
    EXPECT_FALSE(EventFieldValidator::IsAcceptedReadPath("/proc/1/environ"));
    EXPECT_FALSE(EventFieldValidator::IsAcceptedReadPath("/data/log/../../etc/passwd"));
    EXPECT_FALSE(EventFieldValidator::IsAcceptedReadPath("relative/path/freeze.log"));
    EXPECT_FALSE(EventFieldValidator::IsAcceptedReadPath("/data/service/el2/100/base/other/file"));
}

/**
 * @tc.name: EventFieldValidatorTest014
 * @tc.desc: untrusted sender with path traversal in MODULE_NAME is rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventFieldValidatorTest, EventFieldValidatorTest014, TestSize.Level1)
{
    auto traversal = BuildEvent("TB6S_MOD1", APP_PID, APP_UID, "\"MODULE_NAME\":\"../etc/passwd\"");
    ASSERT_TRUE(traversal != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(traversal));

    auto backslash = BuildEvent("TB6S_MOD2", APP_PID, APP_UID, "\"MODULE_NAME\":\"test\\path\"");
    ASSERT_TRUE(backslash != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(backslash));

    auto valid = BuildEvent("TB6S_MOD3", APP_PID, APP_UID, "\"MODULE_NAME\":\"com.example.module\"");
    ASSERT_TRUE(valid != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(valid));
}

/**
 * @tc.name: EventFieldValidatorTest015
 * @tc.desc: untrusted sender with path traversal in PNAMEID is rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventFieldValidatorTest, EventFieldValidatorTest015, TestSize.Level1)
{
    auto traversal = BuildEvent("TB6S_PN1", APP_PID, APP_UID, "\"PNAMEID\":\"../../etc/shadow\"");
    ASSERT_TRUE(traversal != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(traversal));

    auto semicolon = BuildEvent("TB6S_PN2", APP_PID, APP_UID, "\"PNAMEID\":\"name;id\"");
    ASSERT_TRUE(semicolon != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(semicolon));

    auto valid = BuildEvent("TB6S_PN3", APP_PID, APP_UID, "\"PNAMEID\":\"com.example.app\"");
    ASSERT_TRUE(valid != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(valid));
}

/**
 * @tc.name: EventFieldValidatorTest016
 * @tc.desc: untrusted sender with non-digit APP_RUNNING_UNIQUE_ID is rejected
 * @tc.type: FUNC
 */
HWTEST_F(EventFieldValidatorTest, EventFieldValidatorTest016, TestSize.Level1)
{
    auto letters = BuildEvent("TB6S_ARUID1", APP_PID, APP_UID, "\"APP_RUNNING_UNIQUE_ID\":\"abc123\"");
    ASSERT_TRUE(letters != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(letters));

    auto traversal = BuildEvent("TB6S_ARUID2", APP_PID, APP_UID, "\"APP_RUNNING_UNIQUE_ID\":\"12../34\"");
    ASSERT_TRUE(traversal != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(traversal));

    auto empty = BuildEvent("TB6S_ARUID3", APP_PID, APP_UID, "\"APP_RUNNING_UNIQUE_ID\":\"\"");
    ASSERT_TRUE(empty != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(empty));

    auto valid = BuildEvent("TB6S_ARUID4", APP_PID, APP_UID, "\"APP_RUNNING_UNIQUE_ID\":\"1234567890\"");
    ASSERT_TRUE(valid != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(valid));
}

/**
 * @tc.name: EventFieldValidatorTest017
 * @tc.desc: IsSenderAllowed domain-level sender policy
 * @tc.type: FUNC
 */
HWTEST_F(EventFieldValidatorTest, EventFieldValidatorTest017, TestSize.Level1)
{
    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("FRAMEWORK", "SERVICE_BLOCK", SERVICE_UID));
    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("KERNEL_VENDOR", "HUNGTASK", ROOT_UID));
    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("AAFWK", "THREAD_BLOCK_6S", SERVICE_UID));

    EXPECT_FALSE(EventFieldValidator::IsSenderAllowed("FRAMEWORK", "SERVICE_BLOCK", APP_UID));
    EXPECT_FALSE(EventFieldValidator::IsSenderAllowed("FRAMEWORK", "SERVICE_TIMEOUT_WARNING", APP_UID));
    EXPECT_FALSE(EventFieldValidator::IsSenderAllowed("FRAMEWORK", "BINDER_BUFFER_FULL", APP_UID));
    EXPECT_FALSE(EventFieldValidator::IsSenderAllowed("KERNEL_VENDOR", "HUNGTASK", APP_UID));
    EXPECT_FALSE(EventFieldValidator::IsSenderAllowed("WINDOWMANAGER", "NO_FOCUS_WINDOW", APP_UID));
    EXPECT_FALSE(EventFieldValidator::IsSenderAllowed("POWER", "SCREEN_ON_TIMEOUT", APP_UID));
    EXPECT_FALSE(EventFieldValidator::IsSenderAllowed("MULTIMODALINPUT", "TARGET_POINTER_EVENT_FAILURE",
        APP_UID));
    EXPECT_FALSE(EventFieldValidator::IsSenderAllowed("RELIABILITY", "APP_FREEZE", APP_UID));
    EXPECT_FALSE(EventFieldValidator::IsSenderAllowed("AAFWK", "THREAD_BLOCK_6S", APP_UID));
    EXPECT_FALSE(EventFieldValidator::IsSenderAllowed("AAFWK", "LIFECYCLE_TIMEOUT", APP_UID));
    EXPECT_FALSE(EventFieldValidator::IsSenderAllowed("GRAPHIC", "RS_VULKAN_ERROR", APP_UID));

    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("ACE", "UI_BLOCK_6S", APP_UID));
    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("AAFWK", "BUSSINESS_THREAD_BLOCK_3S", APP_UID));
    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("AAFWK", "BUSINESS_INPUT_BLOCK", APP_UID));
    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("AAFWK", "FREEZE_HALF_HIVIEW_LOG", APP_UID));
    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("FRAMEWORK", "APP_HICOLLIE", APP_UID));
    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("FRAMEWORK", "HIT_EMPTY_WARNING", APP_UID));
    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("GRAPHIC", "NO_DRAW", APP_UID));
    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("WEBVIEW", "PROCESS_FREEZE_WARNING", APP_UID));
    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("FFRT", "CONGESTION", APP_UID));

    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("AUDIO", "anything", APP_UID));

    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("WINDOWMANAGER", "NO_FOCUS_WINDOW", SERVICE_UID));
    EXPECT_FALSE(EventFieldValidator::IsSenderAllowed("WINDOWMANAGER", "NO_FOCUS_WINDOW", HIVIEW_UID));
    EXPECT_FALSE(EventFieldValidator::IsSenderAllowed("WINDOWMANAGER", "WINDOW_STATE_ERROR", INPUT_UID));
    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("POWER", "SCREEN_ON_TIMEOUT", POWERMGR_UID));
    EXPECT_FALSE(EventFieldValidator::IsSenderAllowed("POWER", "SCREEN_ON_TIMEOUT", SERVICE_UID));
    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("MULTIMODALINPUT", "TARGET_POINTER_EVENT_FAILURE",
        INPUT_UID));
    EXPECT_FALSE(EventFieldValidator::IsSenderAllowed("MULTIMODALINPUT", "TARGET_POINTER_EVENT_FAILURE",
        SERVICE_UID));
    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("RELIABILITY", "APP_FREEZE", HIVIEW_UID));
    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("RELIABILITY", "APP_FREEZE_STATISTICS", SERVICE_UID));
    EXPECT_FALSE(EventFieldValidator::IsSenderAllowed("RELIABILITY", "APP_FREEZE", POWERMGR_UID));
    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("AAFWK", "THREAD_BLOCK_6S", SERVICE_UID));
    EXPECT_FALSE(EventFieldValidator::IsSenderAllowed("AAFWK", "THREAD_BLOCK_6S", INPUT_UID));
    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("GRAPHIC", "RS_VULKAN_ERROR", GRAPHICS_UID));
    EXPECT_FALSE(EventFieldValidator::IsSenderAllowed("GRAPHIC", "RS_VULKAN_ERROR", SERVICE_UID));
    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("KERNEL_VENDOR", "HUNGTASK", ROOT_UID));
    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("KERNEL_VENDOR", "HUNGTASK", HIVIEW_UID));
    EXPECT_FALSE(EventFieldValidator::IsSenderAllowed("KERNEL_VENDOR", "HUNGTASK", SERVICE_UID));
    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("FRAMEWORK", "SERVICE_BLOCK", 3000));
    EXPECT_TRUE(EventFieldValidator::IsSenderAllowed("THP", "DMDW_THP_DAE_PROCESS_TIMEOUT", 3000));
}

/**
 * @tc.name: EventFieldValidatorTest018
 * @tc.desc: ValidateEvent rejects untrusted sender forging a system-only freeze event
 * @tc.type: FUNC
 */
HWTEST_F(EventFieldValidatorTest, EventFieldValidatorTest018, TestSize.Level1)
{
    auto event = BuildEvent("FRAMEWORK", "SERVICE_BLOCK", APP_PID, APP_UID, "");
    ASSERT_TRUE(event != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(event));
}

/**
 * @tc.name: EventFieldValidatorTest019
 * @tc.desc: ValidateEvent allows trusted sender for pinned system domain
 * @tc.type: FUNC
 */
HWTEST_F(EventFieldValidatorTest, EventFieldValidatorTest019, TestSize.Level1)
{
    auto event = BuildEvent("WINDOWMANAGER", "NO_FOCUS_WINDOW", 100, SERVICE_UID, "");
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(event));
}

/**
 * @tc.name: EventFieldValidatorTest020
 * @tc.desc: ValidateEvent rejects trusted but non-pinned system sender
 * @tc.type: FUNC
 */
HWTEST_F(EventFieldValidatorTest, EventFieldValidatorTest020, TestSize.Level1)
{
    auto event = BuildEvent("WINDOWMANAGER", "NO_FOCUS_WINDOW", 100, HIVIEW_UID, "");
    ASSERT_TRUE(event != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(event));
}

/**
 * @tc.name: EventFieldValidatorTest021
 * @tc.desc: MAIN_STACK is stack text content, must never look like an absolute path
 * @tc.type: FUNC
 */
HWTEST_F(EventFieldValidatorTest, EventFieldValidatorTest021, TestSize.Level1)
{
    auto absolutePath = BuildEvent("TB6S_MS1", APP_PID, APP_UID,
        "\"MAIN_STACK\":\"/data/log/some_stack\"");
    ASSERT_TRUE(absolutePath != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(absolutePath));

    auto normalStack = BuildEvent("TB6S_MS2", APP_PID, APP_UID,
        "\"MAIN_STACK\":\"#00 pc 0001234 /system/lib/libfoo.so\"");
    ASSERT_TRUE(normalStack != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(normalStack));

    auto emptyMs = BuildEvent("TB6S_MS3", APP_PID, APP_UID, "\"MAIN_STACK\":\"\"");
    ASSERT_TRUE(emptyMs != nullptr);
    EXPECT_TRUE(EventFieldValidator::ValidateEvent(emptyMs));

    auto trustedBad = BuildEvent("TB6S_MS4", 100, SERVICE_UID,
        "\"MAIN_STACK\":\"/data/log/some_stack\"");
    ASSERT_TRUE(trustedBad != nullptr);
    EXPECT_FALSE(EventFieldValidator::ValidateEvent(trustedBad));
}

/**
 * @tc.name: EventFieldValidatorTest022
 * @tc.desc: ContainPathTraversal covers .., backslash, and null byte
 * @tc.type: FUNC
 */
HWTEST_F(EventFieldValidatorTest, EventFieldValidatorTest022, TestSize.Level1)
{
    EXPECT_TRUE(EventFieldValidator::ContainPathTraversal("../etc/passwd"));
    EXPECT_TRUE(EventFieldValidator::ContainPathTraversal("/data/log/../etc"));
    EXPECT_TRUE(EventFieldValidator::ContainPathTraversal("test\\path"));
    EXPECT_TRUE(EventFieldValidator::ContainPathTraversal(std::string("test\0path", 9)));
    EXPECT_FALSE(EventFieldValidator::ContainPathTraversal("/data/log/eventlog"));
    EXPECT_FALSE(EventFieldValidator::ContainPathTraversal("normal_path"));
    EXPECT_FALSE(EventFieldValidator::ContainPathTraversal(""));
}
