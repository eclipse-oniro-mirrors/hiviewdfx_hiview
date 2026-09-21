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

#include <cstdio>
#include <fstream>
#include <memory>
#include <string>

#include "faultlog_undeletable.h"
#include "faultlog_util.h"
#include "test_utils.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
/**
 * @tc.name: FaultLogUndeletable001
 * @tc.desc: Test FaultLogUndeletable ReportToAppEvent always returns false
 * @tc.type: FUNC
 */
HWTEST(FaultLogUndeletableTest, FaultLogUndeletable001, testing::ext::TestSize.Level3)
{
    FaultLogUndeletable handler;
    SysEventCreator sysEventCreator("RELIABILITY", "FILE_UNDELETEABLE", SysEventCreator::FAULT);
    auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    EXPECT_FALSE(handler.ReportToAppEvent(sysEvent));
    EXPECT_FALSE(handler.ReportToAppEvent(nullptr));
}

/**
 * @tc.name: FaultLogUndeletable002
 * @tc.desc: Test IsValidUndeletableEvent reject branches: reason mismatch and uid mismatch
 * @tc.type: FUNC
 */
HWTEST(FaultLogUndeletableTest, FaultLogUndeletable002, testing::ext::TestSize.Level3)
{
    FaultLogUndeletable handler;
    SysEventCreator reasonCreator("RELIABILITY", "FILE_UNDELETEABLE", SysEventCreator::FAULT);
    reasonCreator.SetKeyValue("REASON", "Signal:DEBUG SIGNAL(FDSAN)");
    auto reasonEvent = std::make_shared<SysEvent>("test", nullptr, reasonCreator);
    EXPECT_FALSE(handler.IsValidUndeletableEvent(*reasonEvent));

    constexpr int32_t mismatchUid = 456; // 456 : simulated uid mismatch value
    SysEventCreator uidCreator("RELIABILITY", "FILE_UNDELETEABLE", SysEventCreator::FAULT);
    uidCreator.SetKeyValue("REASON", "Signal:DEBUG SIGNAL(UNDELETEABLE) inode_id:12345");
    uidCreator.SetKeyValue("UID", mismatchUid);
    auto uidEvent = std::make_shared<SysEvent>("test", nullptr, uidCreator);
    EXPECT_FALSE(handler.IsValidUndeletableEvent(*uidEvent));
}

/**
 * @tc.name: FaultLogUndeletable003
 * @tc.desc: Test IsValidUndeletableEvent with temp file and FillFaultLogInfo fields
 * @tc.type: FUNC
 */
HWTEST(FaultLogUndeletableTest, FaultLogUndeletable003, testing::ext::TestSize.Level3)
{
    constexpr int32_t testPid = 4321;          // 4321 : simulated pid
    constexpr int64_t testTime = 1607161345;   // 1607161345 : simulated happen time
    constexpr int32_t testInodeId = 12345;     // 12345 : simulated inode id
    FaultLogUndeletable handler;
    SysEventCreator creator("RELIABILITY", "FILE_UNDELETEABLE", SysEventCreator::FAULT);
    creator.SetKeyValue("REASON", "Signal:DEBUG SIGNAL(UNDELETEABLE) inode_id:12345");
    creator.SetKeyValue("MODULE", "test_undeletable");
    creator.SetKeyValue("PID", testPid);
    creator.SetKeyValue("HAPPEN_TIME", testTime);
    creator.SetKeyValue("INODE_ID", testInodeId);
    auto sysEvent = std::make_shared<SysEvent>("test", nullptr, creator);

    std::string tempPath = GetUndeletableTempLogName(testPid, testTime);
    EXPECT_FALSE(handler.IsValidUndeletableEvent(*sysEvent));

    std::ofstream outFile(tempPath);
    outFile << "Reason:Signal:DEBUG SIGNAL(UNDELETEABLE) inode_id:12345\n";
    outFile.close();
    EXPECT_TRUE(handler.IsValidUndeletableEvent(*sysEvent));

    auto info = handler.FillFaultLogInfo(*sysEvent);
    EXPECT_EQ(info.pid, testPid);
    EXPECT_EQ(info.time, testTime);
    EXPECT_EQ(info.logPath, tempPath);
    EXPECT_TRUE(info.summary.empty());
    EXPECT_FALSE(info.dumpLogToFaultlogger);
    EXPECT_FALSE(info.reportToAppEvent);
    EXPECT_EQ(handler.GetFaultModule(*sysEvent), "test_undeletable");
    (void)std::remove(tempPath.c_str());
}
} // namespace HiviewDFX
} // namespace OHOS
