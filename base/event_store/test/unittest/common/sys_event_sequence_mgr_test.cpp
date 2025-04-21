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

#include "sys_event_sequence_mgr_test.h"

#include <gmock/gmock.h>

#include "hiview_logger.h"
#include "sys_event_sequence_mgr.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("SysEventSequenceMgrTest");
using namespace  OHOS::HiviewDFX::EventStore;
namespace {
}

void SysEventSequenceMgrTest::SetUpTestCase()
{
}

void SysEventSequenceMgrTest::TearDownTestCase()
{
}

void SysEventSequenceMgrTest::SetUp()
{
}

void SysEventSequenceMgrTest::TearDown()
{
}

/**
 * @tc.name: SysEventSequenceMgrTest001
 * @tc.desc: test apis of class SysEventSequenceManager
 * @tc.type: FUNC
 * @tc.require: issueI9U6IV
 */
HWTEST_F(SysEventSequenceMgrTest, SysEventSequenceMgrTest001, testing::ext::TestSize.Level3)
{
    auto eventSeq = EventStore::SysEventSequenceManager::GetInstance().GetSequence();
    int64_t startSeq = EventStore::SysEventSequenceManager::GetInstance().GetStartSequence();
    ASSERT_EQ(eventSeq, startSeq);

    EventStore::SysEventSequenceManager::GetInstance().SetSequence(eventSeq + 1000); // 1000 is a test offset
    auto eventSeqNew = EventStore::SysEventSequenceManager::GetInstance().GetSequence();
    ASSERT_NE(eventSeq, eventSeqNew);
}
} // namespace HiviewDFX
} // namespace OHOS