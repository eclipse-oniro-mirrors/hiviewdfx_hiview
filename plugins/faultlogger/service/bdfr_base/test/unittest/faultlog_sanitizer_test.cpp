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

#include "faultlog_sanitizer.h"
#include "file_util.h"
#include "test_utils.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
/**
 * @tc.name: FaultLogSanitizer001
 * @tc.desc: Test cjError ReportToAppEvent Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer001, testing::ext::TestSize.Level3)
{
    std::string summmay = "adaf";
    SysEventCreator sysEventCreator("CJ_RUNTIME", "CJERROR", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("SUMMARY", summmay);
    sysEventCreator.SetKeyValue("name_", "ADDR_SANITIZER");
    sysEventCreator.SetKeyValue("happenTime_", 1670248360359); // 1670248360359 : Simulate happenTime_ value
    sysEventCreator.SetKeyValue("REASON", "std.core:Exception");
    sysEventCreator.SetKeyValue("tz_", "+0800");
    sysEventCreator.SetKeyValue("pid_", 2413); // 2413 : Simulate pid_ value
    sysEventCreator.SetKeyValue("tid_", 2413); // 2413 : Simulate tid_ value
    sysEventCreator.SetKeyValue("what_", 3); // 3 : Simulate what_ value
    sysEventCreator.SetKeyValue("PACKAGE_NAME", "com.ohos.systemui");
    sysEventCreator.SetKeyValue("VERSION", "1.0.0");
    sysEventCreator.SetKeyValue("TYPE", 3); // 3 : Simulate TYPE value
    sysEventCreator.SetKeyValue("VERSION", "1.0.0");

    auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    FaultLogSanitizer san;
    san.info_.reportToAppEvent = false;
    bool ret = san.ReportToAppEvent(sysEvent);
    EXPECT_EQ(ret, false);

    sysEventCreator.SetKeyValue("LOG_PATH", "1.0.0");
    sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    san.info_.reportToAppEvent = true;
    ret = san.ReportToAppEvent(sysEvent);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: FaultLogSanitizer002
 * @tc.desc: Test ParseSanitizerEasyEvent Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogSanitizerTest, FaultLogSanitizer002, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    auto runTest = [&sanitizer](const std::string& input,
        const std::unordered_map<std::string, std::string>& expected) {
        SysEventCreator sysEventCreator("RELIABILITY", "ADDR_SANITIZER", SysEventCreator::FAULT);
        sysEventCreator.SetKeyValue("DATA", input);

        auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
        sanitizer.ParseSanitizerEasyEvent(*sysEvent);

        for (const auto& [key, val] : expected) {
            EXPECT_EQ(sysEvent->GetEventValue(key), val);
        }
    };
    runTest("FAULT_TYPE:8;MODULE:debugsanitizer;SUMMARY:debug text with ; and :",
            {{"FAULT_TYPE", "8"},
             {"MODULE", "debugsanitizer"},
             {"SUMMARY", "debug text with ; and :"}});
    runTest("FAULT_TYPE;MODULE:debugsanitizer:2;SUMMARY:debug text with ; and :",
            {{"FAULT_TYPE", ""},
             {"MODULE", "debugsanitizer:2"},
             {"SUMMARY", "debug text with ; and :"}});
    runTest("SUMMARY:only summary",
            {{"SUMMARY", "only summary"}});
    runTest("FAULT_TYPE:;SUMMARY:only summary",
            {{"FAULT_TYPE", ""},
             {"SUMMARY", "only summary"}});
}

} // namespace HiviewDFX
} // namespace OHOS
