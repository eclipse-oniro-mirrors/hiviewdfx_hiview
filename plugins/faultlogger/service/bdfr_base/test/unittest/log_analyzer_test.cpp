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

#include "common_defines.h"
#include "constants.h"
#include "file_util.h"
#include "log_analyzer.h"
#include "string_util.h"
#include "tbox.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {

/**
 * @tc.name: AnalysisFaultlogTest001
 * @tc.desc: create cpp crash event and check AnalysisFaultlog
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, AnalysisFaultlogTest001, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create a cpp crash event and pass it to faultlogger
     * @tc.expected: AnalysisFaultlog return expected result
     */
    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 0;
    info.pid = 7497;
    info.faultLogType = FaultLogType::CPP_CRASH;
    info.module = "com.example.testapplication";
    info.reason = "TestReason";
    std::map<std::string, std::string> eventInfos;
    ASSERT_EQ(AnalysisFaultlog(info, eventInfos), false);
    ASSERT_EQ(!eventInfos["FINGERPRINT"].empty(), true);
}

/**
 * @tc.name: AnalysisFaultlogTest002
 * @tc.desc: create Js crash FaultLogInfo and check AnalysisFaultlog
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, AnalysisFaultlogTest002, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create Js crash FaultLogInfo
     * @tc.expected: AnalysisFaultlog return expected result
     */
    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 0;
    info.pid = 7497;
    info.faultLogType = FaultLogType::JS_CRASH;
    info.module = "com.example.testapplication";
    info.reason = "TestReason";
    std::map<std::string, std::string> eventInfos;
    ASSERT_EQ(AnalysisFaultlog(info, eventInfos), false);
    ASSERT_EQ(!eventInfos["FINGERPRINT"].empty(), true);
}

/**
 * @tc.name: AnalysisFaultlogTestAppFreezeSummary
 * @tc.desc: verify the real AppFreeze fixture uses the producer summary before fingerprinting
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, AnalysisFaultlogTestAppFreezeSummary, testing::ext::TestSize.Level2)
{
    constexpr char faultLogPath[] =
        "/data/test/test_data/SmartParser/test_faultlogger_data/AppFreezeCrashLogTest001/"
        "appfreeze-com.example.jsinject-20010039-19700326211815.tmp";
    constexpr char summaryPath[] = "/data/test/req1-appfreeze-summary-analysis.txt";
    const std::string summary =
        "---AppFreezeHeaviestStack Begin---\n"
        "version:1\n"
        "status:success\n"
        "total_samples:3\n"
        "busiest_count:2\n"
        "busiest_ratio_permille:666\n"
        "first_snapshot_time:100\n"
        "stack_id:7\n"
        "frame_count:3\n"
        "frame_0:summary-first\n"
        "frame_1:summary-second\n"
        "frame_2:summary-last\n"
        "---AppFreezeHeaviestStack End---\n";
    ASSERT_TRUE(FileUtil::FileExists(faultLogPath));
    ASSERT_TRUE(FileUtil::SaveStringToFile(summaryPath, summary));

    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 20010039;
    info.pid = 7497;
    info.faultLogType = FaultLogType::APP_FREEZE;
    info.module = "com.example.jsinject";
    info.reason = "UI_BLOCK_6S@summary";
    info.logPath = faultLogPath;
    info.sectionMap[FaultKey::FREEZE_INFO_PATH] = summaryPath;
    info.sectionMap[FaultKey::SAMPLER_COUNT] = "3";

    std::map<std::string, std::string> eventInfos;
    ASSERT_TRUE(AnalysisFaultlog(info, eventInfos));
    EXPECT_EQ(eventInfos[FaultKey::SAMPLER_COUNT], "3");
    EXPECT_EQ(eventInfos[FaultKey::BUSIEST_STACK_COUNT], "2");
    EXPECT_EQ(eventInfos[FaultKey::BUSIEST_STACK_RATIO_PERMILLE], "666");
    EXPECT_EQ(eventInfos[FaultKey::STACK_SOURCE], "HEAVIEST_SAMPLE");
    EXPECT_EQ(eventInfos[FaultKey::HAS_MAIN_THREAD_STACK], "true");
    EXPECT_EQ(eventInfos[FaultKey::HEAVIEST_STACK_STATUS], "success");
    EXPECT_EQ(eventInfos[FaultKey::LOG_VALIDITY], "FULL");
    EXPECT_TRUE(eventInfos[FaultKey::LOG_INVALID_REASON].empty());
    EXPECT_EQ(eventInfos[FaultKey::FIRST_FRAME], "summary-first");
    EXPECT_EQ(eventInfos[FaultKey::SECOND_FRAME], "summary-second");
    EXPECT_EQ(eventInfos[FaultKey::LAST_FRAME], "summary-last");

    const std::string expectedRaw = info.module + StringUtil::GetLeftSubstr(info.reason, "@") +
        "summary-firstsummary-secondsummary-last";
    EXPECT_EQ(eventInfos[FaultKey::FINGERPRINT], Tbox::CalcFingerPrint(expectedRaw, 0, FP_BUFFER));
    unlink(summaryPath);
}

HWTEST(FaultloggerUtilsUnittest, AnalysisFaultlogTestAppFreezeSummaryRepeatability,
    testing::ext::TestSize.Level2)
{
    constexpr char faultLogPath[] =
        "/data/test/test_data/SmartParser/test_faultlogger_data/AppFreezeCrashLogTest001/"
        "appfreeze-com.example.jsinject-20010039-19700326211815.tmp";
    constexpr char summaryPath[] = "/data/test/req1-appfreeze-summary-repeatability.txt";
    const std::string summary =
        "---AppFreezeHeaviestStack Begin---\n"
        "version:1\n"
        "status:success\n"
        "total_samples:3\n"
        "busiest_count:2\n"
        "busiest_ratio_permille:666\n"
        "first_snapshot_time:100\n"
        "stack_id:7\n"
        "frame_count:3\n"
        "frame_0:summary-first\n"
        "frame_1:summary-second\n"
        "frame_2:summary-last\n"
        "---AppFreezeHeaviestStack End---\n";
    ASSERT_TRUE(FileUtil::FileExists(faultLogPath));
    ASSERT_TRUE(FileUtil::SaveStringToFile(summaryPath, summary));

    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 20010039;
    info.pid = 7497;
    info.faultLogType = FaultLogType::APP_FREEZE;
    info.module = "com.example.jsinject";
    info.reason = "UI_BLOCK_6S@summary";
    info.logPath = faultLogPath;
    info.sectionMap[FaultKey::FREEZE_INFO_PATH] = summaryPath;
    info.sectionMap[FaultKey::SAMPLER_COUNT] = "3";

    std::map<std::string, std::string> baseline;
    ASSERT_TRUE(AnalysisFaultlog(info, baseline));
    constexpr int runs = 1000;
    int mismatches = 0;
    for (int i = 0; i < runs; ++i) {
        std::map<std::string, std::string> current;
        ASSERT_TRUE(AnalysisFaultlog(info, current));
        if (current != baseline) {
            ++mismatches;
        }
    }
    RecordProperty("runs", runs);
    RecordProperty("mismatches", mismatches);
    EXPECT_EQ(mismatches, 0);
    unlink(summaryPath);
}
} // namespace HiviewDFX
} // namespace OHOS
