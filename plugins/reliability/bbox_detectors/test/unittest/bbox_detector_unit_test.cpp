/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include <fstream>
#include "bbox_detector_unit_test.h"

#include "bbox_detector_plugin.h"

#include "bbox_detectors_mock.h"
#include "bbox_event_recorder.h"
#include "hisysevent_util_mock.h"
#include "panic_report_recovery.h"
#include "smart_parser.h"
#include "sys_event.h"
#include "sys_event_dao.h"
#include "tbox.h"
using namespace std;

namespace OHOS {
namespace HiviewDFX {
using namespace testing;
using namespace testing::ext;
void BBoxDetectorUnitTest::SetUpTestCase(void) {}

void BBoxDetectorUnitTest::TearDownTestCase(void) {}

void BBoxDetectorUnitTest::SetUp(void)
{
    FileUtil::ForceCreateDirectory("/data/test/bbox/panic_log/");
    FileUtil::ForceCreateDirectory("/data/test/bbox/ap_log/");
}

void BBoxDetectorUnitTest::TearDown(void)
{
    FileUtil::ForceRemoveDirectory("/data/test/bbox/");
}

void GenerateFile(const std::string &path, unsigned int size)
{
    constexpr int bufferSize = 1024;
    constexpr int charSize = 26;
    std::ofstream ofs;
    ofs.open(path, std::ios::out | std::ios::trunc);
    for (unsigned int i = 0; i < size; i++) {
        for (int j = 0; j < bufferSize; ++j) {
            ofs << static_cast<char>(rand() % charSize + 'a');
        }
    }
    ofs << std::endl;
    ofs.close();
}

/**
 * @tc.name: BBoxDetectorUnitTest001
 * @tc.desc: check bbox config parser whether it is passed.
 *           1.parse bbox config;
 *           2.check result;
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST_F(BBoxDetectorUnitTest, BBoxDetectorUnitTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. init bboxDetector and parse hisysevent.
     */

    /**
     * @tc.steps: step2. check func.
     * @tc.expect: get right result for checking
     */
    SysEventCreator sysEventCreator("KERNEL_VENDOR", "PANIC", SysEventCreator::FAULT);
    auto sysEvent = make_shared<SysEvent>("test", nullptr, sysEventCreator);
    auto testPlugin = make_shared<BBoxDetectorPlugin>();
    shared_ptr<Event> event = dynamic_pointer_cast<Event>(sysEvent);
    EXPECT_EQ(testPlugin->CanProcessEvent(event), true);
}

/**
 * @tc.name: BBoxDetectorUnitTest008
 * @tc.desc: check whether the plugin initialize success.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(BBoxDetectorUnitTest, BBoxDetectorUnitTest008, TestSize.Level1)
{
    auto testPlugin = make_shared<BBoxDetectorPlugin>();
    testPlugin->AddBootScanEvent();
    testPlugin->AddDetectBootCompletedTask();
    testPlugin->workLoop_ = std::make_shared<EventLoop>("test");
    testPlugin->AddBootScanEvent();
    testPlugin->AddDetectBootCompletedTask();
    testPlugin->NotifyBootStable();
    testPlugin->NotifyBootCompleted();
    ASSERT_NE(testPlugin->workLoop_, nullptr);
}

/**
 * @tc.name: BboxEventRecorder001
 * @tc.desc: check BboxEventRecorder.
 * @tc.type: FUNC
 */
HWTEST(BboxEventRecorderTets, BboxEventRecorder001, TestSize.Level1)
{
    BboxEventRecorder recorder;
    std::string event = "MODEMCRASH";
    time_t now = time(nullptr);
    std::string logPath = "/root/testDir";

    ASSERT_FALSE(recorder.IsExistEvent(event, now, logPath));
    ASSERT_TRUE(recorder.AddEventToMaps(event, now, logPath));
    ASSERT_TRUE(recorder.IsExistEvent(event, now, logPath));

    logPath = "/root/testDir2";
    ASSERT_FALSE(recorder.IsExistEvent(event, now, logPath));
}

std::string LocalTimeFormat()
{
    time_t t = time(nullptr);
    struct tm *tm_info = localtime(&t);
    const size_t bufferLen = 16;
    const size_t resultLen = 14;
    char buffer[bufferLen] = {0};
    if (strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", tm_info) != resultLen) {
        return "";
    }
    return std::string(buffer);
}
}  // namespace HiviewDFX
}  // namespace OHOS
