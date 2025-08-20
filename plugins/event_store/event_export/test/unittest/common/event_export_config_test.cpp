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

#include "event_export_config_test.h"

#include <vector>

#include "export_config_manager.h"
#include "export_event_list_parser.h"
#include "hiview_global.h"
#include "parameter_ex.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
// all value is from resource file: test_event_export_config.json and test_events.json
constexpr char TEST_CONFIG_DIR[] = "/data/test/test_data/";
constexpr char TEST_CONFIG_FILE[] = "/data/test/test_data/sys_event_export/test_event_export_config.json";
constexpr char TEST_MODULE_NAME[] = "test";
constexpr char TEST_SETTING_DB_PARAM_NAME[] = "test_param_key";
constexpr char TEST_SETTING_DB_PARAM_ENABLED_VAL[] = "1";
constexpr size_t TEST_PERIODIC_MODULE_CNT = 2;
constexpr size_t TEST_TRIGGLE_MODULE_CNT = 1;
constexpr int64_t TEST_CAPACITY = 100;
constexpr int64_t TEST_SIZE = 2;
constexpr int64_t TEST_CYCLE = 3600;
constexpr int64_t TEST_FILE_STORE_DAY_CNT = 7;
constexpr size_t TEST_EXPORT_NAME_CNT = 3;
constexpr size_t TEST_EXPORT_CFG_FILE_CNT = 2;
constexpr int64_t TEST_EXPORT_CFG_VERSION = 202404061011;
constexpr char TEST_EXPORT_CFG_FILE[] = "/data/test/test_data/test_events_v2.json";
constexpr char INVALID_TEST_EXPORT_CFG_FILE1[] = "/data/test/test_data/invalid_test_events1.json";
constexpr char INVALID_TEST_EXPORT_CFG_FILE2[] = "/data/test/test_data/invalid_test_events2.json";
constexpr char INVALID_TEST_EXPORT_CFG_FILE3[] = "/data/test/test_data/invalid_test_events3.json";
constexpr int64_t DEFAULT_VERSION = 0;
constexpr char TEST_CONFIG_FILE1[] = "/data/test/test_data/sys_event_export/test1_event_export_config.json";
constexpr char TEST_CONFIG_FILE2[] = "/data/test/test_data/sys_event_export/test2_event_export_config.json";

class EventExportConfigTestContext : public HiviewContext {
public:
    std::string GetHiViewDirectory(DirectoryType type __UNUSED)
    {
        return TEST_CONFIG_DIR;
    }
};
}
void EventExportConfigParseTest::SetUpTestCase()
{
}

void EventExportConfigParseTest::TearDownTestCase()
{
}

void EventExportConfigParseTest::SetUp()
{
}

void EventExportConfigParseTest::TearDown()
{
}

/**
 * @tc.name: EventExportConfigParseTest001
 * @tc.desc: EventConfigManager test
 * @tc.type: FUNC
 * @tc.require: issueI9E8HA
 */
HWTEST_F(EventExportConfigParseTest, EventExportConfigParseTest001, testing::ext::TestSize.Level3)
{
    EventExportConfigTestContext context;
    HiviewGlobal::CreateInstance(context);
    auto& manager = ExportConfigManager::GetInstance();
    std::vector<std::string> moduleNames;
    manager.GetModuleNames(moduleNames);
    ASSERT_EQ(moduleNames.size(), TEST_PERIODIC_MODULE_CNT + TEST_TRIGGLE_MODULE_CNT);
    auto testModuleName = moduleNames.at(0);
    ASSERT_EQ(testModuleName, TEST_MODULE_NAME);
    auto exportConfig = manager.GetExportConfig(testModuleName);
    ASSERT_NE(exportConfig, nullptr);
    std::vector<std::shared_ptr<ExportConfig>> configs;
    manager.GetPeriodicExportConfigs(configs);
    ASSERT_EQ(configs.size(), TEST_PERIODIC_MODULE_CNT);
    configs.clear();
    manager.GetTriggleExportConfigs(configs);
    ASSERT_EQ(configs.size(), TEST_TRIGGLE_MODULE_CNT);
}

/**
 * @tc.name: EventExportConfigParseTest002
 * @tc.desc: EventConfigParser test
 * @tc.type: FUNC
 * @tc.require: issueI9E8HA
 */
HWTEST_F(EventExportConfigParseTest, EventExportConfigParseTest002, testing::ext::TestSize.Level3)
{
    ExportConfigParser parser(TEST_CONFIG_FILE, TEST_MODULE_NAME);
    auto exportConfig = parser.Parse();
    ASSERT_NE(exportConfig, nullptr);
    ASSERT_EQ(exportConfig->moduleName, TEST_MODULE_NAME);
    ASSERT_EQ(exportConfig->exportDir, "/data/test/test_data/sys_event_export/");
    ASSERT_EQ(exportConfig->maxCapcity, TEST_CAPACITY);
    ASSERT_EQ(exportConfig->maxSize, TEST_SIZE);
    ASSERT_EQ(exportConfig->taskCycle, TEST_CYCLE);
    ASSERT_EQ(exportConfig->dayCnt, TEST_FILE_STORE_DAY_CNT);
    ExportConfigParser parser1(TEST_CONFIG_FILE1, TEST_MODULE_NAME);
    exportConfig = parser1.Parse();
    ASSERT_NE(exportConfig, nullptr);
    ASSERT_EQ(exportConfig->moduleName, TEST_MODULE_NAME);
    ASSERT_NE(exportConfig->exportDir, "/data/test/test_data/sys_event_export/");
}

/**
 * @tc.name: EventExportConfigParseTest003
 * @tc.desc: EventExportList test
 * @tc.type: FUNC
 * @tc.require: issueI9E8HA
 */
HWTEST_F(EventExportConfigParseTest, EventExportConfigParseTest003, testing::ext::TestSize.Level3)
{
    ExportConfigParser parser(TEST_CONFIG_FILE, TEST_MODULE_NAME);
    auto exportConfig = parser.Parse();
    ASSERT_NE(exportConfig, nullptr);
    ASSERT_EQ(exportConfig->eventsConfigFiles.size(), TEST_EXPORT_CFG_FILE_CNT);
}

/**
 * @tc.name: EventExportConfigParseTest004
 * @tc.desc: SettingDbParam test
 * @tc.type: FUNC
 * @tc.require: issueI9E8HA
 */
HWTEST_F(EventExportConfigParseTest, EventExportConfigParseTest004, testing::ext::TestSize.Level3)
{
    ExportConfigParser parser(TEST_CONFIG_FILE, TEST_MODULE_NAME);
    auto exportConfig = parser.Parse();
    ASSERT_NE(exportConfig, nullptr);
    ASSERT_EQ(exportConfig->exportSwitchParam.name, TEST_SETTING_DB_PARAM_NAME);
    ASSERT_EQ(exportConfig->exportSwitchParam.enabledVal, TEST_SETTING_DB_PARAM_ENABLED_VAL);
    ASSERT_EQ(exportConfig->sysUpgradeParam.name, TEST_SETTING_DB_PARAM_NAME);
    ASSERT_EQ(exportConfig->sysUpgradeParam.enabledVal, TEST_SETTING_DB_PARAM_ENABLED_VAL);
}

/**
 * @tc.name: EventExportConfigParseTest005
 * @tc.desc: EventExportConfigParseTest005 test
 * @tc.type: FUNC
 * @tc.require: issueICI4CO
 */
HWTEST_F(EventExportConfigParseTest, EventExportConfigParseTest005, testing::ext::TestSize.Level3)
{
    ExportConfigParser parser1(TEST_CONFIG_FILE1, TEST_MODULE_NAME);
    auto exportConfig = parser1.Parse();
    ASSERT_NE(exportConfig, nullptr);
    if (Parameter::IsOversea()) {
        if (Parameter::IsBetaVersion()) {
            ASSERT_EQ(exportConfig->exportDir, "/data/test/test_data/sys_event_export/180/");
            ASSERT_EQ(exportConfig->taskType, 180);    // 180 is expected value
            ASSERT_EQ(exportConfig->taskCycle, 2000);  // 2000 is expected value
        } else {
            ASSERT_EQ(exportConfig->exportDir, "/data/test/test_data/sys_event_export/3000/");
            ASSERT_EQ(exportConfig->taskType, 3000);   // 3000 is expected value
            ASSERT_EQ(exportConfig->taskCycle, 3000);  // 3000 is expected value
        }
    } else {
        if (Parameter::IsBetaVersion()) {
            ASSERT_EQ(exportConfig->exportDir, "/data/test/test_data/sys_event_export/0/");
            ASSERT_EQ(exportConfig->taskType, ALL_EVENT_TASK_TYPE);
            ASSERT_EQ(exportConfig->taskCycle, 1000);  // 1000 is expected value
        } else {
            ASSERT_EQ(exportConfig->exportDir, "/data/test/test_data/sys_event_export/2000/");
            ASSERT_EQ(exportConfig->taskType, 2000);   // 2000 is expected value
            ASSERT_EQ(exportConfig->taskCycle, 2000);  // 2000 is expected value
        }
    }
    ASSERT_EQ(exportConfig->maxCapcity, TEST_CAPACITY);
    ASSERT_EQ(exportConfig->maxSize, TEST_SIZE);
    ASSERT_EQ(exportConfig->dayCnt, TEST_FILE_STORE_DAY_CNT);
    ASSERT_EQ(exportConfig->inheritedModule, "inherited_test_module");

    ExportConfigParser parser2(TEST_CONFIG_FILE2, TEST_MODULE_NAME);
    exportConfig = parser2.Parse();
    ASSERT_EQ(exportConfig, nullptr);
}

/**
 * @tc.name: ExportEventListParserTest005
 * @tc.desc: ExportEventListParser test
 * @tc.type: FUNC
 * @tc.require: issueIAC4BC
 */
HWTEST_F(EventExportConfigParseTest, ExportEventListParserTest005, testing::ext::TestSize.Level3)
{
    ExportEventListParser parser(TEST_EXPORT_CFG_FILE);
    ExportEventList list;
    parser.GetExportEventList(list);
    ASSERT_GT(list.size(), 0);
    auto iter = list.find("DOMAIN1");
    ASSERT_NE(iter, list.end());
    iter = list.find("DOMAIN2");
    ASSERT_NE(iter, list.end());
    ASSERT_EQ(iter->second.size(), TEST_EXPORT_NAME_CNT);
    ASSERT_EQ(parser.GetConfigurationVersion(), TEST_EXPORT_CFG_VERSION);
}

/**
 * @tc.name: ExportEventListParserTest006
 * @tc.desc: ExportEventListParser test
 * @tc.type: FUNC
 * @tc.require: issueIAC4BC
 */
HWTEST_F(EventExportConfigParseTest, ExportEventListParserTest006, testing::ext::TestSize.Level3)
{
    ExportEventListParser parser1(INVALID_TEST_EXPORT_CFG_FILE1);
    ExportEventList list;
    parser1.GetExportEventList(list);
    ASSERT_EQ(list.size(), 0);
    ASSERT_EQ(parser1.GetConfigurationVersion(), DEFAULT_VERSION);
    ExportEventListParser parser2(INVALID_TEST_EXPORT_CFG_FILE2);
    parser2.GetExportEventList(list);
    ASSERT_EQ(list.size(), 0);
    ASSERT_EQ(parser2.GetConfigurationVersion(), TEST_EXPORT_CFG_VERSION);
    ExportEventListParser parser3(INVALID_TEST_EXPORT_CFG_FILE3);
    parser3.GetExportEventList(list);
    ASSERT_EQ(list.size(), 0);
}
} // namespace HiviewDFX
} // namespace OHOS