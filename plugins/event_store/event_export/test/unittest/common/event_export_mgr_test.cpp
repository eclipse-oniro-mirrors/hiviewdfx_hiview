/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "event_export_mgr_test.h"

#include "event_expire_task.h"
#include "event_export_engine.h"
#include "event_export_task.h"
#include "event_export_util.h"
#include "export_dir_creator.h"
#include "file_util.h"
#include "hiview_global.h"
#include "setting_observer_manager.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr char PARAM_NAME[] = "PARAM_NAME";
constexpr char DEFAULT_VAL[] = "DEFAULT_VAL";
constexpr char TEST_CFG_DIR[] = "/data/test/test_data/cfg/";
constexpr char TEST_WORK_DIR[] = "/data/test/test_data/work/";
constexpr char TEST_CFG_FILE[] = "/data/test/test_data/cfg/sys_event_export/test3_event_export_config.json";
constexpr char TEST_MODULE_NAME[] = "test3";

class EventExportMgrTestContext : public HiviewContext {
public:
    std::string GetHiViewDirectory(DirectoryType type)
    {
        if (type == HiviewContext::DirectoryType::WORK_DIRECTORY) {
            return TEST_WORK_DIR;
        }
        return TEST_CFG_DIR;
    }
};
}

void EventExportMgrTest::SetUpTestCase()
{
}

void EventExportMgrTest::TearDownTestCase()
{
}

void EventExportMgrTest::SetUp()
{
}

void EventExportMgrTest::TearDown()
{
}

/**
 * @tc.name: EventExportMgrTest001
 * @tc.desc: Test apis of SettingObserverManager
 * @tc.type: FUNC
 * @tc.require: issueI9GHI8
 */
HWTEST_F(EventExportMgrTest, EventExportMgrTest001, testing::ext::TestSize.Level3)
{
    SettingObserver::ObserverCallback callback =
        [] (const std::string& paramKey) {
            // do nothing
        };
    // observer not found
    ASSERT_TRUE(SettingObserverManager::GetInstance()->UnregisterObserver(PARAM_NAME));
    ASSERT_TRUE(SettingObserverManager::GetInstance()->RegisterObserver(PARAM_NAME, callback));
    // observer exit
    ASSERT_TRUE(SettingObserverManager::GetInstance()->RegisterObserver(PARAM_NAME, callback));
    ASSERT_TRUE(SettingObserverManager::GetInstance()->UnregisterObserver(PARAM_NAME));
    auto value = SettingObserverManager::GetInstance()->GetStringValue(PARAM_NAME, DEFAULT_VAL);
    ASSERT_EQ(value, DEFAULT_VAL);
}

/**
 * @tc.name: EventExportMgrTest002
 * @tc.desc: Test apis of EventExportEngine
 * @tc.type: FUNC
 * @tc.require: issueICSFYS
 */
HWTEST_F(EventExportMgrTest, EventExportEngine001, testing::ext::TestSize.Level3)
{
    EventExportMgrTestContext context;
    HiviewGlobal::CreateInstance(context);
    auto& eventExportEngine = EventExportEngine::GetInstance();
    eventExportEngine.SetTaskDelayedSecond(1); // delay 1 second
    eventExportEngine.Start();
    TimeUtil::Sleep(5); // sleep 5 seconds
    eventExportEngine.Stop();

    std::vector<std::string> eventZipFiles;
    FileUtil::GetDirFiles(TEST_WORK_DIR, eventZipFiles);
    ASSERT_FALSE(eventZipFiles.empty());
}

/**
 * @tc.name: EventExportMgrTest003
 * @tc.desc: Test apis of EventExpireTask
 * @tc.type: FUNC
 * @tc.require: issueICSFYS
 */
HWTEST_F(EventExportMgrTest, EventExportMgrTest003, testing::ext::TestSize.Level3)
{
    EventExpireTask invalidTask(nullptr);
    invalidTask.Run();
    ExportConfigParser parser(TEST_CFG_FILE, TEST_MODULE_NAME);
    auto exportConfig = parser.Parse();
    ASSERT_NE(exportConfig, nullptr);
    EventExpireTask validTask(exportConfig);
    validTask.Run();
}

/**
 * @tc.name: EventExportMgrTest004
 * @tc.desc: Test apis of EventExportTask
 * @tc.type: FUNC
 * @tc.require: issueICSFYS
 */
HWTEST_F(EventExportMgrTest, EventExportMgrTest004, testing::ext::TestSize.Level3)
{
    EventExportTask invalidTask(nullptr);
    invalidTask.Run();
    ExportConfigParser parser(TEST_CFG_FILE, TEST_MODULE_NAME);
    auto exportConfig = parser.Parse();
    ASSERT_NE(exportConfig, nullptr);
    EventExportTask validTask(exportConfig);
    validTask.Run();
}

/**
 * @tc.name: EventExportUtilTest001
 * @tc.desc: Test apis of EventExportUtil
 * @tc.type: FUNC
 * @tc.require: issueICSFYS
 */
HWTEST_F(EventExportMgrTest, EventExportUtilTest001, testing::ext::TestSize.Level3)
{
    EventExportMgrTestContext context;
    HiviewGlobal::CreateInstance(context);
    ASSERT_FALSE(EventExportUtil::CheckAndPostExportEvent(nullptr));
    ExportConfigParser parser(TEST_CFG_FILE, TEST_MODULE_NAME);
    auto exportConfig = parser.Parse();
    ASSERT_NE(exportConfig, nullptr);
    exportConfig->exportDir = "/data/test/test_data/cfg/sys_event_export/";
    ASSERT_TRUE(EventExportUtil::CheckAndPostExportEvent(exportConfig));
    exportConfig->needPostEvent = false;
    ASSERT_FALSE(EventExportUtil::CheckAndPostExportEvent(exportConfig));
    exportConfig->needPostEvent = true;
    exportConfig->exportDir = "/data/test/test_data/no_exist_dir/";
    ASSERT_FALSE(EventExportUtil::CheckAndPostExportEvent(exportConfig));
    exportConfig->exportDir = "/data/test/test_data/cfg/sys_event_export/";
    exportConfig->taskType = ALL_EVENT_TASK_TYPE;
    ASSERT_TRUE(EventExportUtil::CheckAndPostExportEvent(exportConfig));
    exportConfig->taskType = 30; // 30 is a test task type
    ASSERT_TRUE(EventExportUtil::CheckAndPostExportEvent(exportConfig));

    ASSERT_TRUE(EventExportUtil::RegisterSettingObserver(exportConfig));
    EventExportUtil::UnregisterSettingObserver(exportConfig);

    auto& manager = ExportDbManager::GetInstance();
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), INVALID_SEQ_VAL);
    EventExportUtil::SyncDbByExportSwitchStatus(exportConfig, false);
    ASSERT_GT(manager.GetExportBeginSeq(TEST_MODULE_NAME), 0);
    EventExportUtil::SyncDbByExportSwitchStatus(exportConfig, true);
    ASSERT_EQ(manager.GetExportBeginSeq(TEST_MODULE_NAME), INVALID_SEQ_VAL);
    EventExportUtil::InitEnvBeforeExport(exportConfig);
    ASSERT_TRUE(FileUtil::FileExists(exportConfig->exportDir));
}

/**
 * @tc.name: ExportDirCreator001
 * @tc.desc: Test apis of ExportDirCreator
 * @tc.type: FUNC
 * @tc.require: issueICSFYS
 */
HWTEST_F(EventExportMgrTest, ExportDirCreator001, testing::ext::TestSize.Level3)
{
    EventExportMgrTestContext context;
    HiviewGlobal::CreateInstance(context);
    ASSERT_TRUE(ExportDirCreator::GetInstance().CreateExportDir("/data/test/"));
    ASSERT_TRUE(ExportDirCreator::GetInstance().CreateExportDir("/data/service/el1/public|hiview_upload/"));
    ASSERT_FALSE(ExportDirCreator::GetInstance().CreateExportDir("/data/service/el11/public|hiview_upload/"));
}
} // namespace HiviewDFX
} // namespace OHOS