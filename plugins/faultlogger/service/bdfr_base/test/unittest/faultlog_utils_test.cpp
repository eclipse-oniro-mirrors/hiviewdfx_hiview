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
#include <gtest/gtest.h>

#include <unistd.h>
#include <sys/syscall.h>

#include "file_util.h"
#include "faultlog_util.h"
#include "hiview_logger.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "FaultloggerUT");

/**
 * @tc.name: GetFaultNameByTypeTest001
 * @tc.desc: test GetFaultNameByType
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, GetFaultNameByTypeTest001, testing::ext::TestSize.Level3)
{
    ASSERT_EQ(GetFaultNameByType(FaultLogType::JS_CRASH, true), "jscrash");
    ASSERT_EQ(GetFaultNameByType(FaultLogType::JS_CRASH, false), "JS_ERROR");

    ASSERT_EQ(GetFaultNameByType(FaultLogType::CPP_CRASH, true), "cppcrash");
    ASSERT_EQ(GetFaultNameByType(FaultLogType::CPP_CRASH, false), "CPP_CRASH");

    ASSERT_EQ(GetFaultNameByType(FaultLogType::APP_FREEZE, true), "appfreeze");
    ASSERT_EQ(GetFaultNameByType(FaultLogType::APP_FREEZE, false), "APP_FREEZE");

    ASSERT_EQ(GetFaultNameByType(FaultLogType::SYS_FREEZE, true), "sysfreeze");
    ASSERT_EQ(GetFaultNameByType(FaultLogType::SYS_FREEZE, false), "SYS_FREEZE");

    ASSERT_EQ(GetFaultNameByType(FaultLogType::SYS_WARNING, true), "syswarning");
    ASSERT_EQ(GetFaultNameByType(FaultLogType::SYS_WARNING, false), "SYS_WARNING");

    ASSERT_EQ(GetFaultNameByType(FaultLogType::APPFREEZE_WARNING, true), "appfreezewarning");
    ASSERT_EQ(GetFaultNameByType(FaultLogType::APPFREEZE_WARNING, false), "APPFREEZE_WARNING");

    ASSERT_EQ(GetFaultNameByType(FaultLogType::RUST_PANIC, true), "rustpanic");
    ASSERT_EQ(GetFaultNameByType(FaultLogType::RUST_PANIC, false), "RUST_PANIC");

    ASSERT_EQ(GetFaultNameByType(FaultLogType::ADDR_SANITIZER, true), "sanitizer");
    ASSERT_EQ(GetFaultNameByType(FaultLogType::ADDR_SANITIZER, false), "ADDR_SANITIZER");

    ASSERT_EQ(GetFaultNameByType(FaultLogType::CJ_ERROR, true), "cjerror");
    ASSERT_EQ(GetFaultNameByType(FaultLogType::CJ_ERROR, false), "CJ_ERROR");

    ASSERT_EQ(GetFaultNameByType(FaultLogType::ALL, true), "Unknown");
    ASSERT_EQ(GetFaultNameByType(FaultLogType::ALL, false), "Unknown");
}

/**
 * @tc.name: GetFaultNameByTypeTest002
 * @tc.desc: test GetFaultNameByType
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, GetFaultNameByTypeTest002, testing::ext::TestSize.Level3)
{
    std::string result = GetFaultNameByType(FaultLogType::SYS_FREEZE, false);
    ASSERT_EQ(result, "SYS_FREEZE");
    result = GetFaultNameByType(FaultLogType::SYS_WARNING, false);
    ASSERT_EQ(result, "SYS_WARNING");
    result = GetFaultNameByType(FaultLogType::APPFREEZE_WARNING, false);
    ASSERT_EQ(result, "APPFREEZE_WARNING");
    result = GetFaultNameByType(FaultLogType::CJ_ERROR, false);
    ASSERT_EQ(result, "CJ_ERROR");
}

/**
 * @tc.name: GetFaultLogName001
 * @tc.desc: Test calling GetFaultLogName Func
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, GetFaultLogName001, testing::ext::TestSize.Level3)
{
    FaultLogInfo info {
        .time = 1607161345,
        .id = 0,
        .faultLogType = FaultLogType::ADDR_SANITIZER,
        .module = "FaultloggerUnittest"
    };
    info.sanitizerType = "ASAN";
    std::string fileName = GetFaultLogName(info);
    ASSERT_EQ(fileName, "asan-FaultloggerUnittest-0-20201205174225345.log");
    info.sanitizerType = "HWASAN";
    fileName = GetFaultLogName(info);
    ASSERT_EQ(fileName, "hwasan-FaultloggerUnittest-0-20201205174225345.log");
}

/**
 * @tc.name: GetFaultLogName002
 * @tc.desc: test GetFaultLogName
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, GetFaultLogName002, testing::ext::TestSize.Level3)
{
    std::string result = GetFaultNameByType(FaultLogType::ADDR_SANITIZER, false);
    ASSERT_EQ(result, "ADDR_SANITIZER");

    FaultLogInfo info;
    info.module = "test/test";
    info.faultLogType = FaultLogType::ADDR_SANITIZER;
    info.sanitizerType = "TSAN";
    std::string str = GetFaultLogName(info);
    ASSERT_EQ(str, "tsan-test-0-19700101080000000.log");
    info.sanitizerType = "UBSAN";
    str = GetFaultLogName(info);
    ASSERT_EQ(str, "ubsan-test-0-19700101080000000.log");
    info.sanitizerType = "GWP-ASAN";
    str = GetFaultLogName(info);
    ASSERT_EQ(str, "gwpasan-test-0-19700101080000000.log");
    info.sanitizerType = "HWASAN";
    str = GetFaultLogName(info);
    ASSERT_EQ(str, "hwasan-test-0-19700101080000000.log");
    info.sanitizerType = "ASAN";
    str = GetFaultLogName(info);
    ASSERT_EQ(str, "asan-test-0-19700101080000000.log");
    info.sanitizerType = "GWP-ASANS";
    str = GetFaultLogName(info);
    ASSERT_EQ(str, "sanitizer-test-0-19700101080000000.log");

    str = RegulateModuleNameIfNeed("");
    ASSERT_EQ(str, "");
}

/**
 * @tc.name: GetLogTypeByNameTest001
 * @tc.desc: Test calling GetLogTypeByName Func
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, GetLogTypeByNameTest001, testing::ext::TestSize.Level3)
{
    ASSERT_EQ(GetLogTypeByName("jscrash"), FaultLogType::JS_CRASH);
    ASSERT_EQ(GetLogTypeByName("cppcrash"), FaultLogType::CPP_CRASH);
    ASSERT_EQ(GetLogTypeByName("appfreeze"), FaultLogType::APP_FREEZE);
    ASSERT_EQ(GetLogTypeByName("sysfreeze"), FaultLogType::SYS_FREEZE);
    ASSERT_EQ(GetLogTypeByName("syswarning"), FaultLogType::SYS_WARNING);
    ASSERT_EQ(GetLogTypeByName("appfreezewarning"), FaultLogType::APPFREEZE_WARNING);
    ASSERT_EQ(GetLogTypeByName("sanitizer"), FaultLogType::ADDR_SANITIZER);
    ASSERT_EQ(GetLogTypeByName("cjerror"), FaultLogType::CJ_ERROR);
    ASSERT_EQ(GetLogTypeByName("all"), FaultLogType::ALL);
    ASSERT_EQ(GetLogTypeByName("ALL"), FaultLogType::ALL);
    ASSERT_EQ(GetLogTypeByName("Unknown"), -1);
}

/**
 * @tc.name: ExtractInfoFromFileNameTest001
 * @tc.desc: check ExtractInfoFromFileName Func
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, ExtractInfoFromFileNameTest001, testing::ext::TestSize.Level3)
{
    std::string filename = "appfreeze-com.ohos.systemui-10006-20170805172159";
    auto info = ExtractInfoFromFileName(filename);
    ASSERT_EQ(info.pid, 0);
    ASSERT_EQ(info.faultLogType, FaultLogType::APP_FREEZE); // 4 : APP_FREEZE
    ASSERT_EQ(info.module, "com.ohos.systemui");
    ASSERT_EQ(info.id, 10006); // 10006 : test uid
}

/**
 * @tc.name: ExtractInfoFromFileNameTest002
 * @tc.desc: check ExtractInfoFromFileName Func
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, ExtractInfoFromFileNameTest002, testing::ext::TestSize.Level3)
{
    std::string filename = "appfreeze";
    auto info = ExtractInfoFromFileName(filename);
    ASSERT_EQ(info.pid, 0);
    ASSERT_EQ(info.time, 0);
}

/**
 * @tc.name: ExtractInfoFromFileNameTest003
 * @tc.desc: check ExtractInfoFromFileName Func
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, ExtractInfoFromFileNameTest003, testing::ext::TestSize.Level3)
{
    std::string filename = "appfreezewarning-com.ohos.systemui-10006-20170805172159";
    auto info = ExtractInfoFromFileName(filename);
    ASSERT_EQ(info.pid, 0);
    ASSERT_EQ(info.faultLogType, FaultLogType::APPFREEZE_WARNING); // 7 : APPFREEZE_WARNING
    ASSERT_EQ(info.module, "com.ohos.systemui");
    ASSERT_EQ(info.id, 10006); // 10006 : test uid
}

/**
 * @tc.name: ExtractInfoFromTempFileTest001
 * @tc.desc: check ExtractInfoFromTempFile Func
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, ExtractInfoFromTempFileTest001, testing::ext::TestSize.Level3)
{
    std::string filename = "appfreeze-10006-20170805172159";
    auto info = ExtractInfoFromTempFile(filename);
    ASSERT_EQ(info.faultLogType, FaultLogType::APP_FREEZE); // 4 : APP_FREEZE
    ASSERT_EQ(info.pid, 10006); // 10006 : test uid

    std::string filename3 = "jscrash-10006-20170805172159";
    auto info3 = ExtractInfoFromTempFile(filename3);
    ASSERT_EQ(info3.faultLogType, FaultLogType::JS_CRASH); // 3 : JS_CRASH
    ASSERT_EQ(info3.pid, 10006); // 10006 : test uid

    std::string filename4 = "cppcrash-10006-20170805172159";
    auto info4 = ExtractInfoFromTempFile(filename4);
    ASSERT_EQ(info4.faultLogType, FaultLogType::CPP_CRASH); // 2 : CPP_CRASH
    ASSERT_EQ(info4.pid, 10006); // 10006 : test uid

    std::string filename5 = "all-10006-20170805172159";
    auto info5 = ExtractInfoFromTempFile(filename5);
    ASSERT_EQ(info5.faultLogType, FaultLogType::ALL); // 0 : ALL
    ASSERT_EQ(info5.pid, 10006); // 10006 : test uid

    std::string filename6 = "other-10006-20170805172159";
    auto info6 = ExtractInfoFromTempFile(filename6);
    ASSERT_EQ(info6.faultLogType, -1); // -1 : other
    ASSERT_EQ(info6.pid, 10006); // 10006 : test uid
}

/**
 * @tc.name: GetDebugSignalTempLogNameTest001
 * @tc.desc: Test calling GetDebugSignalTempLogName Func
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, GetDebugSignalTempLogNameTest001, testing::ext::TestSize.Level3)
{
    FaultLogInfo info {
        .time = 1607161345,
        .id = 0,
        .faultLogType = FaultLogType::ADDR_SANITIZER,
        .module = "FaultloggerUnittest"
    };
    string fileName = GetDebugSignalTempLogName(info);
    ASSERT_EQ(fileName, "/data/log/faultlog/temp/stacktrace-0-1607161345");
    fileName = GetSanitizerTempLogName(info.pid, std::to_string(info.time));
    ASSERT_EQ(fileName, "/data/log/faultlog/temp/sanitizer-0-1607161345");
    string str;
    ASSERT_EQ(GetThreadStack(str, 0), "");
}

/**
 * @tc.name: GetDebugSignalTempLogNameTest002
 * @tc.desc: test GetDebugSignalTempLogName
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, GetDebugSignalTempLogNameTest002, testing::ext::TestSize.Level3)
{
    FaultLogInfo info;
    info.pid = 123;
    info.time = 123456789;
    auto fileName = GetDebugSignalTempLogName(info);
    ASSERT_EQ(fileName, "/data/log/faultlog/temp/stacktrace-123-123456789");
}

/**
 * @tc.name: GetThreadStackTest001
 * @tc.desc: test GetThreadStack
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, GetThreadStackTest001, testing::ext::TestSize.Level3)
{
    std::string path;
    auto stack = GetThreadStack(path, 0);
    ASSERT_TRUE(stack.empty());

    path = "/data/log/faultlog/faultlogger/appfreeze-com.example.jsinject-20010039-19700326211815.tmp";
    const int thread1 = 3443;
    stack = GetThreadStack(path, thread1);
    ASSERT_FALSE(stack.empty());

    const int thread2 = 3444;
    stack = GetThreadStack(path, thread2);
    ASSERT_FALSE(stack.empty());
}

/**
 * @tc.name: ExtractSubMoudleNameTest001
 * @tc.desc: Test ExtractSubMoudleName func
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, ExtractSubMoudleNameTest001, testing::ext::TestSize.Level3)
{
    std::string moduleName = "com.ohos.sceneboard:";
    ASSERT_FALSE(ExtractSubMoudleName(moduleName));
    std::string endName = "";
    ASSERT_EQ(endName, moduleName);
}

/**
 * @tc.name: ExtractSubMoudleNameTest002
 * @tc.desc: Test ExtractSubMoudleName func
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, ExtractSubMoudleNameTest002, testing::ext::TestSize.Level3)
{
    std::string moduleName = "com.ohos.sceneboard:123ExtractSubMoudleName/123ExtractSubMoudleName_321";
    ASSERT_TRUE(ExtractSubMoudleName(moduleName));
    std::string endName = "ExtractSubMoudleName_123ExtractSubMoudleName";
    ASSERT_EQ(endName, moduleName);
}

/**
 * @tc.name: ExtractSubMoudleNameTest003
 * @tc.desc: Test ExtractSubMoudleName func
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, ExtractSubMoudleNameTest003, testing::ext::TestSize.Level3)
{
    std::string moduleName = "com.ohos.test";
    ASSERT_FALSE(ExtractSubMoudleName(moduleName));
    std::string moduleName2 = "com.ohos.sceneboard:test:1";
    ASSERT_TRUE(ExtractSubMoudleName(moduleName2));
    std::string endName = "test";
    ASSERT_EQ(endName, moduleName2);
}
} // namespace HiviewDFX
} // namespace OHOS
