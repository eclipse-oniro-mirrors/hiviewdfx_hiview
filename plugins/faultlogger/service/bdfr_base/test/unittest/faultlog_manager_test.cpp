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

#include "faultlog_manager.h"
#include "faultlog_formatter.h"
#include "faultlog_util.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
/**
 * @tc.name: FaultLogManager::CreateTempFaultLogFile
 * @tc.desc: Test calling CreateTempFaultLogFile Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogManagerTest, FaultlogManager001, testing::ext::TestSize.Level3)
{
    FaultLogManager faultLogManager;
    faultLogManager.Init();
    int fd = faultLogManager.CreateTempFaultLogFile(1607161345, 0, 2, "FaultloggerUnittest");
    ASSERT_GT(fd, 0);
    std::string content = "testContent";
    TEMP_FAILURE_RETRY(write(fd, content.data(), content.length()));
    close(fd);
}

/**
 * @tc.name: FaultLogManager::FaultlogManager
 * @tc.desc: Test calling FaultlogManager Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogManagerTest, FaultlogManager002, testing::ext::TestSize.Level3)
{
    FaultLogManager faultLogManager;
    std::list<std::string> infoVec = {"1", "2", "3", "4", "5"};
    faultLogManager.ReduceLogFileListSize(infoVec, 1);
    ASSERT_EQ(infoVec.size(), 1);
}

/**
 * @tc.name: FaultLogManager::GetFaultLogFileList
 * @tc.desc: Test calling GetFaultLogFileList Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogManagerTest, GetFaultLogFileList001, testing::ext::TestSize.Level3)
{
    FaultLogManager faultLogManager;
    faultLogManager.Init();
    std::list<std::string> fileList = faultLogManager.GetFaultLogFileList("FaultloggerUnittest", 1607161344, 0, 2, 1);
    ASSERT_EQ(fileList.size(), 1);
}

/**
 * @tc.name: FaultLogManager::WriteFaultLogToFile
 * @tc.desc: Test calling WriteFaultLogToFile Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogManagerTest, WriteFaultLogToFile001, testing::ext::TestSize.Level3)
{
    FaultLogManager faultLogManager;
    faultLogManager.Init();
    FaultLogInfo info {
        .time = 1607161345,
        .id = 0,
        .faultLogType = 2,
        .module = ""
    };
    info.faultLogType = FaultLogType::JS_CRASH;
    FaultLogger::WriteFaultLogToFile(0, info.faultLogType, info.sectionMap);
    info.faultLogType = FaultLogType::SYS_FREEZE;
    FaultLogger::WriteFaultLogToFile(0, info.faultLogType, info.sectionMap);
    info.faultLogType = FaultLogType::SYS_WARNING;
    FaultLogger::WriteFaultLogToFile(0, info.faultLogType, info.sectionMap);
    info.faultLogType = FaultLogType::APPFREEZE_WARNING;
    FaultLogger::WriteFaultLogToFile(0, info.faultLogType, info.sectionMap);
    info.faultLogType = FaultLogType::RUST_PANIC;
    FaultLogger::WriteFaultLogToFile(0, info.faultLogType, info.sectionMap);
    info.faultLogType = FaultLogType::ADDR_SANITIZER;
    FaultLogger::WriteFaultLogToFile(0, info.faultLogType, info.sectionMap);
    info.faultLogType = FaultLogType::ADDR_SANITIZER;
    FaultLogger::WriteFaultLogToFile(0, info.faultLogType, info.sectionMap);
    info.faultLogType = FaultLogType::ALL;
    FaultLogger::WriteFaultLogToFile(0, info.faultLogType, info.sectionMap);
    ASSERT_EQ(info.pid, 0);
}

/**
 * @tc.name: FaultLogManager::GetFaultLogContent
 * @tc.desc: Test calling GetFaultLogContent Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogManagerTest, GetFaultLogContent001, testing::ext::TestSize.Level3)
{
    FaultLogManager faultLogManager;
    faultLogManager.Init();
    FaultLogInfo info {
        .time = 1607161345,
        .id = 0,
        .faultLogType = 2,
        .module = "FaultloggerUnittest"
    };
    std::string fileName = GetFaultLogName(info);
    std::string content;
    ASSERT_TRUE(faultLogManager.GetFaultLogContent(fileName, content));
    ASSERT_EQ(content, "testContent");
}

std::string GetTargetFileName(int32_t faultLogType, int64_t time)
{
    std::map<int, std::string> fileNames = {
        {1, "Unknown"},
        {2, "cppcrash"}, // 2 : faultLogType to cppcrash
        {3, "jscrash"}, // 3 : faultLogType to jscrash
        {4, "appfreeze"}, // 4 : faultLogType to appfreeze
        {5, "sysfreeze"}, // 5 : faultLogType to sysfreeze
        {6, "syswarning"}, // 6 : faultLogType to syswarning
        {7, "appfreezewarning"}, // 7 : faultLogType to appfreezewarning
        {8, "rustpanic"}, // 8 : faultLogType to rustpanic
        {9, "cjerror"}, // 9 : faultLogType to cjerror
        {10, "sanitizer"}, // 9 : faultLogType to sanitizer
    };
    std::string fileName = fileNames[faultLogType];
    return fileName + "-FaultloggerUnittest1111-0-" + GetFormatedTimeWithMillsec(time) + ".log";
}

/**
 * @tc.name: FaultLogManager::SaveFaultLogToFile
 * @tc.desc: Test calling SaveFaultLogToFile Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogManagerTest, FaultLogManagerTest003, testing::ext::TestSize.Level3)
{
    FaultLogInfo info;
    FaultLogManager faultLogManager;
    faultLogManager.Init();
    for (int i = 1; i <= 10; i++) {
        info.time = std::time(nullptr);
        info.pid = getpid();
        info.id = 0;
        info.faultLogType = i;
        info.module = "FaultloggerUnittest1111";
        info.reason = "unittest for SaveFaultLogInfo";
        info.summary = "summary for SaveFaultLogInfo";
        info.sectionMap["APPVERSION"] = "1.0";
        info.sectionMap["FAULT_MESSAGE"] = "abort";
        info.sectionMap["TRACEID"] = "0x1646145645646";
        info.sectionMap["KEY_THREAD_INFO"] = "Test Thread Info";
        info.sectionMap["REASON"] = "TestReason";
        info.sectionMap["STACKTRACE"] = "#01 xxxxxx\n#02 xxxxxx\n";

        std::string fileName = faultLogManager.SaveFaultLogToFile(info);
        if (fileName.find("FaultloggerUnittest1111") == std::string::npos) {
            FAIL();
        }
        std::string targetFileName = GetTargetFileName(i, info.time);
        ASSERT_EQ(fileName, targetFileName);
    }
}

/**
 * @tc.name: FaultLogManager::GetFaultLogFilePathTest001
 * @tc.desc: Test calling GetFaultLogFilePath Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogManagerTest, GetFaultLogFilePathTest001, testing::ext::TestSize.Level3)
{
    FaultLogManager faultLogManager;
    faultLogManager.Init();
    std::string fileName = "com.freeze.test001-202506023.log";

    std::string faultLogFilePath = faultLogManager.GetFaultLogFilePath(FaultLogType::APP_FREEZE, fileName);
    ASSERT_EQ(faultLogFilePath, "/data/log/faultlog/faultlogger/com.freeze.test001-202506023.log");

    faultLogFilePath = faultLogManager.GetFaultLogFilePath(FaultLogType::SYS_WARNING, fileName);
    ASSERT_EQ(faultLogFilePath, "/data/log/warninglog/com.freeze.test001-202506023.log");
    faultLogFilePath = faultLogManager.GetFaultLogFilePath(FaultLogType::APPFREEZE_WARNING, fileName);
    ASSERT_EQ(faultLogFilePath, "/data/log/warninglog/com.freeze.test001-202506023.log");
}

/**
 * @tc.name: FaultLogManager::GetFaultLogFileFdTest001
 * @tc.desc: Test calling GetFaultLogFileFd Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogManagerTest, GetFaultLogFileFdTest001, testing::ext::TestSize.Level3)
{
    FaultLogManager faultLogManager;
    int faultLogFileFd = -1;
    faultLogManager.Init();
    std::string fileName = "com.freeze.test001-202506023.log";

    faultLogFileFd = faultLogManager.GetFaultLogFileFd(FaultLogType::APP_FREEZE, fileName);
    ASSERT_TRUE(faultLogFileFd > 0);
    close(faultLogFileFd);

    faultLogFileFd = faultLogManager.GetFaultLogFileFd(FaultLogType::SYS_WARNING, fileName);
    ASSERT_TRUE(faultLogFileFd > 0);
    close(faultLogFileFd);

    faultLogFileFd = faultLogManager.GetFaultLogFileFd(FaultLogType::APPFREEZE_WARNING, fileName);
    ASSERT_TRUE(faultLogFileFd > 0);
    close(faultLogFileFd);
}

} // namespace HiviewDFX
} // namespace OHOS
