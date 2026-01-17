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
#include <fstream>
#include <gtest/gtest.h>
#include <memory>

#include "bbox_detectors_base.h"
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
// using namespace testing;
using namespace testing::ext;
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
 * @tc.name: BBoxDetectorUnitTest002
 * @tc.desc: check whether fault is processed,and check whether fault file is pasered
 *           1. check whether fault file is pasered;
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST(BBoxDetectorBaseUnitTest, BBoxDetectorUnitTest002, testing::ext::TestSize.Level0)
{
    /**
     * @tc.steps: step1. construct panic file path
     * @tc.steps: step2. Analysis panic event
     * @tc.steps: step3. check result
     */
    string stack = R"("dump_backtrace+0x0/0x184"
                     "show_stack+0x2c/0x3c",
                     "dump_stack+0xc0/0x11c",
                     "panic+0x1cc/0x3dc",
                     "sysrq_handle_term+0x0/0x94",
                     "__handle_sysrq+0x15c/0x184",
                     "write_sysrq_trigger+0xb0/0xf4",
                     "proc_reg_write+0x94/0x120",
                     "vfs_write+0x184/0x380",
                     "ksys_write+0x74/0xc8",
                     "__arm64_sys_write+0x24/0x34",
                     "el0_svc_common+0x104/0x180",
                     "do_el0_svc+0x2c/0x3c",
                     "el0_svc+0x10/0x1c",
                     "vks_write+0x123/0xa6",
                     "el0_sync+0x180/0x1c0"
                    )";

    std::map<std::string, std::string> eventInfos;
    eventInfos.insert(std::pair("END_STACK", stack));
    eventInfos.insert(std::pair("PNAME", "PANIC"));
    eventInfos.insert(std::pair("Eventid", "901000002"));
    Tbox::FilterTrace(eventInfos);

    EXPECT_STREQ(eventInfos["FIRST_FRAME"].c_str(), "sysrq_handle_term+0x0/0x94");
    EXPECT_STREQ(eventInfos["SECOND_FRAME"].c_str(), "__handle_sysrq+0x15c/0x184");
}

/**
 * @tc.name: BBoxDetectorUnitTest003
 * @tc.desc: check the interface IsRecoveryPanicEvent.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST(BBoxDetectorBaseUnitTest, BBoxDetectorUnitTest003, TestSize.Level1)
{
    SysEventCreator sysEventCreator("KERNEL_VENDOR", "PANIC", SysEventCreator::FAULT);
    auto sysEvent = make_shared<SysEvent>("test", nullptr, sysEventCreator);
    sysEvent->SetEventValue("LOG_PATH", "/data/test/bbox/panic_log/test");
    ASSERT_TRUE(PanicReport::IsRecoveryPanicEvent(sysEvent));
    sysEvent->SetEventValue("LOG_PATH", "OTHERS");
    ASSERT_FALSE(PanicReport::IsRecoveryPanicEvent(sysEvent));
}

/**
 * @tc.name: BBoxDetectorUnitTest004
 * @tc.desc: check whether the param of isShortStartUp work.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST(BBoxDetectorBaseUnitTest, BBoxDetectorUnitTest004, TestSize.Level1)
{
    FileUtil::ForceCreateDirectory("/data/test/bbox");
    bool isLastStartUpShort;
    ASSERT_TRUE(PanicReport::InitPanicReport(isLastStartUpShort));
    PanicReport::IsLastShortStartUp(isLastStartUpShort);
    ASSERT_TRUE(PanicReport::InitPanicReport(isLastStartUpShort));
    ASSERT_TRUE(PanicReport::IsLastShortStartUp(isLastStartUpShort));
}

/**
 * @tc.name: BBoxDetectorUnitTest005
 * @tc.desc: check whether the file will be deleted when it is over limit.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST(BBoxDetectorBaseUnitTest, BBoxDetectorUnitTest005, TestSize.Level1)
{
    FileUtil::ForceCreateDirectory("/data/test/bbox/panic_log");
    constexpr const char* testFile = "/data/test/bbox/panic_log/test";
    constexpr const unsigned int testFileSize = 5 * 1024 + 1;
    GenerateFile(testFile, testFileSize);
    ASSERT_TRUE(FileUtil::FileExists(testFile));
    bool isLastStartUpShort;
    ASSERT_TRUE(PanicReport::InitPanicReport(isLastStartUpShort));
    ASSERT_FALSE(FileUtil::FileExists(testFile));
}

/**
 * @tc.name: BBoxDetectorUnitTest006
 * @tc.desc: check whether the process of compress is work.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST(BBoxDetectorBaseUnitTest, BBoxDetectorUnitTest006, TestSize.Level1)
{
    FileUtil::ForceCreateDirectory("/data/test/bbox/panic_log");
    FileUtil::ForceCreateDirectory("/data/test/bbox/ap_log");
    constexpr const char* testHmKLog = "/data/test/bbox/ap_log/hm_klog.txt";
    GenerateFile(testHmKLog, 0);
    constexpr const char* testLastFastBootLog = "/data/test/bbox/ap_log/last_fastboot_log";
    GenerateFile(testLastFastBootLog, 0);

    constexpr const char* testHmSnap = "/data/test/bbox/ap_log/hm_snapshot.txt";
    GenerateFile(testHmSnap, 0);
    constexpr const char* testHappenTimeStr1 = "20170805171207-00000001";
    PanicReport::CompressAndCopyLogFiles("/data/test/bbox/", testHappenTimeStr1);
    auto bboxSaveLogFlags = PanicReport::LoadBboxSaveFlagFromFile();
    ASSERT_EQ(bboxSaveLogFlags.happenTime, testHappenTimeStr1);
    ASSERT_TRUE(FileUtil::FileExists(PanicReport::GetBackupFilePath(testHappenTimeStr1)));

    GenerateFile(testHmSnap, 40 * 1024);
    constexpr const char* testHappenTimeStr2 = "20170805171207-00000002";
    PanicReport::CompressAndCopyLogFiles("/data/test/bbox/", testHappenTimeStr2);
    ASSERT_EQ(FileUtil::GetFolderSize("/data/test/bbox/panic_log/"), 0);

    GenerateFile(testHmSnap, 6 * 1024);
    constexpr const char* testHappenTimeStr3 = "20170805171207-00000003";
    PanicReport::CompressAndCopyLogFiles("/data/test/bbox/", testHappenTimeStr3);
    bboxSaveLogFlags = PanicReport::LoadBboxSaveFlagFromFile();
    ASSERT_EQ(bboxSaveLogFlags.happenTime, testHappenTimeStr3);
    ASSERT_TRUE(FileUtil::FileExists(PanicReport::GetBackupFilePath(testHappenTimeStr3)));
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

/**
 * @tc.name: SaveBboxLogFlagsToFile001
 * @tc.desc: target dir is not exist, test SaveBboxLogFlagsToFile.
 * @tc.type: FUNC
 */
HWTEST(SaveBboxLogFlagsToFileTest, SaveBboxLogFlagsToFile001, TestSize.Level1)
{
    FileUtil::ForceRemoveDirectory("/data/test/bbox/");

    PanicReport::BboxSaveLogFlags bboxSaveLogFlags;
    ASSERT_FALSE(PanicReport::SaveBboxLogFlagsToFile(bboxSaveLogFlags));
}

/**
 * @tc.name: ClearFilesInDir001
 * @tc.desc: flage file is open, test SaveBboxLogFlagsToFile.
 * @tc.type: FUNC
 */
HWTEST(ClearFilesInDirTest, ClearFilesInDir001, TestSize.Level1)
{
    /**
     * test case: input is invalid
     * */
    ASSERT_FALSE(PanicReport::ClearFilesInDir(""));
    ASSERT_FALSE(PanicReport::ClearFilesInDir("/data/test/invalid_path"));

    /**
     * test case: input is file path
     * */
    ASSERT_TRUE(FileUtil::ForceCreateDirectory("/data/test/bbox/"));
    ASSERT_TRUE(FileUtil::SaveStringToFile("/data/test/bbox/test_file", "test file"));
    ASSERT_FALSE(PanicReport::ClearFilesInDir("/data/test/bbox/test_file"));
}

/**
 * @tc.name: GetParamValueFromFile001
 * @tc.desc: file path is invalid.
 * @tc.type: FUNC
 */
HWTEST(GetParamValueFromFileTest, GetParamValueFromFile001, TestSize.Level1)
{
    /**
     * test case: input is invalid
     * */
    ASSERT_EQ(PanicReport::GetParamValueFromFile("", "last_bootup_keypoint"), "");
    ASSERT_EQ(PanicReport::GetParamValueFromFile("/data/test/invalid_file_path", "last_bootup_keypoint"), "");
}

/**
 * @tc.name: TryToReportRecoveryPanicEvent001
 * @tc.desc: file path is invalid.
 * @tc.type: FUNC
 */
HWTEST(TryToReportRecoveryPanicEventTest, TryToReportRecoveryPanicEvent001, TestSize.Level1)
{
    ASSERT_TRUE(FileUtil::ForceCreateDirectory("/data/test/bbox/"));

    PanicReport::BboxSaveLogFlags bboxSaveLogFlags;
    bboxSaveLogFlags.isPanicUploaded = true;
    bboxSaveLogFlags.happenTime = "0";
    bboxSaveLogFlags.factoryRecoveryTime = PanicReport::GetLastRecoveryTime();
    bboxSaveLogFlags.softwareVersion = "UT_TEST";
    ASSERT_TRUE(PanicReport::SaveBboxLogFlagsToFile(bboxSaveLogFlags));

    ASSERT_FALSE(PanicReport::TryToReportRecoveryPanicEvent());

    bboxSaveLogFlags.isPanicUploaded = false;
    ASSERT_TRUE(PanicReport::SaveBboxLogFlagsToFile(bboxSaveLogFlags));
    ASSERT_FALSE(PanicReport::TryToReportRecoveryPanicEvent());
}

/**
 * @tc.name: BBoxDetectorModuleTest008
 * @tc.desc: check event is recovery panic
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST(BBoxDetectorModuleTest, BBoxDetectorModuleTest008, TestSize.Level1)
{
    /**
     * @tc.steps: step1. construct PANIC" SysEvent
     * @tc.steps: step2. construct BBOXDetectorPlugin
     * @tc.steps: step3. OnEvent
     * @tc.steps: step4. check result should return true
     */
    SysEventCreator sysEventCreator("KERNEL_VENDOR", "PANIC", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("SUMMARY", "bootup_keypoint:97");
    sysEventCreator.SetKeyValue("HAPPEN_TIME", "443990995");
    sysEventCreator.SetKeyValue("LOG_PATH", "/data/test/bbox/panic_log/");
    sysEventCreator.SetKeyValue("SUB_LOG_PATH", "19700106031950-00001111");
    sysEventCreator.SetKeyValue("MODULE", "AP");
    sysEventCreator.SetKeyValue("REASON", "HM_PANIC:HM_PANIC_SYSMGR");

    auto sysEvent = make_shared<SysEvent>("test", nullptr, sysEventCreator);
    shared_ptr<Event> event = dynamic_pointer_cast<Event>(sysEvent);
    BBoxDetectorsBase bboxBase;
    auto eventRecorder = std::make_shared<BboxEventRecorder>();
    bool isLastStartUpShort = false;
    bboxBase.HandleBBoxEvent(sysEvent, eventRecorder, isLastStartUpShort);

    ASSERT_TRUE(sysEvent->GetEventValue("FIRST_FRAME").empty());
    ASSERT_TRUE(sysEvent->GetEventValue("SECOND_FRAME").empty());
    ASSERT_TRUE(sysEvent->GetEventValue("LAST_FRAME").empty());
}

/**
 * @tc.name: StartBootScan001
 * @tc.desc: coverage StartBootScan.
 * @tc.type: FUNC
 */
HWTEST(StartBootScanTest, StartBootScan001, TestSize.Level1)
{
    FileUtil::ForceCreateDirectory("/data/hisi_logs/");
    FileUtil::ForceCreateDirectory("/data/log/bbox/");
    std::string bboxContent = "[system exception],module[AP],category[NORMALBOOT],event[COLDBOOT]," \
                               "time[19700106031950-00001111],sysreboot[true],errdesc[boot_up_keypoint:46]," \
                               "logpath[/data/test/hisi_logs/]\n" \
                               "[system exception],module[AP],category[],event[COLDBOOT]," \
                               "time[19700106031950-00001111],sysreboot[true],errdesc[boot_up_keypoint:46]," \
                               "logpath[/data/test/hisi_logs/]\n" \
                               "[system exception],module[AP],category[TEST:123],event[COLDBOOT]," \
                               "time[19700106031950-00001111],sysreboot[true],errdesc[boot_up_keypoint:46]," \
                               "logpath[/data/test/hisi_logs/]";
    bboxContent += "\n[system exception],module[AP],category[TEST:123],event[COLDBOOT],time[" + LocalTimeFormat() +
        "-00001111],sysreboot[true],errdesc[boot_up_keypoint:46],logpath[/data/test/hisi_logs/]\n";
    EXPECT_CALL(MockHisyseventUtil::GetInstance(), IsEventProcessed).WillRepeatedly(testing::Return(false));
    FileUtil::SaveStringToFile("/data/log/bbox/history.log", bboxContent, true);
    std::shared_ptr<BboxEventRecorder> eventRecorder = std::make_shared<BboxEventRecorder>();
    BBoxDetectorsBase bboxBase;
    bboxBase.StartBootScan(eventRecorder);
    ASSERT_TRUE(eventRecorder);
}

/**
 * @tc.name: StartBootScan002
 * @tc.desc: coverage StartBootScan.
 * @tc.type: FUNC
 */
HWTEST(StartBootScanTest, StartBootScan002, TestSize.Level1)
{
    FileUtil::ForceCreateDirectory("/data/hisi_logs/");
    FileUtil::ForceCreateDirectory("/data/log/bbox/");
    std::string hisiContent = "system exception core [CP], reason [CP_S_RILD_EXCEPTION], " \
                               "time [19700106031950-00001111], sysreboot [false], " \
                               "bootup_keypoint [250], category [MODEMCRASH]";
    FileUtil::SaveStringToFile("/data/hisi_logs/history.log", hisiContent, true);
    std::string bboxContent = "[system exception],module[AP],category[NORMALBOOT],event[COLDBOOT]," \
                               "time[19700106031950-00001111],sysreboot[true],errdesc[boot_up_keypoint:46]," \
                               "logpath[/data/test/hisi_logs/]\n" \
                               "[system exception],module[AP],category[],event[COLDBOOT]," \
                               "time[19700106031950-00001111],sysreboot[true],errdesc[boot_up_keypoint:46]," \
                               "logpath[/data/test/hisi_logs/]\n" \
                               "[system exception],module[AP],category[TEST:123],event[COLDBOOT]," \
                               "time[19700106031950-00001111],sysreboot[true],errdesc[boot_up_keypoint:46]," \
                               "logpath[/data/test/hisi_logs/]\n";
    bboxContent += "[system exception],module[AP],category[TEST:123],event[COLDBOOT],time[" + LocalTimeFormat() +
        "-00001111],sysreboot[true],errdesc[boot_up_keypoint:46],logpath[/data/test/hisi_logs/]\n";
    EXPECT_CALL(MockHisyseventUtil::GetInstance(), IsEventProcessed).WillRepeatedly(testing::Return(false));
    std::shared_ptr<BboxEventRecorder> eventRecorder = std::make_shared<BboxEventRecorder>();
    BBoxDetectorsBase bboxBase;
    bboxBase.StartBootScan(eventRecorder);
    ASSERT_TRUE(eventRecorder);
}
}  // namespace HiviewDFX
}  // namespace OHOS
