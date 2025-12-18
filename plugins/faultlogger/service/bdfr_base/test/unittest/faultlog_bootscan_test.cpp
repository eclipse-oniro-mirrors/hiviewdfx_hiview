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
#include <cerrno>
#include <cstring>
#include <fstream>
#include <gtest/gtest.h>
#include <ctime>
#include <unistd.h>

#include "faultevent_listener.h"
#include "faultlog_bootscan.h"
#include "faultlog_formatter.h"
#include "faultlog_util.h"
#include "file_util.h"
#include "hisysevent_manager.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {

/**
 * @tc.name: FaultLogBootScan001
 * @tc.desc: Test big file
 * @tc.type: FUNC
 */
HWTEST(FaultLogBootScanTest, FaultLogBootScan001, testing::ext::TestSize.Level3)
{
    std::string path = "/data/test/test_data/FaultLogBootScan001/cppcrash-197-1502809621426";
    ASSERT_FALSE(FaultLogBootScan::IsCrashTempBigFile(path));
}

/**
 * @tc.name: FaultLogBootScan002
 * @tc.desc: Test big file, file size exceeds limit
 * @tc.type: FUNC
 */
HWTEST(FaultLogBootScanTest, FaultLogBootScan002, testing::ext::TestSize.Level3)
{
    std::string path = "/data/test/test_data/FaultLogBootScan002/cppcrash-197-1502809621426";
    std::ofstream file(path, std::ios::app);
    ASSERT_TRUE(file.is_open()) << std::strerror(errno);
    file << std::setfill('0') << std::setw(1024 * 1024 * 5) << 0;
    file.close();

    ASSERT_TRUE(FaultLogBootScan::IsCrashTempBigFile(path));
}

/**
 * @tc.name: IsCrashTypeTest001
 * @tc.desc: Test calling IsCrashType Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogBootScanTest, IsCrashTypeTest001, testing::ext::TestSize.Level3)
{
    std::string fileName = "/data/log/faultlog/temp/freeze-114-";
    bool ret = FaultLogBootScan::IsCrashType(fileName);
    ASSERT_EQ(ret, false);
}

/**
 * @tc.name: IsInValidTimeTest001
 * @tc.desc: Test calling IsCrashType Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogBootScanTest, IsInValidTimeTest001, testing::ext::TestSize.Level3)
{
    std::string fileName = "/data/log/faultlog/temp/freeze-114-";
    time_t now = time(nullptr);
    FaultLogBootScan::IsInValidTime(fileName, now);
    ASSERT_TRUE(FaultLogBootScan::IsInValidTime(fileName, 0));
}

static std::shared_ptr<FaultEventListener> faultEventListener = nullptr;

static void StartHisyseventListen(std::string domain, std::string eventName)
{
    faultEventListener = std::make_shared<FaultEventListener>();
    ListenerRule tagRule(domain, eventName, RuleType::WHOLE_WORD);
    std::vector<ListenerRule> sysRules = {tagRule};
    HiSysEventManager::AddListener(faultEventListener, sysRules);
}

/**
 * @tc.name: FaultloggerTest001
 * @tc.desc: Test calling Faultlogger.StartBootScan Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogBootScanTest, FaultloggerTest001, testing::ext::TestSize.Level3)
{
    StartHisyseventListen("RELIABILITY", "CPP_CRASH");
    time_t now = time(nullptr);
    std::vector<std::string> keyWords = { std::to_string(now) };
    faultEventListener->SetKeyWords(keyWords);
    std::string timeStr = GetFormatedTimeWithMillsec(now);
    std::string content = "Pid:101\nUid:0\nProcess name:BootScanUnittest\nReason:unittest for StartBootScan\n"
        "Fault thread info:\nTid:101, Name:BootScanUnittest\n#00 xxxxxxx\n#01 xxxxxxx\n";
    ASSERT_TRUE(FileUtil::SaveStringToFile("/data/log/faultlog/temp/cppcrash-101-" + std::to_string(now), content));
    FaultLogBootScan::StartBootScan();

    // check faultlog file content
    std::string fileName = "/data/log/faultlog/faultlogger/cppcrash-BootScanUnittest-0-" + timeStr + ".log";
    ASSERT_TRUE(FileUtil::FileExists(fileName));
    ASSERT_GT(FileUtil::GetFileSize(fileName), 0ul);

    // check event database
    ASSERT_TRUE(faultEventListener->CheckKeyWords());
    sleep(1);
}

/**
 * @tc.name: FaultloggerTest002
 * @tc.desc: Test calling Faultlogger.StartBootScan Func
 * @tc.type: FUNC
 */
HWTEST(FaultLogBootScanTest, FaultloggerTest002, testing::ext::TestSize.Level3)
{
    StartHisyseventListen("RELIABILITY", "CPP_CRASH_NO_LOG");
    std::vector<std::string> keyWords = { "BootScanUnittest" };
    faultEventListener->SetKeyWords(keyWords);
    time_t now = time(nullptr);
    std::string content = "Pid:102\nUid:0\nProcess name:BootScanUnittest\nReason:unittest for StartBootScan\n"
        "Fault thread info:\nTid:102, Name:BootScanUnittest\n";
    std::string fileName = "/data/log/faultlog/temp/cppcrash-102-" + std::to_string(now);
    ASSERT_TRUE(FileUtil::SaveStringToFile(fileName, content));
    FaultLogBootScan::StartBootScan();
    ASSERT_FALSE(FileUtil::FileExists(fileName));

    // check event database
    ASSERT_TRUE(faultEventListener->CheckKeyWords());
    sleep(1);
}

/**
 * @tc.name: FaultloggerTest003
 * @tc.desc: Test calling Faultlogger.StartBootScan Func, for full log
 * @tc.type: FUNC
 */
HWTEST(FaultLogBootScanTest, FaultloggerTest003, testing::ext::TestSize.Level3)
{
    StartHisyseventListen("RELIABILITY", "CPP_CRASH");
    time_t now = time(nullptr);
    std::vector<std::string> keyWords = { std::to_string(now) };
    faultEventListener->SetKeyWords(keyWords);
    std::string timeStr = GetFormatedTimeWithMillsec(now);
    std::string regs = "r0:00000019 r1:0097cd3c\nr4:f787fd2c\nfp:f787fd18 ip:7fffffff pc:0097c982\n";
    std::string otherThreadInfo =
        "Tid:1336, Name:BootScanUnittes\n#00 xxxxxx\nTid:1337, Name:BootScanUnittes\n#00 xx\n";
    std::string content = std::string("Pid:111\nUid:0\nProcess name:BootScanUnittest\n") +
        "Reason:unittest for StartBootScan\n" +
        "Fault thread info:\nTid:111, Name:BootScanUnittest\n#00 xxxxxxx\n#01 xxxxxxx\n" +
        "Registers:\n" + regs +
        "Other thread info:\n" + otherThreadInfo +
        "Memory near registers:\nr1(/data/xxxxx):\n    0097cd34 47886849\n    0097cd38 96059d05\n\n" +
        "Maps:\n96e000-978000 r--p 00000000 /data/xxxxx\n978000-9a6000 r-xp 00009000 /data/xxxx\n";
    ASSERT_TRUE(FileUtil::SaveStringToFile("/data/log/faultlog/temp/cppcrash-111-" + std::to_string(now), content));
    FaultLogBootScan::StartBootScan();

    // check faultlog file content
    std::string fileName = "/data/log/faultlog/faultlogger/cppcrash-BootScanUnittest-0-" + timeStr + ".log";
    ASSERT_TRUE(FileUtil::FileExists(fileName));
    ASSERT_GT(FileUtil::GetFileSize(fileName), 0ul);

    // check regs and otherThreadInfo is ok
    std::string logInfo;
    FileUtil::LoadStringFromFile(fileName, logInfo);
    ASSERT_TRUE(logInfo.find(regs) != std::string::npos);
    ASSERT_TRUE(logInfo.find(otherThreadInfo) != std::string::npos);

    // check event database
    ASSERT_TRUE(faultEventListener->CheckKeyWords());
    sleep(1);
}

/**
 * @tc.name: FaultloggerTest004
 * @tc.desc: Test calling Faultlogger.StartBootScan Func, for full cpp crash log limit
 * @tc.type: FUNC
 */
HWTEST(FaultLogBootScanTest, FaultloggerTest004, testing::ext::TestSize.Level3)
{
    StartHisyseventListen("RELIABILITY", "CPP_CRASH");
    time_t now = time(nullptr);
    std::vector<std::string> keyWords = { std::to_string(now) };
    faultEventListener->SetKeyWords(keyWords);
    std::string timeStr = GetFormatedTimeWithMillsec(now);
    std::string fillMapsContent = "96e000-978000 r--p 00000000 /data/xxxxx\n978000-9a6000 r-xp 00009000 /data/xxxx\n";
    std::string regs = "r0:00000019 r1:0097cd3c\nr4:f787fd2c\nfp:f787fd18 ip:7fffffff pc:0097c982\n";
    std::string otherThreadInfo =
        "Tid:1336, Name:BootScanUnittes\n#00 xxxxxx\nTid:1337, Name:BootScanUnittes\n#00 xx\n";
    std::string content = std::string("Pid:111\nUid:0\nProcess name:BootScanUnittest\n") +
        "Reason:unittest for StartBootScan\n" +
        "Fault thread info:\nTid:111, Name:BootScanUnittest\n#00 xxxxxxx\n#01 xxxxxxx\n" +
        "Registers:\n" + regs +
        "Other thread info:\n" + otherThreadInfo +
        "Memory near registers:\nr1(/data/xxxxx):\n    0097cd34 47886849\n    0097cd38 96059d05\n\n" +
        "Maps:\n96e000-978000 r--p 00000000 /data/xxxxx\n978000-9a6000 r-xp 00009000 /data/xxxx\n";
    // let content more than 512k, trigger loglimit
    for (int i = 0; i < 10000; i++) {
        content += fillMapsContent;
    }

    ASSERT_TRUE(FileUtil::SaveStringToFile("/data/log/faultlog/temp/cppcrash-114-" + std::to_string(now), content));
    FaultLogBootScan::StartBootScan();
    // check faultlog file content
    std::string fileName = "/data/log/faultlog/faultlogger/cppcrash-BootScanUnittest-0-" + timeStr + ".log";
    ASSERT_TRUE(FileUtil::FileExists(fileName));
    ASSERT_GT(FileUtil::GetFileSize(fileName), 0ul);
    if (FaultLogger::IsFaultLogLimit()) {
        ASSERT_LT(FileUtil::GetFileSize(fileName), 514 * 1024ul);
    } else {
        ASSERT_GT(FileUtil::GetFileSize(fileName), 512 * 1024ul);
    }
    // check event database
    ASSERT_TRUE(faultEventListener->CheckKeyWords());
    sleep(1);
}

} // namespace HiviewDFX
} // namespace OHOS
