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
#include <algorithm>
#include <chrono>
#include <gtest/gtest.h>
#include <unistd.h>

#include "constants.h"
#include "file_util.h"
#include "freeze_stack_summary.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr char SUMMARY_PREFIX[] =
    "---AppFreezeHeaviestStack Begin---\n"
    "version:1\n"
    "status:success\n"
    "total_samples:3\n"
    "busiest_count:2\n"
    "busiest_ratio_permille:666\n"
    "first_snapshot_time:100\n"
    "stack_id:7\n";
constexpr char SUMMARY_SUFFIX[] = "---AppFreezeHeaviestStack End---\n";
}

HWTEST(FreezeStackSummaryTest, ParseThreeFrames, TestSize.Level1)
{
    FreezeStackSummary summary;
    std::string content = std::string(SUMMARY_PREFIX) +
        "frame_count:3\nframe_0:first\\nframe\nframe_1:second\\:frame\nframe_2:last\n" + SUMMARY_SUFFIX;
    ASSERT_TRUE(ParseFreezeStackSummary(content, summary));
    ASSERT_EQ(summary.totalSamples, 3);
    ASSERT_EQ(summary.busiestCount, 2);
    ASSERT_EQ(summary.busiestRatioPermille, 666);
    ASSERT_EQ(summary.frames.size(), 3);
    ASSERT_EQ(summary.frames[0], "first\nframe");
    ASSERT_EQ(summary.frames[1], "second:frame");
}

HWTEST(FreezeStackSummaryTest, RejectsDuplicateKeyAndTruncatedSegment, TestSize.Level1)
{
    FreezeStackSummary summary;
    std::string duplicate = std::string(SUMMARY_PREFIX) +
        "frame_count:1\nframe_count:1\nframe_0:first\n" + SUMMARY_SUFFIX;
    EXPECT_FALSE(ParseFreezeStackSummary(duplicate, summary));
    std::string truncated = std::string(SUMMARY_PREFIX) + "frame_count:1\nframe_0:first\n";
    EXPECT_FALSE(ParseFreezeStackSummary(truncated, summary));
}

HWTEST(FreezeStackSummaryTest, ProjectsOneAndTwoFramesFromSameStack, TestSize.Level1)
{
    const std::string one =
        "---AppFreezeHeaviestStack Begin---\nversion:1\nstatus:success\n"
        "total_samples:1\nbusiest_count:1\nbusiest_ratio_permille:1000\n"
        "first_snapshot_time:100\nstack_id:8\n"
        "frame_count:1\nframe_0:only\n" + std::string(SUMMARY_SUFFIX);
    FreezeStackSummary summary;
    ASSERT_TRUE(ParseFreezeStackSummary(one, summary));
    ASSERT_EQ(summary.frames.size(), 1);

    const std::string two =
        "---AppFreezeHeaviestStack Begin---\nversion:1\nstatus:success\n"
        "total_samples:2\nbusiest_count:2\nbusiest_ratio_permille:1000\n"
        "first_snapshot_time:100\nstack_id:9\n"
        "frame_count:2\nframe_0:first\nframe_1:last\n" + std::string(SUMMARY_SUFFIX);
    ASSERT_TRUE(ParseFreezeStackSummary(two, summary));
    ASSERT_EQ(summary.frames.size(), 2);
    ASSERT_EQ(summary.frames[1], "last");
}

HWTEST(FreezeStackSummaryTest, UsesLaterReadableCandidate, TestSize.Level1)
{
    constexpr char firstPath[] = "/data/test/req1-empty-freeze-summary.txt";
    constexpr char secondPath[] = "/data/test/req1-valid-freeze-summary.txt";
    const std::string valid = std::string(SUMMARY_PREFIX) +
        "frame_count:1\nframe_0:main\n" + SUMMARY_SUFFIX;
    ASSERT_TRUE(FileUtil::SaveStringToFile(firstPath, ""));
    ASSERT_TRUE(FileUtil::SaveStringToFile(secondPath, valid));

    std::map<std::string, std::string> eventInfos;
    eventInfos[FaultKey::FIRST_FRAME] = "fallback";
    EXPECT_TRUE(ApplyFreezeStackSummary(std::string(firstPath) + "," + secondPath, eventInfos));
    EXPECT_EQ(eventInfos[FaultKey::FIRST_FRAME], "main");
    EXPECT_EQ(eventInfos[FaultKey::LAST_FRAME], "main");
    EXPECT_EQ(eventInfos[FaultKey::STACK_SOURCE], "HEAVIEST_SAMPLE");
    unlink(firstPath);
    unlink(secondPath);
}

HWTEST(FreezeStackSummaryTest, NoSampleWithoutInstantStackIsInvalid, TestSize.Level1)
{
    constexpr char path[] = "/data/test/req1-no-sample-freeze-summary.txt";
    const std::string noSample =
        "---AppFreezeHeaviestStack Begin---\nversion:1\nstatus:no_sample\n"
        "total_samples:0\nbusiest_count:0\nbusiest_ratio_permille:0\n"
        "first_snapshot_time:0\nstack_id:0\nframe_count:0\n" + std::string(SUMMARY_SUFFIX);
    ASSERT_TRUE(FileUtil::SaveStringToFile(path, noSample));

    std::map<std::string, std::string> eventInfos;
    EXPECT_FALSE(ApplyFreezeStackSummary(path, eventInfos));
    EXPECT_EQ(eventInfos[FaultKey::STACK_SOURCE], "NONE");
    EXPECT_EQ(eventInfos[FaultKey::HAS_MAIN_THREAD_STACK], "false");
    EXPECT_EQ(eventInfos[FaultKey::HEAVIEST_STACK_STATUS], "no_sample");
    EXPECT_EQ(eventInfos[FaultKey::LOG_VALIDITY], "INVALID");
    unlink(path);
}

HWTEST(FreezeStackSummaryTest, ApplySummaryP99UnderFiveMilliseconds, TestSize.Level2)
{
    constexpr char path[] = "/data/test/req1-perf-freeze-summary.txt";
    const std::string valid = std::string(SUMMARY_PREFIX) +
        "frame_count:3\nframe_0:first\nframe_1:second\nframe_2:last\n" + SUMMARY_SUFFIX;
    ASSERT_TRUE(FileUtil::SaveStringToFile(path, valid));

    constexpr int iterations = 10000;
    std::vector<int64_t> durations;
    durations.reserve(iterations);
    for (int i = 0; i < iterations; ++i) {
        std::map<std::string, std::string> eventInfos;
        const auto start = std::chrono::steady_clock::now();
        ASSERT_TRUE(ApplyFreezeStackSummary(path, eventInfos));
        const auto end = std::chrono::steady_clock::now();
        durations.push_back(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
    }

    std::sort(durations.begin(), durations.end());
    const size_t p99Index = (durations.size() * 99) / 100;
    ASSERT_LT(p99Index, durations.size());
    RecordProperty("iterations", iterations);
    RecordProperty("p99_us", durations[p99Index]);
    EXPECT_LT(durations[p99Index], 5000);
    unlink(path);
}
}  // namespace HiviewDFX
}  // namespace OHOS
