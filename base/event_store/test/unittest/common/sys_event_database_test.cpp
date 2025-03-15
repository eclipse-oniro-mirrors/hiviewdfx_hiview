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
#include "sys_event_database_test.h"

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
#include "sys_event_database.h"

namespace OHOS {
namespace HiviewDFX {
void SysEventDatabaseTest::SetUpTestCase()
{
}

void SysEventDatabaseTest::TearDownTestCase()
{
}

void SysEventDatabaseTest::SetUp()
{
}

void SysEventDatabaseTest::TearDown()
{
}

/**
 * @tc.name: EventDatabaseTest001
 * @tc.desc: test the class of SysEventDatabase.
 * @tc.type: FUNC
 */
HWTEST_F(SysEventDatabaseTest, EventDatabaseTest001, testing::ext::TestSize.Level0)
{
    SysEventCreator sysEventCreator("WINDOWMANAGER", "NO_FOCUS_WINDOW", SysEventCreator::STATISTIC);
    std::vector<int> values = {1, 2, 3};
    sysEventCreator.SetKeyValue("test1", values);
    time_t now = time(nullptr);
    sysEventCreator.SetKeyValue("test2", now);
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    sysEvent->SetLevel("CRITICAL");
    int64_t testSeq = 0;
    sysEvent->SetEventSeq(testSeq);
    ASSERT_EQ(EventStore::SysEventDatabase::GetInstance().Insert(sysEvent), 0);
    EventStore::SysEventDatabase::GetInstance().Clear();
}
} // namespace HiviewDFX
} // namespace OHOS