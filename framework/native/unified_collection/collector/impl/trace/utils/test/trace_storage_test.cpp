/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "app_event_task_storage.h"
#include "file_util.h"
#include "rdb_helper.h"
#include "time_util.h"
#include "trace_storage.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
namespace {
const char TEST_DB_PATH[] = "/data/test/trace_storage_test/";
}
class TraceStorageTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase()
    {
        if (!FileUtil::FileExists(TEST_DB_PATH)) {
            FileUtil::ForceCreateDirectory(TEST_DB_PATH);
        }
    };
    static void TearDownTestCase()
    {
        if (FileUtil::FileExists(TEST_DB_PATH)) {
            FileUtil::ForceRemoveDirectory(TEST_DB_PATH);
        }
    };
};

/**
 * @tc.name: TraceStorageTest001
 * @tc.desc: used to test TraceStorage api
 * @tc.type: FUNC
*/
HWTEST_F(TraceStorageTest, TraceStorageTest001, TestSize.Level1)
{
    auto traceStorage = std::make_shared<TraceStorage>(TEST_DB_PATH);
    TraceFlowRecord traceFlowRecord = {.systemTime = "2024-06-14", .callerName = "hiview",
        .usedSize = 1000000}; // 1000000 : test size
    traceStorage->Store(traceFlowRecord);
    TraceFlowRecord traceFlowRecordQuery;
    traceFlowRecordQuery.callerName = "hiview";
    traceStorage->Query(traceFlowRecordQuery);
    ASSERT_EQ(traceFlowRecordQuery.systemTime, traceFlowRecord.systemTime);
    ASSERT_EQ(traceFlowRecordQuery.callerName, traceFlowRecord.callerName);
    ASSERT_EQ(traceFlowRecordQuery.usedSize, traceFlowRecord.usedSize);
    traceFlowRecord.usedSize = 2000000; // 2000000 : test size
    traceStorage->Store(traceFlowRecord);
    traceStorage->Query(traceFlowRecordQuery);
    ASSERT_EQ(traceFlowRecordQuery.usedSize, traceFlowRecord.usedSize);
}

/**
 * @tc.name: TraceStorageTest002
 * @tc.desc: used to test TraceStorage api
 * @tc.type: FUNC
*/
HWTEST_F(TraceStorageTest, TraceStorageTest002, TestSize.Level1)
{
    auto traceStorage = std::make_shared<TraceStorage>(TEST_DB_PATH);
    AppEventTask appEventTaskOld;
    const int64_t secondsOfOneDay = 3600 * 24; // 3600 * 24 : seconds per hour; hours per day
    const int64_t secondsOfThreeDays = 3 * 3600 * 24; // 3 * 3600 * 24 : three days; seconds per hour; hours per day
    auto dateOneDayAgo = TimeUtil::TimestampFormatToDate(TimeUtil::GetSeconds() - secondsOfOneDay, "%Y%m%d");
    auto dateThreeDaysAgo = TimeUtil::TimestampFormatToDate(TimeUtil::GetSeconds() - secondsOfThreeDays, "%Y%m%d");
    appEventTaskOld.uid_ = 100; // 100 : test uid
    appEventTaskOld.taskDate_ = std::stoll(dateThreeDaysAgo, nullptr, 0);
    traceStorage->StoreAppEventTask(appEventTaskOld);
    AppEventTask appEventTaskQuery;
    appEventTaskQuery.id_ = 0;
    traceStorage->QueryAppEventTask(appEventTaskOld.uid_, appEventTaskOld.taskDate_, appEventTaskQuery);
    ASSERT_GT(appEventTaskQuery.id_, 0);
    appEventTaskOld.taskDate_ = std::stoll(dateOneDayAgo, nullptr, 0);
    traceStorage->StoreAppEventTask(appEventTaskOld);
    traceStorage->RemoveOldAppEventTask(appEventTaskOld.taskDate_);
    appEventTaskQuery.id_ = 0;
    traceStorage->QueryAppEventTask(appEventTaskOld.uid_, appEventTaskOld.taskDate_, appEventTaskQuery);
    ASSERT_GT(appEventTaskQuery.id_, 0);
    appEventTaskQuery.id_ = 0;
    traceStorage->QueryAppEventTask(appEventTaskOld.uid_, std::stoll(dateThreeDaysAgo, nullptr, 0), appEventTaskQuery);
    ASSERT_EQ(appEventTaskQuery.id_, 0);
}
}
}
