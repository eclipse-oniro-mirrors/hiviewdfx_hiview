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

/**
 * @tc.name: SysEventSequenceMgrTest001
 * @tc.desc: test apis of class SysEventSequenceManager
 * @tc.type: FUNC
 * @tc.require: issueIBT9BB
 */
HWTEST_F(SysEventSequenceMgrTest, SysEventSequenceMgrTest001, testing::ext::TestSize.Level3)
{
    HiviewTestContext hiviewTestContext;
    HiviewGlobal::CreateInstance(hiviewTestContext);
    std::string eventSeqFilePath = GetLogDir() + "sys_event_db/event_sequence";
    FileUtil::SaveStringToFile(eventSeqFilePath, "0");
    std::string eventSeqBackupFilePath = GetLogDir() + "sys_event_db/event_sequence_backup";
    FileUtil::SaveStringToFile(eventSeqBackupFilePath, "1000");
    ASSERT_EQ(EventStore::SysEventSequenceManager::GetInstance().GetSequence(), 1100); // 1100 is expected seq value
}

/**
 * @tc.name: SysEventSequenceMgrTest002
 * @tc.desc: test apis of class SysEventSequenceManager
 * @tc.type: FUNC
 * @tc.require: issueI9U6IV
 */
HWTEST_F(SysEventSequenceMgrTest, SysEventSequenceMgrTest002, testing::ext::TestSize.Level3)
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