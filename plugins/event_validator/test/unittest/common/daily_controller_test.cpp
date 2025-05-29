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
#include <iostream>

#include <gtest/gtest.h>

#include "daily_controller.h"
#include "file_util.h"
#include "hiview_platform.h"
#include "parameter_ex.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;

class DailyControllerTest : public testing::Test {
public:
    void SetUp();
    void TearDown();

private:
    HiviewPlatform platform;
};

namespace {
const std::string WORK_PATH = "/data/test/hiview/daily_control/";
const std::string CONFIG_PATH = "/data/test/hiview/daily_control/event_threshold.json";
const std::string CONFIG_PATH2 = "/data/test/hiview/daily_control/event_threshold2.json";
const std::string TEST_DOMAIN = "DEFAULT_DOMAIN";
const std::string TEST_NAME = "DEFAULT_NAME";

std::shared_ptr<SysEvent> CreateEvent(const std::string& domain, const std::string& name,
    SysEventCreator::EventType type = SysEventCreator::FAULT)
{
    SysEventCreator sysEventCreator(domain, name, type);
    return std::make_shared<SysEvent>("", nullptr, sysEventCreator);
}

void EventThresholdTest(DailyController& controller, std::shared_ptr<SysEvent> event, uint32_t threshold)
{
    for (uint32_t i = 0; i < threshold; i++) {
        ASSERT_TRUE(controller.CheckThreshold(event));
    }

    // failed to check after the threshold is exceeded
    ASSERT_FALSE(controller.CheckThreshold(event));
}

void EventWithoutThresholdTest(DailyController& controller, std::shared_ptr<SysEvent> event, uint32_t threshold)
{
    for (uint32_t i = 0; i < threshold; i++) {
        ASSERT_TRUE(controller.CheckThreshold(event));
    }
}
}

void DailyControllerTest::SetUp()
{
    platform.GetPluginMap();
    const std::string dbDir = WORK_PATH + "sys_event_threshold/";
    if (FileUtil::IsDirectory(dbDir) && !FileUtil::ForceRemoveDirectory(dbDir)) {
        std::cout << "Failed to remove diretory=" << dbDir << std::endl;
    }
}

void DailyControllerTest::TearDown()
{}

/**
 * @tc.name: DailyControllerTest001
 * @tc.desc: FAULT event test.
 * @tc.type: FUNC
 * @tc.require: issueI9MZ5Z
 */
HWTEST_F(DailyControllerTest, DailyControllerTest001, TestSize.Level0)
{
    DailyController controller(WORK_PATH, CONFIG_PATH);
    auto event = CreateEvent(TEST_DOMAIN, TEST_NAME, SysEventCreator::FAULT);
    constexpr uint32_t thresholdOnBeta = 100;
    constexpr uint32_t thresholdOnCommercial = 20;
    uint32_t threshold = Parameter::IsBetaVersion() ? thresholdOnBeta : thresholdOnCommercial;
    EventThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest002
 * @tc.desc: STATISTIC event test.
 * @tc.type: FUNC
 * @tc.require: issueI9MZ5Z
 */
HWTEST_F(DailyControllerTest, DailyControllerTest002, TestSize.Level0)
{
    DailyController controller(WORK_PATH, CONFIG_PATH);
    auto event = CreateEvent(TEST_DOMAIN, TEST_NAME, SysEventCreator::STATISTIC);
    constexpr uint32_t thresholdOnBeta = 100;
    constexpr uint32_t thresholdOnCommercial = 20;
    uint32_t threshold = Parameter::IsBetaVersion() ? thresholdOnBeta : thresholdOnCommercial;
    EventThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest003
 * @tc.desc: SECURITY event test.
 * @tc.type: FUNC
 * @tc.require: issueI9MZ5Z
 */
HWTEST_F(DailyControllerTest, DailyControllerTest003, TestSize.Level0)
{
    DailyController controller(WORK_PATH, CONFIG_PATH);
    auto event = CreateEvent(TEST_DOMAIN, TEST_NAME, SysEventCreator::SECURITY);
    constexpr uint32_t thresholdOnBeta = 100;
    constexpr uint32_t thresholdOnCommercial = 20;
    uint32_t threshold = Parameter::IsBetaVersion() ? thresholdOnBeta : thresholdOnCommercial;
    EventThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest004
 * @tc.desc: BEHAVIOR event test.
 * @tc.type: FUNC
 * @tc.require: issueI9MZ5Z
 */
HWTEST_F(DailyControllerTest, DailyControllerTest004, TestSize.Level0)
{
    DailyController controller(WORK_PATH, CONFIG_PATH);
    auto event = CreateEvent(TEST_DOMAIN, TEST_NAME, SysEventCreator::BEHAVIOR);
    constexpr uint32_t thresholdOnBeta = 2000;
    constexpr uint32_t thresholdOnCommercial = 100;
    uint32_t threshold = Parameter::IsBetaVersion() ? thresholdOnBeta : thresholdOnCommercial;
    EventThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest005
 * @tc.desc: Custom FAULT event test.
 * @tc.type: FUNC
 * @tc.require: issueI9MZ5Z
 */
HWTEST_F(DailyControllerTest, DailyControllerTest005, TestSize.Level0)
{
    DailyController controller(WORK_PATH, CONFIG_PATH);
    auto event = CreateEvent("TEST_DOMAIN1", "FAULT_EVENT", SysEventCreator::FAULT);
    constexpr uint32_t thresholdOnBeta = 200;
    constexpr uint32_t thresholdOnCommercial = 40;
    uint32_t threshold = Parameter::IsBetaVersion() ? thresholdOnBeta : thresholdOnCommercial;
    EventThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest006
 * @tc.desc: Custom STATISTIC event test.
 * @tc.type: FUNC
 * @tc.require: issueI9MZ5Z
 */
HWTEST_F(DailyControllerTest, DailyControllerTest006, TestSize.Level0)
{
    DailyController controller(WORK_PATH, CONFIG_PATH);
    auto event = CreateEvent("TEST_DOMAIN1", "STATISTIC_EVENT", SysEventCreator::STATISTIC);
    constexpr uint32_t thresholdOnBeta = 200;
    constexpr uint32_t thresholdOnCommercial = 40;
    uint32_t threshold = Parameter::IsBetaVersion() ? thresholdOnBeta : thresholdOnCommercial;
    EventThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest007
 * @tc.desc: Custom SECURITY event test.
 * @tc.type: FUNC
 * @tc.require: issueI9MZ5Z
 */
HWTEST_F(DailyControllerTest, DailyControllerTest007, TestSize.Level0)
{
    DailyController controller(WORK_PATH, CONFIG_PATH);
    auto event = CreateEvent("TEST_DOMAIN2", "SECURITY_EVENT", SysEventCreator::SECURITY);
    constexpr uint32_t thresholdOnBeta = 200;
    constexpr uint32_t thresholdOnCommercial = 40;
    uint32_t threshold = Parameter::IsBetaVersion() ? thresholdOnBeta : thresholdOnCommercial;
    EventThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest008
 * @tc.desc: Custom BEHAVIOR event test.
 * @tc.type: FUNC
 * @tc.require: issueI9MZ5Z
 */
HWTEST_F(DailyControllerTest, DailyControllerTest008, TestSize.Level0)
{
    DailyController controller(WORK_PATH, CONFIG_PATH);
    auto event = CreateEvent("TEST_DOMAIN2", "BEHAVIOR_EVENT", SysEventCreator::BEHAVIOR);
    constexpr uint32_t thresholdOnBeta = 3000;
    constexpr uint32_t thresholdOnCommercial = 200;
    uint32_t threshold = Parameter::IsBetaVersion() ? thresholdOnBeta : thresholdOnCommercial;
    EventThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest009
 * @tc.desc: FAULT event test without threshold.
 * @tc.type: FUNC
 * @tc.require: issueI9NVZ1
 */
HWTEST_F(DailyControllerTest, DailyControllerTest009, TestSize.Level0)
{
    DailyController controller(WORK_PATH, "");
    auto event = CreateEvent(TEST_DOMAIN, TEST_NAME, SysEventCreator::FAULT);
    constexpr uint32_t threshold = 1000;
    EventWithoutThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest010
 * @tc.desc: STATISTIC event test without threshold.
 * @tc.type: FUNC
 * @tc.require: issueI9NVZ1
 */
HWTEST_F(DailyControllerTest, DailyControllerTest010, TestSize.Level0)
{
    DailyController controller(WORK_PATH, "");
    auto event = CreateEvent(TEST_DOMAIN, TEST_NAME, SysEventCreator::STATISTIC);
    constexpr uint32_t threshold = 1000;
    EventWithoutThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest011
 * @tc.desc: SECURITY event test without threshold.
 * @tc.type: FUNC
 * @tc.require: issueI9NVZ1
 */
HWTEST_F(DailyControllerTest, DailyControllerTest011, TestSize.Level0)
{
    DailyController controller(WORK_PATH, "");
    auto event = CreateEvent(TEST_DOMAIN, TEST_NAME, SysEventCreator::SECURITY);
    constexpr uint32_t threshold = 1000;
    EventWithoutThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest012
 * @tc.desc: BEHAVIOR event test without threshold.
 * @tc.type: FUNC
 * @tc.require: issueI9NVZ1
 */
HWTEST_F(DailyControllerTest, DailyControllerTest012, TestSize.Level0)
{
    DailyController controller(WORK_PATH, "");
    auto event = CreateEvent(TEST_DOMAIN, TEST_NAME, SysEventCreator::BEHAVIOR);
    constexpr uint32_t threshold = 10000;
    EventWithoutThresholdTest(controller, event, threshold);
}

/**
 * @tc.name: DailyControllerTest013
 * @tc.desc: test OnConfigUpdate.
 * @tc.type: FUNC
 * @tc.require: issuesIC4YXE
 */
HWTEST_F(DailyControllerTest, DailyControllerTest013, TestSize.Level1)
{
    DailyController controller(WORK_PATH, CONFIG_PATH);
    auto event = CreateEvent(TEST_DOMAIN, "UPDATE_NAME", SysEventCreator::FAULT);
    uint32_t thresholdOnBeta = 100; // 100: from event_threshold.json
    uint32_t thresholdOnCommercial = 20; // 100: from event_threshold.json
    uint32_t threshold = Parameter::IsBetaVersion() ? thresholdOnBeta : thresholdOnCommercial;
    EventThresholdTest(controller, event, threshold);

    controller.OnConfigUpdate(CONFIG_PATH2);
    thresholdOnBeta = 99; // 99: from event_threshold2.json, 200 - 100 - 1
    thresholdOnCommercial = 19; // 19: from event_threshold2.json, 40 - 20 - 1
    threshold = Parameter::IsBetaVersion() ? thresholdOnBeta : thresholdOnCommercial;
    EventThresholdTest(controller, event, threshold);
}
