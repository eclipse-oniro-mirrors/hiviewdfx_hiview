 /*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
#include "event_logger_catcher_test.h"

#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>

#include <fcntl.h>
#include <sys/prctl.h>
#include <unistd.h>

#include "securec.h"
#include "common_utils.h"
#include "file_util.h"
#define private public
#include "dmesg_catcher.h"
#include "event_log_task.h"
#include "open_stacktrace_catcher.h"
#include "shell_catcher.h"
#include "peer_binder_catcher.h"
#undef private
#include "binder_catcher.h"
#include "memory_catcher.h"
#include "event_logger.h"
#include "event_log_catcher.h"
#include "sys_event.h"
#include "hisysevent.h"
#include "eventlogger_util_test.h"
#include "log_catcher_utils.h"
using namespace testing::ext;
using namespace OHOS::HiviewDFX;

namespace OHOS {
namespace HiviewDFX {
void EventloggerCatcherTest::SetUp()
{
    /**
     * @tc.setup: create an event loop and multiple event handlers
     */
    printf("SetUp.\n");
    InitSeLinuxEnabled();
}

void EventloggerCatcherTest::TearDown()
{
    /**
     * @tc.teardown: destroy the event loop we have created
     */
    CancelSeLinuxEnabled();
    printf("TearDown.\n");
}

/**
 * @tc.name: EventLogCatcher
 * @tc.desc: test EventLogCatcher
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, EventLogCatcherTest_001, TestSize.Level3)
{
    auto eventLogCatcher = std::make_shared<EventLogCatcher>();
    EXPECT_TRUE(eventLogCatcher->GetLogSize() == -1);
    eventLogCatcher->SetLogSize(1);
    EXPECT_TRUE(eventLogCatcher->GetLogSize() == 1);
    EXPECT_TRUE(eventLogCatcher->AppendFile(-1, "") == 0);
    auto fd = open("/data/test/catcherFile", O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd < 0) {
        printf("Fail to create catcherFile. errno: %d\n", errno);
        FAIL();
    }
    int res = eventLogCatcher->Catch(fd, 1);
    EXPECT_TRUE(res == 0);
    std::string fileName = "/data/test/catcherFile";
    eventLogCatcher->AppendFile(fd, fileName);
    EXPECT_TRUE(eventLogCatcher->AppendFile(fd, "") == 0);
    close(fd);
}

/**
 * @tc.name: EventlogTask
 * @tc.desc: test EventLogTask
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, EventlogTask_001, TestSize.Level3)
{
    auto fd = open("/data/test/testFile", O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd < 0) {
        printf("Fail to create testFile. errno: %d\n", errno);
        FAIL();
    }
    SysEventCreator sysEventCreator("HIVIEWDFX", "EventlogTask", SysEventCreator::FAULT);
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>("EventlogTask", nullptr, sysEventCreator);
    std::unique_ptr<EventLogTask> logTask = std::make_unique<EventLogTask>(fd, 1, sysEvent);
    logTask->AddStopReason(fd, nullptr, "Test");
    auto eventLogCatcher = std::make_shared<EventLogCatcher>();
    logTask->AddStopReason(fd, eventLogCatcher, "Test");
    logTask->AddSeparator(fd, eventLogCatcher);
    bool ret = logTask->ShouldStopLogTask(fd, 1, -1, eventLogCatcher);
    EXPECT_EQ(ret, false);
    ret = logTask->ShouldStopLogTask(fd, 1, 20000, eventLogCatcher);
    EXPECT_EQ(ret, false);
    logTask->status_ = EventLogTask::Status::TASK_TIMEOUT;
    ret = logTask->ShouldStopLogTask(fd, 0, 1, eventLogCatcher);
    EXPECT_EQ(ret, true);
    close(fd);
}

/**
 * @tc.name: EventlogTask
 * @tc.desc: test EventlogTask
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, EventlogTask_002, TestSize.Level3)
{
    auto fd = open("/data/test/testFile", O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd < 0) {
        printf("Fail to create testFile. errno: %d\n", errno);
        FAIL();
    }
    SysEventCreator sysEventCreator("HIVIEWDFX", "EventlogTask", SysEventCreator::FAULT);
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>("EventlogTask", nullptr, sysEventCreator);
    std::unique_ptr<EventLogTask> logTask = std::make_unique<EventLogTask>(fd, 1, sysEvent);
    EXPECT_EQ(logTask->GetTaskStatus(), EventLogTask::Status::TASK_RUNNABLE);
    auto ret = logTask->StartCompose();
    EXPECT_EQ(ret, 2);
    EXPECT_EQ(logTask->GetTaskStatus(), EventLogTask::Status::TASK_RUNNING);
    ret = logTask->StartCompose();
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(logTask->GetLogSize(), 0);
    logTask->GetThermalInfo(fd);
    close(fd);
}

/**
 * @tc.name: EventlogTask
 * @tc.desc: test EventlogTask
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, EventlogTask_003, TestSize.Level3)
{
    auto fd = open("/data/test/testFile", O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd < 0) {
        printf("Fail to create testFile. errno: %d\n", errno);
        FAIL();
    }
    SysEventCreator sysEventCreator("HIVIEWDFX", "EventlogTask", SysEventCreator::FAULT);
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>("EventlogTask", nullptr, sysEventCreator);
    std::unique_ptr<EventLogTask> logTask = std::make_unique<EventLogTask>(fd, 1, sysEvent);
    logTask->AddLog("cmd:scbCS");
    logTask->AppStackCapture();
    logTask->SystemStackCapture();
    logTask->BinderLogCapture();
    logTask->WMSUsageCapture();
    logTask->AMSUsageCapture();
    logTask->PMSUsageCapture();
    logTask->DPMSUsageCapture();
    logTask->HilogCapture();
    logTask->RSUsageCapture();
    logTask->Screenshot();
    logTask->DmesgCapture();
    logTask->SCBSessionCapture();
    logTask->SCBViewParamCapture();
    logTask->LightHilogCapture();
    logTask->SCBWMSCapture();
    logTask->DumpAppMapCapture();
    logTask->SCBWMSEVTCapture();
    logTask->SysrqCapture(true);
    logTask->MemoryUsageCapture();
    logTask->DMSUsageCapture();
    logTask->CpuUsageCapture();
    logTask->MMIUsageCapture();
    logTask->HitraceCapture();
    logTask->SCBWMSEVTCapture();
    logTask->InputHilogCapture();
    logTask->RemoteStackCapture();
    logTask->AddLog("Test");
    logTask->AddLog("cmd:w");
    logTask->status_ = EventLogTask::Status::TASK_RUNNING;
    auto ret = logTask->StartCompose();
    printf("task size: %d\n", static_cast<int>(logTask->tasks_.size()));
    EXPECT_EQ(logTask->PeerBinderCapture("Test"), false);
    EXPECT_EQ(logTask->PeerBinderCapture("pb"), false);
    EXPECT_EQ(logTask->PeerBinderCapture("pb:1:a"), true);
    close(fd);
}

/**
 * @tc.name: EventlogTask
 * @tc.desc: test EventlogTask
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, EventlogTask_004, TestSize.Level3)
{
    auto fd = open("/data/test/vreFile", O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd < 0) {
        printf("Fail to create vreFile. errno: %d\n", errno);
        FAIL();
    }
    SysEventCreator sysEventCreator("HIVIEWDFX", "EventlogTask", SysEventCreator::FAULT);
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>("EventlogTask", nullptr, sysEventCreator);
    sysEvent->SetEventValue("PID", getpid());
    sysEvent->SetEventValue("APPNODEID", 2025);
    sysEvent->SetEventValue("APPNODENAME", "test appNodeName");
    sysEvent->SetEventValue("LEASHWINDOWID", 319);
    sysEvent->SetEventValue("LEASHWINDOWNAME", "test leashWindowName");
    sysEvent->SetEventValue("EXT_INFO", "test ext_info");

    sysEvent->domain_ = "AAFWK";
    sysEvent->eventName_ = "APP_INPUT_BLOCK";
    std::unique_ptr<EventLogTask> logTask = std::make_unique<EventLogTask>(fd, 1, sysEvent);
    logTask->SaveRsVulKanError();
    sysEvent->domain_ = "GRAPHIC";
    sysEvent->eventName_ = "RS_VULKAN_ERROR";
    logTask->SaveRsVulKanError();
    close(fd);

    std::string line;
    std::ifstream ifs("/data/test/vreFile", std::ios::in);
    if (ifs.is_open()) {
        while (std::getline(ifs, line)) {
            if (line.find("APPNODEID") != std::string::npos) {
                printf("%s", line.c_str());
                EXPECT_EQ(line, "APPNODEID=2025");
            }
            if (line.find("EXT_INFO") != std::string::npos) {
                printf("%s", line.c_str());
                EXPECT_EQ(line, "EXT_INFO=test ext_info");
            }
        }
    }
    EXPECT_EQ(sysEvent->GetEventValue("PROCESS_NAME"), "EventloggerCatcherTest");
}

/**
 * @tc.name: BinderCatcherTest_001
 * @tc.desc: add testcase code coverage
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, BinderCatcherTest_001, TestSize.Level1)
{
    auto binderCatcher = std::make_shared<BinderCatcher>();
    bool ret = binderCatcher->Initialize("test", 1, 2);
    EXPECT_EQ(ret, true);
    auto fd = open("/data/test/catcherFile", O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd < 0) {
        printf("Fail to create catcherFile. errno: %d\n", errno);
        FAIL();
    }
    int res = binderCatcher->Catch(fd, 1);
    EXPECT_TRUE(res > 0);
    close(fd);
}

/**
 * @tc.name: MemoryCatcherTest_001
 * @tc.desc: add testcase code coverage
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, MemoryCatcherTest_001, TestSize.Level1)
{
    auto memoryCatcher = std::make_shared<MemoryCatcher>();
    bool ret = memoryCatcher->Initialize("test", 1, 2);
    EXPECT_EQ(ret, true);
    auto fd = open("/data/test/catcherFile", O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd < 0) {
        printf("Fail to create catcherFile. errno: %d\n", errno);
        FAIL();
    }
    int res = memoryCatcher->Catch(fd, 1);
    EXPECT_TRUE(res > 0);
    res = memoryCatcher->Catch(0, 1);
    EXPECT_EQ(res, 0);
    printf("memoryCatcher result: %d\n", res);
    close(fd);
}

/**
 * @tc.name: DmesgCatcherTest_001
 * @tc.desc: add testcase code coverage
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, DmesgCatcherTest_001, TestSize.Level1)
{
    auto dmesgCatcher = std::make_shared<DmesgCatcher>();
    auto jsonStr = "{\"domain_\":\"KERNEL_VENDOR\"}";
    std::shared_ptr<SysEvent> event = std::make_shared<SysEvent>("DmesgCatcherTest_001",
        nullptr, jsonStr);
    event->eventId_ = 0;
    event->domain_ = "KERNEL_VENDOR";
    event->eventName_ = "HUNGTASK";
    event->SetEventValue("PID", 0);
    EXPECT_TRUE(dmesgCatcher->Init(event));
}

/**
 * @tc.name: DmesgCatcherTest_002
 * @tc.desc: add testcase code coverage
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, DmesgCatcherTest_002, TestSize.Level1)
{
    auto dmesgCatcher = std::make_shared<DmesgCatcher>();
    auto fd = open("/data/test/dmesgCatcherFile", O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd < 0) {
        printf("Fail to create dmesgCatcherFile. errno: %d\n", errno);
        FAIL();
    }
    dmesgCatcher->Initialize("", 0, 0);
    int jsonFd = 1;
    EXPECT_TRUE(dmesgCatcher->Catch(fd, jsonFd) > 0);

    dmesgCatcher->Initialize("", 0, 1);
    EXPECT_TRUE(dmesgCatcher->Catch(fd, jsonFd) > 0);

    dmesgCatcher->Initialize("", 1, 0);
    printf("dmesgCatcher result: %d\n", dmesgCatcher->Catch(fd, jsonFd));

    dmesgCatcher->Initialize("", 1, 1);
    printf("dmesgCatcher result: %d\n", dmesgCatcher->Catch(fd, jsonFd));

    close(fd);
}

/**
 * @tc.name: DmesgCatcherTest_003
 * @tc.desc: add testcase code coverage
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, DmesgCatcherTest_003, TestSize.Level1)
{
    auto dmesgCatcher = std::make_shared<DmesgCatcher>();
    bool ret = dmesgCatcher->DumpDmesgLog(-1);
    EXPECT_EQ(ret, false);
    auto fd = open("/data/test/dmesgCatcherFile", O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd < 0) {
        printf("Fail to create dmesgCatcherFile. errno: %d\n", errno);
        FAIL();
    }
    ret = dmesgCatcher->DumpDmesgLog(fd);
    EXPECT_EQ(ret, true);
    ret = dmesgCatcher->WriteSysrq();
    EXPECT_EQ(ret, true);
    std::string res = dmesgCatcher->DmesgSaveTofile();
    printf("DmesgSaveTofile size: %zu\n", res.size());
    close(fd);
}

/**
 * @tc.name: OpenStacktraceCatcherTest_001
 * @tc.desc: add testcase code coverage
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, OpenStacktraceCatcherTest_001, TestSize.Level1)
{
    auto fd = open("/data/test/catcherFile", O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd < 0) {
        printf("Fail to create catcherFile. errno: %d\n", errno);
        FAIL();
    }

    auto openStackCatcher = std::make_shared<OpenStacktraceCatcher>();
    ASSERT_EQ(openStackCatcher->Initialize("", 0, 0), false);

    int jsonFd = 1;
    bool ret = openStackCatcher->Catch(fd, jsonFd);
    EXPECT_TRUE(ret == 0);

    EXPECT_EQ(openStackCatcher->Initialize("test", 0, 0), false);
    ret = openStackCatcher->Catch(fd, jsonFd);
    EXPECT_TRUE(ret == 0);

    EXPECT_EQ(openStackCatcher->Initialize("", 1, 0), true);
    EXPECT_EQ(openStackCatcher->Initialize("test", 1, 0), true);
    ret = openStackCatcher->Catch(fd, jsonFd);
    EXPECT_TRUE(ret > 0);
    close(fd);
}

/**
 * @tc.name: OpenStacktraceCatcherTest_002
 * @tc.desc: add testcase code coverage
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, OpenStacktraceCatcherTest_002, TestSize.Level1)
{
    auto fd = open("/data/test/catcherFile", O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd < 0) {
        printf("Fail to create catcherFile. errno: %d\n", errno);
        FAIL();
    }
    
    auto openStackCatcher = std::make_shared<OpenStacktraceCatcher>();
    bool ret = openStackCatcher->Catch(fd, 1);
    EXPECT_TRUE(ret == 0);
    EXPECT_EQ(openStackCatcher->ForkAndDumpStackTrace(fd), 0);
    close(fd);
}

/**
 * @tc.name: PeerBinderCatcherTest_001
 * @tc.desc: add testcase code coverage
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, PeerBinderCatcherTest_001, TestSize.Level1)
{
    auto fd = open("/data/test/catcherFile", O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd < 0) {
        printf("Fail to create catcherFile. errno: %d\n", errno);
        FAIL();
    }
    auto peerBinderCatcher = std::make_shared<PeerBinderCatcher>();
    peerBinderCatcher->Initialize("PeerBinderCatcherTest", 0, 0);
    int jsonFd = 1;
    int res = peerBinderCatcher->Catch(fd, jsonFd);
    EXPECT_TRUE(res < 0);

    peerBinderCatcher->Initialize("a", 0, 0);
    peerBinderCatcher->Initialize("a", 1, 0);
    res = peerBinderCatcher->Catch(fd, jsonFd);
    EXPECT_TRUE(res < 0);

    peerBinderCatcher->Initialize("a", 1, 1);
    res = peerBinderCatcher->Catch(fd, jsonFd);
    EXPECT_TRUE(res > 0);

    int pid = CommonUtils::GetPidByName("foundation");
#ifdef HAS_HIPERF
    std::set<int> pids;
    pids.insert(pid);
    peerBinderCatcher->DoExecHiperf("peerBinderCatcher", pids);
#endif
    peerBinderCatcher->Initialize("", 0, pid);
    peerBinderCatcher->Initialize("foundation", 0, pid);
    peerBinderCatcher->Initialize("foundation", 1, pid);
    peerBinderCatcher->CatcherStacktrace(fd, pid);
    close(fd);
}

/**
 * @tc.name: PeerBinderCatcherTest_002
 * @tc.desc: add testcase code coverage
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, PeerBinderCatcherTest_002, TestSize.Level1)
{
    auto fd = open("/data/test/catcherFile", O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd < 0) {
        printf("Fail to create catcherFile. errno: %d\n", errno);
        FAIL();
    }
    auto peerBinderCatcher = std::make_shared<PeerBinderCatcher>();
    peerBinderCatcher->Initialize("a", 1, 0);
    auto jsonStr = "{\"domain_\":\"KERNEL_VENDOR\"}";
    std::shared_ptr<SysEvent> event = std::make_shared<SysEvent>("PeerBinderCatcherTest_002",
        nullptr, jsonStr);
    event->eventId_ = 0;
    event->domain_ = "KERNEL_VENDOR";
    event->eventName_ = "HUNGTASK";
    event->SetEventValue("PID", 0);
    std::string filePath = "/data/test/catcherFile";
    std::set<int> catchedPids;
    catchedPids.insert(0);
    catchedPids.insert(1);
    int pid = CommonUtils::GetPidByName("foundation");
    catchedPids.insert(pid);
    peerBinderCatcher->Init(event, filePath, catchedPids);
    peerBinderCatcher->Initialize("foundation", 1, pid);
    int res = peerBinderCatcher->Catch(fd, 1);
    EXPECT_GT(res, 0);
    close(fd);
}

/**
 * @tc.name: PeerBinderCatcherTest_003
 * @tc.desc: add testcase code coverage
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, PeerBinderCatcherTest_003, TestSize.Level1)
{
    std::set<int> pids;
    pids.insert(0);
    pids.insert(2);
    auto peerBinderCatcher = std::make_shared<PeerBinderCatcher>();
    PeerBinderCatcher::BinderInfo info = {
        .client = 1,
        .server = 1,
        .wait = 1
    };
    std::map<int, std::list<OHOS::HiviewDFX::PeerBinderCatcher::BinderInfo>> manager;
    manager[info.client].push_back(info);
    peerBinderCatcher->ParseBinderCallChain(manager, pids, 0);
    PeerBinderCatcher::BinderInfo info1 = {
        .client = 2,
        .server = 2,
        .wait = 0
    };
    manager[info1.client].push_back(info1);
    peerBinderCatcher->ParseBinderCallChain(manager, pids, 1);
#ifdef HAS_HIPERF
    pids.insert(3);
    pids.insert(4);
    peerBinderCatcher->DoExecHiperf("peerBinderCatcher", pids);
    peerBinderCatcher->DumpHiperf(pids);
#endif
}

/**
 * @tc.name: PeerBinderCatcherTest_004
 * @tc.desc: add testcase code coverage
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, PeerBinderCatcherTest_004, TestSize.Level1)
{
    auto fd = open("/data/test/catcherFile", O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd < 0) {
        printf("Fail to create catcherFile. errno: %d\n", errno);
        FAIL();
    }
    auto peerBinderCatcher = std::make_shared<PeerBinderCatcher>();
    std::set<int> asyncPids;
    std::set<int> pids = peerBinderCatcher->GetBinderPeerPids(fd, 1, asyncPids);
    EXPECT_TRUE(pids.empty());
}

/**
 * @tc.name: PeerBinderCatcherTest_005
 * @tc.desc: add testcase code coverage
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, PeerBinderCatcherTest_005, TestSize.Level1)
{
    auto peerBinderCatcher = std::make_shared<PeerBinderCatcher>();
    std::list<PeerBinderCatcher::OutputBinderInfo> infoList;
    peerBinderCatcher->AddBinderJsonInfo(infoList, -1);
    PeerBinderCatcher::OutputBinderInfo info = {
        .info = "Test",
        .pid = 0
    };
    infoList.push_back(info);
    peerBinderCatcher->AddBinderJsonInfo(infoList, 1);
    PeerBinderCatcher::OutputBinderInfo info1 = {
        .info = "Test",
        .pid = getpid()
    };
    infoList.push_back(info1);
    peerBinderCatcher->AddBinderJsonInfo(infoList, 1);
    std::string str = "/proc/" + std::to_string(getpid()) + "/cmdline";
    printf("%s\n", str.c_str());
    EXPECT_TRUE(!str.empty());
}

/**
 * @tc.name: PeerBinderCatcherTest_006
 * @tc.desc: add testcase code coverage
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, PeerBinderCatcherTest_006, TestSize.Level1)
{
    auto fd = open("/data/test/peerFile", O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd < 0) {
        printf("Fail to create peerFile. errno: %d\n", errno);
        FAIL();
    }
    std::ofstream testFile;
    std::string path = "/data/test/peerFile";
    testFile.open(path);
    testFile << "0:pid\tcontext:binder\t0:request\t3:started\t"
        "16:max\t4:ready\t521092:free_space\n";
    testFile.close();

    auto peerBinderCatcher = std::make_shared<PeerBinderCatcher>();
    std::ifstream fin;
    fin.open(path.c_str());
    if (!fin.is_open()) {
        printf("open binder file failed, %s\n.", path.c_str());
        FAIL();
    }
    auto fd1 = open("/data/test/peerTestFile", O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd1 < 0) {
        printf("Fail to create peerTestFile. errno: %d\n", errno);
        FAIL();
    }
    std::set<int> asyncPids;
    peerBinderCatcher->BinderInfoParser(fin, fd1, 1, asyncPids);
    std::set<int> pids = peerBinderCatcher->GetBinderPeerPids(fd, 1, asyncPids);
    EXPECT_TRUE(pids.empty());
    pids = peerBinderCatcher->GetBinderPeerPids(-1, 1, asyncPids);
    EXPECT_TRUE(pids.empty());
    fin.close();
}

/**
 * @tc.name: ShellCatcherTest
 * @tc.desc: add testcase code coverage
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, ShellCatcherTest_001, TestSize.Level1)
{
    auto fd = open("/data/test/shellCatcherFile", O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd < 0) {
        printf("Fail to create shellCatcherFile. errno: %d\n", errno);
        FAIL();
    }

    auto shellCatcher = std::make_shared<ShellCatcher>();
    int pid = CommonUtils::GetPidByName("foundation");

    bool res = shellCatcher->Initialize("", ShellCatcher::CATCHER_WMS, pid);
    EXPECT_TRUE(res);

    int jsonFd = 1;
    EXPECT_TRUE(shellCatcher->Catch(fd, jsonFd) < 0);

    shellCatcher->Initialize("hidumper -s WindowManagerService -a -a", ShellCatcher::CATCHER_WMS, pid);
    EXPECT_TRUE(shellCatcher->Catch(fd, jsonFd) > 0);

    shellCatcher->Initialize("hidumper -s AbilityManagerService -a -a", ShellCatcher::CATCHER_AMS, pid);
    EXPECT_TRUE(shellCatcher->Catch(fd, jsonFd) > 0);

    shellCatcher->Initialize("hidumper --cpuusage", ShellCatcher::CATCHER_CPU, pid);
    EXPECT_TRUE(shellCatcher->Catch(fd, jsonFd) > 0);

    shellCatcher->Initialize("hidumper -s PowerManagerService -a -s", ShellCatcher::CATCHER_PMS, pid);
    EXPECT_TRUE(shellCatcher->Catch(fd, jsonFd) > 0);

    shellCatcher->Initialize("hidumper -s DisplayPowerManagerService", ShellCatcher::CATCHER_DPMS, pid);
    EXPECT_TRUE(shellCatcher->Catch(fd, jsonFd) > 0);

    shellCatcher->Initialize("hidumper -s RenderService -a allInfo", ShellCatcher::CATCHER_RS, pid);
    EXPECT_TRUE(shellCatcher->Catch(fd, jsonFd) > 0);

    shellCatcher->Initialize("hilog -x", ShellCatcher::CATCHER_HILOG, 0);
    EXPECT_TRUE(shellCatcher->Catch(fd, jsonFd) > 0);

    shellCatcher->Initialize("hilog -z", ShellCatcher::CATCHER_LIGHT_HILOG, 0);
    EXPECT_TRUE(shellCatcher->Catch(fd, jsonFd) > 0);

    shellCatcher->Initialize("snapshot_display -f", ShellCatcher::CATCHER_SNAPSHOT, 0);
    EXPECT_TRUE(shellCatcher->Catch(fd, jsonFd) > 0);

    shellCatcher->Initialize("scb_debug SCBScenePanel getContainerSession", ShellCatcher::CATCHER_SCBSESSION, 0);
    printf("CATCHER_SCBSESSION result: %s\n", shellCatcher->Catch(fd, jsonFd) > 0 ? "true" : "false");

    shellCatcher->Initialize("scb_debug SCBScenePanel getViewParam", ShellCatcher::CATCHER_SCBVIEWPARAM, 0);
    printf("CATCHER_SCBVIEWPARAM result: %s\n", shellCatcher->Catch(fd, jsonFd) > 0 ? "true" : "false");

    shellCatcher->Initialize("hidumper -s WindowManagerService -a -w -default", ShellCatcher::CATCHER_SCBWMS, 0);
    EXPECT_EQ(shellCatcher->Catch(fd, jsonFd), 0);

    auto jsonStr = "{\"domain_\":\"KERNEL_VENDOR\"}";
    std::shared_ptr<SysEvent> event = std::make_shared<SysEvent>("ShellCatcherTest", nullptr, jsonStr);
    event->SetValue("FOCUS_WINDOW", 4); // 4 test value
    shellCatcher->SetEvent(event);
    shellCatcher->Initialize("hidumper -s WindowManagerService -a -w -default", ShellCatcher::CATCHER_SCBWMS, 0);
    printf("CATCHER_SCBWMS result: %s\n", shellCatcher->Catch(fd, jsonFd) > 0 ? "true" : "false");

    shellCatcher->Initialize("hilog -T InputKeyFlow -z 1000", ShellCatcher::CATCHER_INPUT_HILOG, 0);
    shellCatcher->Initialize("hilog -T InputKeyFlow -e eventId -z 1000",
        ShellCatcher::CATCHER_INPUT_EVENT_HILOG, 0);

    shellCatcher->Initialize("default", -1, 0);
    EXPECT_EQ(shellCatcher->Catch(fd, jsonFd), 0);

    close(fd);
}
 
/**
 * @tc.name: ShellCatcherTest
 * @tc.desc: GET_DISPLAY_SNAPSHOT test
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, ShellCatcherTest_002, TestSize.Level1)
{
    int windowId = 4;
    int ret = HiSysEventWrite(HiSysEvent::Domain::WINDOW_MANAGER,
        "GET_DISPLAY_SNAPSHOT",
        HiSysEvent::EventType::STATISTIC,
        "FOCUS_WINDOW", windowId);
    printf("HiSysEventWrite: %d\n", ret);
    EXPECT_EQ(ret, 0);
}
 
/**
 * @tc.name: ShellCatcherTest
 * @tc.desc: add test
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, ShellCatcherTest_003, TestSize.Level1)
{
    auto fd = open("/data/test/testFile", O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd < 0) {
        printf("Fail to create testFile. errno: %d\n", errno);
        FAIL();
    }
    auto shellCatcher = std::make_shared<ShellCatcher>();
    shellCatcher->Initialize("hidumper -s 1910 -a DumpAppMap", ShellCatcher::CATCHER_DAM, 0);
    EXPECT_TRUE(shellCatcher->Catch(fd, 1) >= 0);
    printf("DumpAppMap result: %s\n", shellCatcher->Catch(fd, 1) > 0 ? "true" : "false");
    close(fd);
}

/**
 * @tc.name: LogCatcherUtilsTest_001
 * @tc.desc: add test
 * @tc.type: FUNC
 */
HWTEST_F(EventloggerCatcherTest, LogCatcherUtilsTest_001, TestSize.Level1)
{
    int pid = getpid();
    int ret = LogCatcherUtils::DumpStacktrace(-1, pid);
    EXPECT_EQ(ret, -1);
    LogCatcherUtils::DumpStacktrace(1, pid);
    LogCatcherUtils::DumpStacktrace(2, pid);
    ret = LogCatcherUtils::WriteKernelStackToFd(2, "Test", -1);
    EXPECT_EQ(ret, -1);
    ret = LogCatcherUtils::WriteKernelStackToFd(200, "Test\n", getprocpid());
    EXPECT_EQ(ret, 0);
    ret = LogCatcherUtils::WriteKernelStackToFd(200, "Test\n", getprocpid());
    EXPECT_EQ(ret, 0);
}
} // namespace HiviewDFX
} // namespace OHOS
