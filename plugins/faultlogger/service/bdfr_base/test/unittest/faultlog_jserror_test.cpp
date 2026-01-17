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
#include <fstream>
#include <gtest/gtest.h>

#include "faultlog_jserror.h"
#include "file_util.h"
#include "sys_event.h"
#include "test_utils.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
/**
 * @tc.name: genjserrorLogTest001
 * @tc.desc: create JS ERROR event and send it to faultlogger
 * @tc.type: FUNC
 */
HWTEST(FaultLogJsErrorTest, genjserrorLogTest001, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create a jss_error event and pass it to faultlogger
     * @tc.expected: the calling is success and the file has been created
     */
    SysEventCreator sysEventCreator("AAFWK", "JSERROR", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("SUMMARY", "Error message:is not callable\nStacktrace:");
    sysEventCreator.SetKeyValue("name_", "JS_ERROR");
    sysEventCreator.SetKeyValue("happenTime_", 1670248360359);
    sysEventCreator.SetKeyValue("REASON", "TypeError");
    sysEventCreator.SetKeyValue("tz_", "+0800");
    sysEventCreator.SetKeyValue("pid_", 2413);
    sysEventCreator.SetKeyValue("tid_", 2413);
    sysEventCreator.SetKeyValue("what_", 3);
    sysEventCreator.SetKeyValue("PACKAGE_NAME", "com.ohos.systemui");
    sysEventCreator.SetKeyValue("VERSION", "1.0.0");
    sysEventCreator.SetKeyValue("TYPE", 3);
    sysEventCreator.SetKeyValue("VERSION", "1.0.0");

    auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    std::shared_ptr<Event> event = std::dynamic_pointer_cast<Event>(sysEvent);
    FaultLogJsError jserror;
    auto result = jserror.AddFaultLog(event);
    ASSERT_EQ(result, true);
    auto ret = remove("/data/test_jsError_info");
    if (ret < 0) {
        GTEST_LOG_(INFO) << "remove /data/test_jsError_info failed";
    }
}

/**
 * @tc.name: FaultLogJsError001
 * @tc.desc: Test jsError ReportToAppEvent Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogJsErrorTest, FaultLogJsError001, testing::ext::TestSize.Level3)
{
    FaultLogJsError jserror;
    jserror.info_.reportToAppEvent = false;
    bool ret = jserror.ReportToAppEvent(nullptr);
    EXPECT_EQ(ret, false);
}

static void ConstructJsErrorAppEvent(std::string summmay)
{
    SysEventCreator sysEventCreator("AAFWK", "JSERROR", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("SUMMARY", summmay);
    sysEventCreator.SetKeyValue("name_", "JS_ERROR");
    sysEventCreator.SetKeyValue("happenTime_", 1670248360359); // 1670248360359 : Simulate happenTime_ value
    sysEventCreator.SetKeyValue("REASON", "TypeError");
    sysEventCreator.SetKeyValue("tz_", "+0800");
    sysEventCreator.SetKeyValue("pid_", 2413); // 2413 : Simulate pid_ value
    sysEventCreator.SetKeyValue("tid_", 2413); // 2413 : Simulate tid_ value
    sysEventCreator.SetKeyValue("what_", 3); // 3 : Simulate what_ value
    sysEventCreator.SetKeyValue("PACKAGE_NAME", "com.ohos.systemui");
    sysEventCreator.SetKeyValue("VERSION", "1.0.0");
    sysEventCreator.SetKeyValue("TYPE", 3); // 3 : Simulate TYPE value
    sysEventCreator.SetKeyValue("VERSION", "1.0.0");
    sysEventCreator.SetKeyValue("PROCESS_LIFETIME", "1s");

    auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    std::shared_ptr<Event> event = std::dynamic_pointer_cast<Event>(sysEvent);
    FaultLogJsError jsError;
    auto result = jsError.AddFaultLog(event);
    ASSERT_EQ(result, true);
}

static void CheckKeyWordsInJsErrorAppEventFile(std::string name)
{
    std::string keywords[] = {
        "\"bundle_name\":", "\"bundle_version\":", "\"crash_type\":", "\"exception\":",
        "\"foreground\":", "\"hilog\":", "\"pid\":", "\"time\":", "\"uid\":", "\"uuid\":",
        "\"name\":", "\"message\":", "\"stack\":", "\"process_life_time\":", "\"release_type\":",
        "\"cpu_abi\":"
    };
    int length = sizeof(keywords) / sizeof(keywords[0]);
    std::string oldFileName = "/data/test_jsError_info";
    int count = CheckKeyWordsInFile(oldFileName, keywords, length, true);
    ASSERT_EQ(count, length) << "ReportJsErrorToAppEventTest001-" + name + " check keywords failed";
    if (FileUtil::FileExists(oldFileName)) {
        std::string newFileName = oldFileName + "_" + name;
        rename(oldFileName.c_str(), newFileName.c_str());
    }
    auto ret = remove("/data/test_jsError_info");
    if (ret == 0) {
        GTEST_LOG_(INFO) << "remove /data/test_jsError_info failed";
    }
}

/**
 * @tc.name: ReportJsErrorToAppEventTest001
 * @tc.desc: create JS ERROR event and send it to hiappevent
 * @tc.type: FUNC
 */
HWTEST(FaultLogJsErrorTest, ReportJsErrorToAppEventTest001, testing::ext::TestSize.Level3)
{
    // has Error name、Error message、Error code、SourceCode、Stacktrace
    std::string summaryHasAll = R"~(Error name:summaryHasAll TypeError
Error message:Obj is not a Valid object
Error code:get BLO
SourceCode:CKSSvalue() {new Error("TestError");}
Stacktrace:
    at anonymous(entry/src/main/ets/pages/index.ets:76:10)
    at anonymous2(entry/src/main/ets/pages/index.ets:76:10)
    at anonymous3(entry/src/main/ets/pages/index.ets:76:10)
)~";
    ConstructJsErrorAppEvent(summaryHasAll);
    CheckKeyWordsInJsErrorAppEventFile("summaryHasAll");
}

static void CheckDeleteStackErrorMessage(std::string name)
{
    std::string keywords[] = {"\"Cannot get SourceMap info, dump raw stack:"};
    int length = sizeof(keywords) / sizeof(keywords[0]);
    std::string oldFileName = "/data/test_jsError_info";
    int count = CheckKeyWordsInFile(oldFileName, keywords, length, true);
    ASSERT_NE(count, length) << "check delete stack error message failed";
}

/**
 * @tc.name: ReportJsErrorToAppEventTest002
 * @tc.desc: create JS ERROR event and send it to hiappevent
 * @tc.type: FUNC
 */
HWTEST(FaultLogJsErrorTest, ReportJsErrorToAppEventTest002, testing::ext::TestSize.Level3)
{
    // has Error name、Error message、Error code、SourceCode、Stacktrace
    std::string summaryNotFindSourcemap = R"~(Error name:summaryNotFindSourcemap Error
Error message:BussinessError 2501000: Operation failed.
Error code:2501000
Stacktrace:
Cannot get SourceMap info, dump raw stack:
  at anonymous(entry/src/main/ets/pages/index.ets:76:10)
  at anonymous2(entry/src/main/ets/pages/index.ets:76:10)
  at anonymous3(entry/src/main/ets/pages/index.ets:76:10)
)~";
    ConstructJsErrorAppEvent(summaryNotFindSourcemap);
    CheckDeleteStackErrorMessage("summaryNotFindSourcemap");
    CheckKeyWordsInJsErrorAppEventFile("summaryNotFindSourcemap");
}

/**
 * @tc.name: ReportJsErrorToAppEventTest003
 * @tc.desc: create JS ERROR event and send it to hiappevent
 * @tc.type: FUNC
 */
HWTEST(FaultLogJsErrorTest, ReportJsErrorToAppEventTest003, testing::ext::TestSize.Level3)
{
    // has Error name、Error message、SourceCode、Stacktrace
    std::string summaryHasNoErrorCode = R"~(Error name:summaryHasNoErrorCode TypeError
Error message:Obj is not a Valid object
SourceCode:CKSSvalue() {new Error("TestError");}
Stacktrace:
    at anonymous(entry/src/main/ets/pages/index.ets:76:10)
    at anonymous2(entry/src/main/ets/pages/index.ets:76:10)
    at anonymous3(entry/src/main/ets/pages/index.ets:76:10)
)~";
    ConstructJsErrorAppEvent(summaryHasNoErrorCode);
    CheckKeyWordsInJsErrorAppEventFile("summaryHasNoErrorCode");
}

/**
 * @tc.name: ReportJsErrorToAppEventTest004
 * @tc.desc: create JS ERROR event and send it to hiappevent
 * @tc.type: FUNC
 */
HWTEST(FaultLogJsErrorTest, ReportJsErrorToAppEventTest004, testing::ext::TestSize.Level3)
{
    // has Error name、Error message、Error code、Stacktrace
    std::string summaryHasNoSourceCode = R"~(Error name:summaryHasNoSourceCode TypeError
Error message:Obj is not a Valid object
Error code:get BLO
Stacktrace:
    at anonymous(entry/src/main/ets/pages/index.ets:76:10)
    at anonymous2(entry/src/main/ets/pages/index.ets:76:10)
    at anonymous3(entry/src/main/ets/pages/index.ets:76:10)
)~";
    ConstructJsErrorAppEvent(summaryHasNoSourceCode);
    CheckKeyWordsInJsErrorAppEventFile("summaryHasNoSourceCode");
}

/**
 * @tc.name: ReportJsErrorToAppEventTest005
 * @tc.desc: create JS ERROR event and send it to hiappevent
 * @tc.type: FUNC
 */
HWTEST(FaultLogJsErrorTest, ReportJsErrorToAppEventTest005, testing::ext::TestSize.Level3)
{
    // has Error name、Error message、Stacktrace
    std::string summaryHasNoErrorCodeAndSourceCode = R"~(Error name:summaryHasNoErrorCodeAndSourceCode TypeError
Error message:Obj is not a Valid object
Stacktrace:
    at anonymous(entry/src/main/ets/pages/index.ets:76:10)
    at anonymous2(entry/src/main/ets/pages/index.ets:76:10)
    at anonymous3(entry/src/main/ets/pages/index.ets:76:10)
)~";
    ConstructJsErrorAppEvent(summaryHasNoErrorCodeAndSourceCode);
    CheckKeyWordsInJsErrorAppEventFile("summaryHasNoErrorCodeAndSourceCode");
}

/**
 * @tc.name: ReportJsErrorToAppEventTest006
 * @tc.desc: create JS ERROR event and send it to hiappevent
 * @tc.type: FUNC
 */
HWTEST(FaultLogJsErrorTest, ReportJsErrorToAppEventTest006, testing::ext::TestSize.Level3)
{
    // has Error name、Error message、Error code、SourceCode
    std::string summaryHasNoStacktrace = R"~(Error name:summaryHasNoStacktrace TypeError
Error message:Obj is not a Valid object
Error code:get BLO
SourceCode:CKSSvalue() {new Error("TestError");}
Stacktrace:
)~";
    ConstructJsErrorAppEvent(summaryHasNoStacktrace);
    CheckKeyWordsInJsErrorAppEventFile("summaryHasNoStacktrace");
}

/**
 * @tc.name: ReportJsErrorToAppEventTest007
 * @tc.desc: create JS ERROR event and send it to hiappevent
 * @tc.type: FUNC
 */
HWTEST(FaultLogJsErrorTest, ReportJsErrorToAppEventTest007, testing::ext::TestSize.Level3)
{
    // has Error name、Error message
    std::string summaryHasErrorNameAndErrorMessage = R"~(Error name:summaryHasErrorNameAndErrorMessage TypeError
Error message:Obj is not a Valid object
Stacktrace:
)~";
    ConstructJsErrorAppEvent(summaryHasErrorNameAndErrorMessage);
    CheckKeyWordsInJsErrorAppEventFile("summaryHasErrorNameAndErrorMessage");
}

static void ConstructJsErrorAppEventWithNoValue(std::string summmay)
{
    SysEventCreator sysEventCreator("AAFWK", "JSERROR", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("SUMMARY", summmay);
    sysEventCreator.SetKeyValue("name_", "JS_ERROR");
    sysEventCreator.SetKeyValue("happenTime_", 1670248360359); // 1670248360359 : Simulate happenTime_ value
    sysEventCreator.SetKeyValue("TYPE", 3); // 3 : Simulate TYPE value
    sysEventCreator.SetKeyValue("VERSION", "1.0.0");
    sysEventCreator.SetKeyValue("PROCESS_LIFETIME", "1s");

    auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    std::shared_ptr<Event> event = std::dynamic_pointer_cast<Event>(sysEvent);
    FaultLogJsError jsError;
    auto result = jsError.AddFaultLog(event);
    ASSERT_EQ(result, true);
}

/**
 * @tc.name: ReportJsErrorToAppEventTest008
 * @tc.desc: create JS ERROR event and send it to hiappevent
 * @tc.type: FUNC
 */
HWTEST(FaultLogJsErrorTest, ReportJsErrorToAppEventTest008, testing::ext::TestSize.Level3)
{
    // has Error name、Error message
    std::string noKeyValue = R"~(Error name:summaryHasErrorNameAndErrorMessage TypeError
Error message:Obj is not a Valid object
Stacktrace:
)~";
    ConstructJsErrorAppEventWithNoValue(noKeyValue);
    CheckKeyWordsInJsErrorAppEventFile("noKeyValue");
}

/**
 * @tc.name: ReportJsErrorToAppEventTest009
 * @tc.desc: create JS ERROR event and send it to hiappevent
 * @tc.type: FUNC
 */
HWTEST(FaultLogJsErrorTest, ReportJsErrorToAppEventTest009, testing::ext::TestSize.Level3)
{
    ConstructJsErrorAppEventWithNoValue("");
    std::string oldFileName = "/data/test_jsError_info";
    ASSERT_TRUE(FileUtil::FileExists(oldFileName));
    auto ret = remove("/data/test_jsError_info");
    if (ret != 0) {
        GTEST_LOG_(INFO) << "remove /data/test_jsError_info failed";
    }
}
} // namespace HiviewDFX
} // namespace OHOS
