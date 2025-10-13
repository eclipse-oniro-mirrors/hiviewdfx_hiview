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

#include "file_util.h"
#include "hiview_global.h"
#include "hiview_logger.h"
#include "sys_event_sequence_mgr.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("SysEventSequenceMgrTest");
using namespace  OHOS::HiviewDFX::EventStore;
namespace {
constexpr char TEST_LOG_DIR[] = "/data/test/SysEventSequenceMgrDir";
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

class HiviewTestContext : public HiviewContext {
public:
    std::string GetHiViewDirectory(DirectoryType type __UNUSED)
    {
        return TEST_LOG_DIR;
    }
};
    
std::string GetLogDir()
{
    std::string workPath = HiviewGlobal::GetInstance()->GetHiViewDirectory(
        HiviewContext::DirectoryType::CONFIG_DIRECTORY);
    if (workPath.back() != '/') {
        workPath = workPath + "/";
    }
    if (!FileUtil::FileExists(workPath)) {
        FileUtil::ForceCreateDirectory(workPath, FileUtil::FILE_PERM_770);
    }
    return workPath;
}

void UpdateThenReadSeqFromFile(int64_t seq, int64_t seqBackup, int64_t& curSeqRead, int64_t& startSeqRead)
{
    HiviewTestContext hiviewTestContext;
    HiviewGlobal::CreateInstance(hiviewTestContext);
    std::string eventSeqFilePath = GetLogDir() + "sys_event_db/event_sequence";
    FileUtil::SaveStringToFile(eventSeqFilePath, std::to_string(seq));
    std::string eventSeqBackupFilePath = GetLogDir() + "sys_event_db/event_sequence_backup";
    FileUtil::SaveStringToFile(eventSeqBackupFilePath, std::to_string(seqBackup));
    curSeqRead = EventStore::SysEventSequenceManager::GetInstance().GetSequence();
    startSeqRead = EventStore::SysEventSequenceManager::GetInstance().GetStartSequence();
}

/**
 * @tc.name: SysEventSequenceMgrTest001
 * @tc.desc: test reading sequence from normal sequence files
 * @tc.type: FUNC
 * @tc.require: issueIBT9BB
 */
HWTEST_F(SysEventSequenceMgrTest, SysEventSequenceMgrTest001, testing::ext::TestSize.Level3)
{
    int64_t curSeq = 0;
    int64_t startSeq = 0;
    UpdateThenReadSeqFromFile(100, 0, curSeq, startSeq); // 100 is normal difference between two sequence files
    ASSERT_EQ(curSeq, 200); // current sequence is expected to be 200
    ASSERT_EQ(curSeq, startSeq);
}

/**
 * @tc.name: SysEventSequenceMgrTest002
 * @tc.desc: test reading sequence from abnormal sequence files
 * @tc.type: FUNC
 * @tc.require: issue#2986
 */
HWTEST_F(SysEventSequenceMgrTest, SysEventSequenceMgrTest002, testing::ext::TestSize.Level3)
{
    int64_t curSeq = 0;
    int64_t startSeq = 0;
    UpdateThenReadSeqFromFile(101, 0, curSeq, startSeq); // 101 is abnormal difference between two sequence files
    ASSERT_GT(curSeq, 0); // current sequence is expected to be greater than 0
    ASSERT_EQ(curSeq, startSeq);
}

/**
 * @tc.name: SysEventSequenceMgrTest003
 * @tc.desc: test apis of class SysEventSequenceManager
 * @tc.type: FUNC
 * @tc.require: issueI9U6IV
 */
HWTEST_F(SysEventSequenceMgrTest, SysEventSequenceMgrTest003, testing::ext::TestSize.Level3)
{
    HiviewTestContext hiviewTestContext;
    HiviewGlobal::CreateInstance(hiviewTestContext);
    auto eventSeq = EventStore::SysEventSequenceManager::GetInstance().GetSequence();
    EventStore::SysEventSequenceManager::GetInstance().SetSequence(eventSeq + 1000); // 1000 is a test offset
    auto eventSeqNew = EventStore::SysEventSequenceManager::GetInstance().GetSequence();
    ASSERT_NE(eventSeq, eventSeqNew);
}
} // namespace HiviewDFX
} // namespace OHOS