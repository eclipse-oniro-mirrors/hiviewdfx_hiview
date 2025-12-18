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

#include "faultlog_cjerror.h"
#include "file_util.h"
#include "test_utils.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
/**
 * @tc.name: FaultLogCjError001
 * @tc.desc: Test cj reportToAppEvent Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogCjErrorTest, FaultLogCjError001, testing::ext::TestSize.Level3)
{
    FaultLogCjError cjError;
    cjError.info_.reportToAppEvent = false;
    bool ret = cjError.ReportToAppEvent(nullptr);
    EXPECT_EQ(ret, false);
}

static void ConstructCjErrorAppEvent(std::string summmay)
{
    SysEventCreator sysEventCreator("CJ_RUNTIME", "CJERROR", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("SUMMARY", summmay);
    sysEventCreator.SetKeyValue("name_", "CJ_ERROR");
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
    std::shared_ptr<Event> event = std::dynamic_pointer_cast<Event>(sysEvent);
    FaultLogCjError cjError;
    bool result = cjError.AddFaultLog(event);
    ASSERT_EQ(result, true);
}

static void CheckKeyWordsInCjErrorAppEventFile(std::string name)
{
    std::string keywords[] = {
        "\"bundle_name\":", "\"bundle_version\":", "\"crash_type\":", "\"exception\":",
        "\"foreground\":", "\"hilog\":", "\"pid\":", "\"time\":", "\"uid\":", "\"uuid\":",
        "\"name\":", "\"message\":", "\"stack\":"
    };
    int length = sizeof(keywords) / sizeof(keywords[0]);
    std::cout << "length:" << length << std::endl;
    std::string oldFileName = "/data/test_cjError_info";
    int count = CheckKeyWordsInFile(oldFileName, keywords, length, false);
    std::cout << "count:" << count << std::endl;
    ASSERT_EQ(count, length) << "ReportCjErrorToAppEventTest001-" + name + " check keywords failed";
    if (FileUtil::FileExists(oldFileName)) {
        std::string newFileName = oldFileName + "_" + name;
        rename(oldFileName.c_str(), newFileName.c_str());
    }
    auto ret = remove("/data/test_cjError_info");
    if (ret == 0) {
        GTEST_LOG_(INFO) << "remove /data/test_cjError_info failed";
    }
}

/**
 * @tc.name: ReportCjErrorToAppEventTest001
 * @tc.desc: create CJ ERROR event and send it to hiappevent
 * @tc.type: FUNC
 */
HWTEST(FaultLogCjErrorTest, ReportCjErrorToAppEventTest001, testing::ext::TestSize.Level3)
{
    // has Error name、Error message、Error code、SourceCode、Stacktrace
    std::string summary = R"~(Uncaught exception was found.
Exception info: throwing foo exception
Stacktrace:
    at anonymous(entry/src/main/ets/pages/index.cj:20)
    at anonymous2(entry/src/main/ets/pages/index.cj:33)
    at anonymous3(entry/src/main/ets/pages/index.cj:77)
)~";
    ConstructCjErrorAppEvent(summary);
    CheckKeyWordsInCjErrorAppEventFile("summary");
}
} // namespace HiviewDFX
} // namespace OHOS
