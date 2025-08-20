/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "triggle_export_test.h"

#include <memory>

#include "export_config_manager.h"
#include "export_db_manager.h"
#include "file_util.h"
#include "hiview_global.h"
#include "setting_observer_manager.h"
#include "sys_event.h"
#include "sys_event_sequence_mgr.h"
#include "time_util.h"
#include "triggle_export_engine.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr int FIRST_TASK_ID = 1;
constexpr char TEST_MODULE_NAME[] = "test4";
constexpr char TEST_CFG_DIR[] = "/data/test/test_data/cfg/";
constexpr char TEST_WORK_DIR[] = "/data/test/test_data/work/";
constexpr int64_t TEST_TASK_TYPE = 180;
constexpr int64_t TEST_TRIGGLE_CYCLE = 5;

std::shared_ptr<SysEvent> BuildTestSysEvent(int64_t seq)
{
    SysEventCreator sysEventCreator("DEMO", "EVENT_NAME", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("KEY", 1);
    sysEventCreator.SetKeyValue("reportInterval_", TEST_TASK_TYPE);
    sysEventCreator.SetKeyValue("seq_", seq);
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    return sysEvent;
}

class TriggleExportTestContext : public HiviewContext {
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

void TriggleExportTest::SetUpTestCase()
{
}

void TriggleExportTest::TearDownTestCase()
{
}

void TriggleExportTest::SetUp()
{
}

void TriggleExportTest::TearDown()
{
}

/**
 * @tc.name: TriggleExportTaskTest001
 * @tc.desc: Test business of TriggleExportTask with null config
 * @tc.type: FUNC
 * @tc.require: issueICT59K
 */
HWTEST_F(TriggleExportTest, TriggleExportTaskTest001, testing::ext::TestSize.Level3)
{
    auto triggleTask = std::make_shared<TriggleExportTask>(nullptr, FIRST_TASK_ID);
    ASSERT_NE(triggleTask, nullptr);
    ASSERT_TRUE(triggleTask->GetModuleName().empty());
    ASSERT_EQ(triggleTask->GetTimeStamp(), 0);
    ASSERT_EQ(triggleTask->GetId(), FIRST_TASK_ID);
    ASSERT_EQ(triggleTask->GetTriggleCycle(), std::chrono::seconds(0));
    triggleTask->AppendEvent(nullptr);
    ASSERT_EQ(triggleTask->GetTimeStamp(), 0);
    auto maxEventSeq = EventStore::SysEventSequenceManager::GetInstance().GetSequence();
    auto sysEvent = BuildTestSysEvent(++maxEventSeq);
    triggleTask->AppendEvent(sysEvent);
    ASSERT_GT(triggleTask->GetTimeStamp(), 0);
}

/**
 * @tc.name: TriggleExportTaskTest002
 * @tc.desc: Test business of TriggleExportTask with normal config
 * @tc.type: FUNC
 * @tc.require: issueICT59K
 */
HWTEST_F(TriggleExportTest, TriggleExportTaskTest002, testing::ext::TestSize.Level3)
{
    std::shared_ptr<ExportConfig> config = std::make_shared<ExportConfig>();
    config->moduleName = TEST_MODULE_NAME;
    config->taskTriggleCycle = TEST_TRIGGLE_CYCLE;
    auto triggleTask = std::make_shared<TriggleExportTask>(config, FIRST_TASK_ID);
    ASSERT_NE(triggleTask, nullptr);
    ASSERT_EQ(triggleTask->GetModuleName(), TEST_MODULE_NAME);
    ASSERT_EQ(triggleTask->GetTriggleCycle(), std::chrono::seconds(TEST_TRIGGLE_CYCLE));
}

/**
 * @tc.name: TriggleExportTaskTest003
 * @tc.desc: Test entire business of TriggleExportTask
 * @tc.type: FUNC
 * @tc.require: issueICT59K
 */
HWTEST_F(TriggleExportTest, TriggleExportTaskTest003, testing::ext::TestSize.Level3)
{
    TriggleExportTestContext context;
    HiviewGlobal::CreateInstance(context);

    std::vector<std::shared_ptr<ExportConfig>> configs;
    ExportConfigManager::GetInstance().GetTriggleExportConfigs(configs);
    ASSERT_EQ(configs.size(), 1); // 1 is expected config number

    auto triggleTask = std::make_shared<TriggleExportTask>(configs.front(), FIRST_TASK_ID);
    auto maxEventSeq = EventStore::SysEventSequenceManager::GetInstance().GetSequence();
    triggleTask->AppendEvent(BuildTestSysEvent(++maxEventSeq));
    triggleTask->AppendEvent(BuildTestSysEvent(++maxEventSeq));
    triggleTask->Run();

    ASSERT_EQ(triggleTask->GetModuleName(), TEST_MODULE_NAME);
}

/**
 * @tc.name:TriggleExportEngineTest001
 * @tc.desc: Test apis of TriggleExportEngine
 * @tc.type: FUNC
 * @tc.require: issueICT59K
 */
HWTEST_F(TriggleExportTest, TriggleExportEngineTest001, testing::ext::TestSize.Level3)
{
    TriggleExportTestContext context;
    HiviewGlobal::CreateInstance(context);

    auto& triggleExportEngine = TriggleExportEngine::GetInstance();
    triggleExportEngine.SetTaskDelayedSecond(1); // 1 second is a test delay value

    auto maxEventSeq = EventStore::SysEventSequenceManager::GetInstance().GetSequence();
    triggleExportEngine.ProcessEvent(nullptr);
    triggleExportEngine.ProcessEvent(BuildTestSysEvent(++maxEventSeq));
    triggleExportEngine.ProcessEvent(BuildTestSysEvent(++maxEventSeq));

    TimeUtil::Sleep(5); // sleep 5 seconds

    std::vector<std::shared_ptr<ExportConfig>> configs;
    ExportConfigManager::GetInstance().GetTriggleExportConfigs(configs);
    ASSERT_EQ(configs.size(), 1); // 1 is expected config number
    auto frontConfig = configs.front();
    ASSERT_NE(frontConfig, nullptr);
    if (SettingObserverManager::GetInstance()->GetStringValue(frontConfig->exportSwitchParam.name)
        == frontConfig->exportSwitchParam.enabledVal) {
        std::vector<std::string> eventZipFiles;
        FileUtil::GetDirFiles(TEST_WORK_DIR, eventZipFiles);
        ASSERT_FALSE(eventZipFiles.empty());
    }
}
} // namespace HiviewDFX
} // namespace OHOS