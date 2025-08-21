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

#include "trigger_export_test.h"

#include <memory>

#include "export_config_manager.h"
#include "export_db_manager.h"
#include "file_util.h"
#include "hiview_global.h"
#include "setting_observer_manager.h"
#include "sys_event.h"
#include "sys_event_sequence_mgr.h"
#include "time_util.h"
#include "trigger_export_engine.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr int FIRST_TASK_ID = 1;
constexpr char TEST_MODULE_NAME[] = "test4";
constexpr char TEST_CFG_DIR[] = "/data/test/test_data/cfg/";
constexpr char TEST_WORK_DIR[] = "/data/test/test_data/work/";
constexpr int64_t TEST_TASK_TYPE = 180;
constexpr int64_t TEST_TRIGGER_CYCLE = 5;

std::shared_ptr<SysEvent> BuildTestSysEvent(int64_t seq)
{
    SysEventCreator sysEventCreator("DEMO", "EVENT_NAME", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("KEY", 1);
    sysEventCreator.SetKeyValue("reportInterval_", TEST_TASK_TYPE);
    sysEventCreator.SetKeyValue("seq_", seq);
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    return sysEvent;
}

class TriggerExportTestContext : public HiviewContext {
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

void TriggerExportTest::SetUpTestCase()
{
}

void TriggerExportTest::TearDownTestCase()
{
}

void TriggerExportTest::SetUp()
{
}

void TriggerExportTest::TearDown()
{
}

/**
 * @tc.name: TriggerExportTaskTest001
 * @tc.desc: Test business of TriggerExportTask with null config
 * @tc.type: FUNC
 * @tc.require: issueICT59K
 */
HWTEST_F(TriggerExportTest, TriggerExportTaskTest001, testing::ext::TestSize.Level3)
{
    auto triggerTask = std::make_shared<TriggerExportTask>(nullptr, FIRST_TASK_ID);
    ASSERT_NE(triggerTask, nullptr);
    ASSERT_TRUE(triggerTask->GetModuleName().empty());
    ASSERT_EQ(triggerTask->GetTimeStamp(), 0);
    ASSERT_EQ(triggerTask->GetId(), FIRST_TASK_ID);
    ASSERT_EQ(triggerTask->GetTriggerCycle(), std::chrono::seconds(0));
    triggerTask->AppendEvent(nullptr);
    ASSERT_EQ(triggerTask->GetTimeStamp(), 0);
    auto maxEventSeq = EventStore::SysEventSequenceManager::GetInstance().GetSequence();
    auto sysEvent = BuildTestSysEvent(++maxEventSeq);
    triggerTask->AppendEvent(sysEvent);
    ASSERT_GT(triggerTask->GetTimeStamp(), 0);
}

/**
 * @tc.name: TriggerExportTaskTest002
 * @tc.desc: Test business of TriggerExportTask with normal config
 * @tc.type: FUNC
 * @tc.require: issueICT59K
 */
HWTEST_F(TriggerExportTest, TriggerExportTaskTest002, testing::ext::TestSize.Level3)
{
    std::shared_ptr<ExportConfig> config = std::make_shared<ExportConfig>();
    config->moduleName = TEST_MODULE_NAME;
    config->taskTriggerCycle = TEST_TRIGGER_CYCLE;
    auto triggerTask = std::make_shared<TriggerExportTask>(config, FIRST_TASK_ID);
    ASSERT_NE(triggerTask, nullptr);
    ASSERT_EQ(triggerTask->GetModuleName(), TEST_MODULE_NAME);
    ASSERT_EQ(triggerTask->GetTriggerCycle(), std::chrono::seconds(TEST_TRIGGER_CYCLE));
}

/**
 * @tc.name: TriggerExportTaskTest003
 * @tc.desc: Test entire business of TriggerExportTask
 * @tc.type: FUNC
 * @tc.require: issueICT59K
 */
HWTEST_F(TriggerExportTest, TriggerExportTaskTest003, testing::ext::TestSize.Level3)
{
    TriggerExportTestContext context;
    HiviewGlobal::CreateInstance(context);

    std::vector<std::shared_ptr<ExportConfig>> configs;
    ExportConfigManager::GetInstance().GetTriggerExportConfigs(configs);
    ASSERT_EQ(configs.size(), 1); // 1 is expected config number

    auto triggerTask = std::make_shared<TriggerExportTask>(configs.front(), FIRST_TASK_ID);
    auto maxEventSeq = EventStore::SysEventSequenceManager::GetInstance().GetSequence();
    triggerTask->AppendEvent(BuildTestSysEvent(++maxEventSeq));
    triggerTask->AppendEvent(BuildTestSysEvent(++maxEventSeq));
    triggerTask->Run();

    ASSERT_EQ(triggerTask->GetModuleName(), TEST_MODULE_NAME);
}

/**
 * @tc.name:TriggerExportEngineTest001
 * @tc.desc: Test apis of TriggerExportEngine
 * @tc.type: FUNC
 * @tc.require: issueICT59K
 */
HWTEST_F(TriggerExportTest, TriggerExportEngineTest001, testing::ext::TestSize.Level3)
{
    TriggerExportTestContext context;
    HiviewGlobal::CreateInstance(context);

    auto& triggerExportEngine = TriggerExportEngine::GetInstance();
    triggerExportEngine.SetTaskDelayedSecond(1); // 1 second is a test delay value

    auto maxEventSeq = EventStore::SysEventSequenceManager::GetInstance().GetSequence();
    triggerExportEngine.ProcessEvent(nullptr);
    triggerExportEngine.ProcessEvent(BuildTestSysEvent(++maxEventSeq));
    triggerExportEngine.ProcessEvent(BuildTestSysEvent(++maxEventSeq));

    TimeUtil::Sleep(5); // sleep 5 seconds

    std::vector<std::shared_ptr<ExportConfig>> configs;
    ExportConfigManager::GetInstance().GetTriggerExportConfigs(configs);
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