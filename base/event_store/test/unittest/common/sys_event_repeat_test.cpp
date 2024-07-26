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
#include "sys_event_repeat_test.h"

#include <chrono>
#include <ctime>
#include <iostream>
#include <limits>
#include <memory>
#include <thread>

#include <gmock/gmock.h>
#include "event.h"
#include "hiview_global.h"
#include "sys_event.h"
#include "sys_event_dao.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr char TEST_PATH[] = "/data/text/";
class TestContext : public HiviewContext {
public:
    std::string GetHiViewDirectory(HiviewContext::DirectoryType type)
    {
        return TEST_PATH;
    }
};
}
void SysEventRepeatTest::SetUpTestCase()
{
}

void SysEventRepeatTest::TearDownTestCase()
{
}

void SysEventRepeatTest::SetUp()
{
}

void SysEventRepeatTest::TearDown()
{
}

/**
 * @tc.name: CheckEventRepeatTest_01
 * @tc.desc: test the function of CheckEventRepeat.
 * @tc.type: FUNC
 */
HWTEST_F(SysEventRepeatTest, CheckEventRepeatTest_01, testing::ext::TestSize.Level0)
{
    TestContext context;
    HiviewGlobal::CreateInstance(context);
    SysEventCreator sysEventCreator("WINDOWMANAGER", "NO_FOCUS_WINDOW", SysEventCreator::FAULT);
    std::vector<int> values = {1, 2, 3};
    sysEventCreator.SetKeyValue("KEY", values);
    time_t now = time(nullptr);
    sysEventCreator.SetKeyValue("testTime", now);
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    sysEvent->SetLevel("CRITICAL");
    int64_t testSeq = 0;
    sysEvent->SetEventSeq(testSeq);
    EventStore::SysEventDao::CheckRepeat(sysEvent);
    ASSERT_EQ(sysEvent->log_, LOG_ALLOW_PACK|LOG_PACKED);

    EventStore::SysEventDao::Insert(sysEvent);
    std::shared_ptr<SysEvent> repeatSysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    testSeq++;
    repeatSysEvent->SetLevel("CRITICAL");
    repeatSysEvent->SetEventSeq(testSeq);
    EventStore::SysEventDao::CheckRepeat(repeatSysEvent);
    ASSERT_EQ(repeatSysEvent->log_, LOG_NOT_ALLOW_PACK|LOG_REPEAT);
}
} // namespace HiviewDFX
} // namespace OHOS