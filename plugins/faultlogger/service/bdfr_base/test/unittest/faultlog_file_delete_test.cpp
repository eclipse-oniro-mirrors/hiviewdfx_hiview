/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "faultlog_file_delete.h"
#include "faultlog_util.h"
#include "test_utils.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
/**
 * @tc.name: FaultLogFileDelete001
 * @tc.desc: Test FaultLogFileDelete ReportToAppEvent always returns false
 * @tc.type: FUNC
 */
HWTEST(FaultLogFileDeleteTest, FaultLogFileDelete001, testing::ext::TestSize.Level3)
{
    FaultLogFileDelete handler;
    SysEventCreator sysEventCreator("RELIABILITY", "FILE_DELETED", SysEventCreator::FAULT);
    auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    EXPECT_FALSE(handler.ReportToAppEvent(sysEvent));
    EXPECT_FALSE(handler.ReportToAppEvent(nullptr));
}

/**
 * @tc.name: FaultLogFileDelete002
 * @tc.desc: Test FillFaultLogInfo fields of FaultLogFileDelete
 * @tc.type: FUNC
 */
HWTEST(FaultLogFileDeleteTest, FaultLogFileDelete002, testing::ext::TestSize.Level3)
{
    constexpr int32_t testPid = 4321;          // 4321 : simulated pid
    constexpr int64_t testTime = 1607161345;   // 1607161345 : simulated happen time
    constexpr int32_t testInodeId = 88888;     // 88888 : simulated inode id
    FaultLogFileDelete handler;
    SysEventCreator creator("RELIABILITY", "FILE_DELETED", SysEventCreator::FAULT);
    creator.SetKeyValue("REASON", "Signal:DEBUG SIGNAL(FILE_DELETE) inode_id:88888");
    creator.SetKeyValue("MODULE", "test_filedelete");
    creator.SetKeyValue("PID", testPid);
    creator.SetKeyValue("HAPPEN_TIME", testTime);
    creator.SetKeyValue("INODE_ID", testInodeId);
    creator.SetKeyValue("SUMMARY", "Thread name:test\n#00 pc 00001234 /system/lib/test.so");
    auto sysEvent = std::make_shared<SysEvent>("test", nullptr, creator);

    auto info = handler.FillFaultLogInfo(*sysEvent);
    EXPECT_EQ(info.pid, testPid);
    EXPECT_EQ(info.time, testTime);
    EXPECT_EQ(info.logPath, "/data/log/faultlog/temp/stacktrace-filedelete-4321-1607161345");
    EXPECT_FALSE(info.summary.empty());
    EXPECT_FALSE(info.dumpLogToFaultlogger);
    EXPECT_FALSE(info.reportToAppEvent);
    EXPECT_EQ(handler.GetFaultModule(*sysEvent), "test_filedelete");
}
} // namespace HiviewDFX
} // namespace OHOS
