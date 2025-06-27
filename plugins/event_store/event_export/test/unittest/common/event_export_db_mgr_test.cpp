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

#include "event_export_db_mgr_test.h"

#include <limits>

#include "export_db_manager.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr char TEST_MODULE_NAME[] = "test_module";
constexpr int64_t FIRST_ENABLED_SEQ = 10;
constexpr int64_t SECOND_ENABLED_SEQ = 100;
constexpr int64_t THIRD_ENABLED_SEQ = 350;
constexpr int64_t FIRST_FINISH_SEQ = 150;
constexpr int64_t SECOND_FINISH_SEQ = 200;
constexpr int64_t THIRD_FINISH_SEQ = 400;
}

void EventExportDbMgrTest::SetUpTestCase()
{
}

void EventExportDbMgrTest::TearDownTestCase()
{
}

void EventExportDbMgrTest::SetUp()
{
}

void EventExportDbMgrTest::TearDown()
{
}

/**
 * @tc.name: EventExportDbMgrTest001
 * @tc.desc: ExportDbManager test with normal steps
 * @tc.type: FUNC
 * @tc.require: issueI9G6TM
 */
HWTEST_F(EventExportDbMgrTest, EventExportDbMgrTest001, testing::ext::TestSize.Level3)
{
    ExportDbManager manager("/data/test/test_data/db_dir1/");
    ASSERT_EQ(manager.GetEventInheritFlagPath("module_name"),
        "/data/test/test_data/db_dir1/event_inherit_flag_module_name");
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), INVALID_SEQ_VAL);
    // export switch on at 10
    manager.HandleExportSwitchChanged(TEST_MODULE_NAME, FIRST_ENABLED_SEQ);
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), FIRST_ENABLED_SEQ);
    // export task finish at 150
    manager.HandleExportTaskFinished(TEST_MODULE_NAME, FIRST_FINISH_SEQ);
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), FIRST_FINISH_SEQ);
    // export switch off
    manager.HandleExportSwitchChanged(TEST_MODULE_NAME, INVALID_SEQ_VAL);
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), INVALID_SEQ_VAL);
    // export switch on at 100
    manager.HandleExportSwitchChanged(TEST_MODULE_NAME, SECOND_ENABLED_SEQ);
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), FIRST_FINISH_SEQ);
    // export task finish at 200
    manager.HandleExportTaskFinished(TEST_MODULE_NAME, SECOND_FINISH_SEQ);
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), SECOND_FINISH_SEQ);
    // export switch off
    manager.HandleExportSwitchChanged(TEST_MODULE_NAME, INVALID_SEQ_VAL);
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), INVALID_SEQ_VAL);
    // export switch on at 350
    manager.HandleExportSwitchChanged(TEST_MODULE_NAME, THIRD_ENABLED_SEQ);
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), THIRD_ENABLED_SEQ);
    // export task finish at 400
    manager.HandleExportTaskFinished(TEST_MODULE_NAME, THIRD_FINISH_SEQ);
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), THIRD_FINISH_SEQ);
}

/**
 * @tc.name: EventExportDbMgrTest002
 * @tc.desc: ExportDbManager test with unnormal steps: switch on->init->switch on
 * @tc.type: FUNC
 * @tc.require: issueI9G6TM
 */
HWTEST_F(EventExportDbMgrTest, EventExportDbMgrTest002, testing::ext::TestSize.Level3)
{
    ExportDbManager manager("/data/test/test_data/db_dir2");
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), INVALID_SEQ_VAL);
    // export switch on at 10
    manager.HandleExportSwitchChanged(TEST_MODULE_NAME, FIRST_ENABLED_SEQ);
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), FIRST_ENABLED_SEQ);
    // export switch on at 100
    manager.HandleExportSwitchChanged(TEST_MODULE_NAME, SECOND_ENABLED_SEQ);
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), SECOND_ENABLED_SEQ);
}

/**
 * @tc.name: EventExportDbMgrTest003
 * @tc.desc: ExportDbManager test with unnormal steps: switch off->init->switch->switch off->switch on
 * @tc.type: FUNC
 * @tc.require: issueI9G6TM
 */
HWTEST_F(EventExportDbMgrTest, EventExportDbMgrTest003, testing::ext::TestSize.Level3)
{
    ExportDbManager manager("/data/test/test_data/db_dir3");
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), INVALID_SEQ_VAL);
    // export switch off
    manager.HandleExportSwitchChanged(TEST_MODULE_NAME, INVALID_SEQ_VAL);
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), INVALID_SEQ_VAL);
    // export switch off
    manager.HandleExportSwitchChanged(TEST_MODULE_NAME, INVALID_SEQ_VAL);
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), INVALID_SEQ_VAL);
    // export switch on at 350
    manager.HandleExportSwitchChanged(TEST_MODULE_NAME, THIRD_ENABLED_SEQ);
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), THIRD_ENABLED_SEQ);
}

/**
 * @tc.name: EventExportDbMgrTest004
 * @tc.desc: ExportDbManager test with unnormal steps: task finished->init->task finished->switch on
 * @tc.type: FUNC
 * @tc.require: issueI9G6TM
 */
HWTEST_F(EventExportDbMgrTest, EventExportDbMgrTest004, testing::ext::TestSize.Level3)
{
    ExportDbManager manager("/data/test/test_data/db_dir4");
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), INVALID_SEQ_VAL);
    // export task finish at 150
    manager.HandleExportTaskFinished(TEST_MODULE_NAME, FIRST_FINISH_SEQ);
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), INVALID_SEQ_VAL);
    // export task finish at 200
    manager.HandleExportTaskFinished(TEST_MODULE_NAME, SECOND_FINISH_SEQ);
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), INVALID_SEQ_VAL);
    // export switch on at 350
    manager.HandleExportSwitchChanged(TEST_MODULE_NAME, THIRD_ENABLED_SEQ);
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), THIRD_ENABLED_SEQ);
}
} // namespace HiviewDFX
} // namespace OHOS