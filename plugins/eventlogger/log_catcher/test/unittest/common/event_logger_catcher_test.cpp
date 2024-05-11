 /*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "event_logger.h"
#include "event_log_catcher.h"
#include "sys_event.h"
#include "hisysevent.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;

namespace OHOS {
namespace HiviewDFX {
constexpr mode_t DEFAULT_MODE = 0644;
void EventloggerCatcherTest::SetUp()
{
    /**
     * @tc.setup: create an event loop and multiple event handlers
     */
    printf("SetUp.\n");
    printf("path_ is %s\n", path_.c_str());

    sleep(1);
    isSelinuxEnabled_ = false;
    char buffer[BUF_SIZE_64] = {'\0'};
    FILE* fp = popen("getenforce", "r");
    if (fp != nullptr) {
        fgets(buffer, sizeof(buffer), fp);
        std::string str = buffer;
        printf("buffer is %s\n", str.c_str());
        if (str.find("Enforcing") != str.npos) {
            printf("Enforcing %s\n", str.c_str());
            isSelinuxEnabled_ = true;
        } else {
            printf("This isn't Enforcing %s\n", str.c_str());
        }
        pclose(fp);
    } else {
        printf("fp == nullptr\n");
    }
    system("setenforce 0");

    constexpr mode_t defaultLogDirMode = 0770;
    if (!FileUtil::FileExists(path_)) {
        FileUtil::ForceCreateDirectory(path_);
        FileUtil::ChangeModeDirectory(path_, defaultLogDirMode);
    }
}

void EventloggerCatcherTest::TearDown()
{
    /**
     * @tc.teardown: destroy the event loop we have created
     */
    if (isSelinuxEnabled_) {
        system("setenforce 1");
        isSelinuxEnabled_ = false;
    }

    printf("TearDown.\n");
}

int EventloggerCatcherTest::GetFdSize(int32_t fd)
{
    struct stat fileStat;
    if (fstat(fd, &fileStat) == -1) {
        return 0;
    }
    return fileStat.st_size;
}

std::string EventloggerCatcherTest::GetFormatTime(unsigned long timestamp)
{
    struct tm tm;
    time_t ts;
    /* 20: the length of 'YYYYmmddHHMMSS' */
    int strLen = 20;
    ts = timestamp;
    localtime_r(&ts, &tm);
    char buf[strLen];

    (void)memset_s(buf, sizeof(buf), 0, strLen);
    strftime(buf, strLen - 1, "%Y%m%d%H%M%S", &tm);
    return std::string(buf, strlen(buf));
}

int EventloggerCatcherTest::JudgmentsFileSize(int minQuantity, const std::string sender)
{
    constexpr mode_t defaultLogFileMode = 0644;
    std::string logPath = path_ + "/" + logFile_;
    auto fd2 = open(logPath.c_str(), O_RDONLY, defaultLogFileMode);
    if (fd2 < 0) {
        printf("second, Fail to create %s. fd2 == %d\n", logFile_.c_str(), fd2);
        return RETURN_OPEN_FAIL2;
    }
    auto size = GetFdSize(fd2);
    printf("%s, file size %d\n", sender.c_str(), size);
    if (size < minQuantity) {
        printf("error %s, less than size %d\n", sender.c_str(), minQuantity);
        close(fd2);
        return RETURN_LESS_THAN_SIZE;
    }
    return fd2;
}

int EventloggerCatcherTest::StartCreate(const std::string action, int pid,
    const std::string packageName, int interval, int minQuantity)
{
    auto eventlogger = std::make_unique<EventLogger>();
    std::string name = "TEST" + packageName;
    std::string jsonStr = R"~({"domain_":"demo","name_":")~" + name + R"~(","pid_":)~" +
        std::to_string(pid) + R"~(,"tid_":6527,"PACKAGE_NAME":")~" + packageName + R"~("})~";
    auto event = std::make_shared<SysEvent>("sender", nullptr, jsonStr);
    std::time_t timeTmp = 0;
    time(&timeTmp);
    event->happenTime_ = timeTmp;
    event->SetValue("eventLog_action", action);
    event->SetValue("eventLog_interval", interval);

    printf("pid is %d, pid_ %d; packageName is %s, PACKAGE_NAME %s\n",
        pid, event->GetPid(), packageName.c_str(), event->GetEventValue("PACKAGE_NAME").c_str());

    std::string idStr = event->eventName_;
    logFile_ = idStr + "-" + GetFormatTime(event->happenTime_) + ".log";

    constexpr mode_t defaultLogFileMode = 0644;
    std::string logPath = path_ + "/" + logFile_;
    printf("logPath is %s\n", logPath.c_str());
    auto fd = open(logPath.c_str(), O_CREAT | O_WRONLY | O_TRUNC, defaultLogFileMode);
    if (fd < 0) {
        printf("Fail to create %s. fd == %d\n", logFile_.c_str(), fd);
        return RETURN_OPEN_FAIL;
    }

    auto logTask = std::make_unique<EventLogTask>(fd, 1, event);
    logTask->AddLog(event->GetValue("eventLog_action"));
    auto ret = logTask->StartCompose();
    if (ret != EventLogTask::TASK_SUCCESS) {
        printf("capture fail %d", ret);
        close(fd);
        return RETURN_TASK_NO_SUCCESS;
    }
    close(fd);

    printf("logTask LogSize is %ld\n", logTask->GetLogSize());
    if (logTask->GetLogSize() < minQuantity) {
        printf("error LogSize less than %d\n", minQuantity);
        return RETURN_TASK_LESS_THAN_SIZE;
    }
    return 0;
}

/**
 * @tc.name: EventloggerCatcherTest001
 * @tc.desc: parse a correct config file and check result
 * @tc.type: FUNC
 * @tc.require: AR000FT62O
 */
HWTEST_F(EventloggerCatcherTest, EventloggerCatcherTest001, TestSize.Level3)
{
    /**
     * @tc.steps: step1. create event handler and events
     */
    constexpr int minQuantity = 8000;
    int pid = CommonUtils::GetPidByName("foundation");
    auto ret = StartCreate("s", pid, "foundation", 0, minQuantity);
    if (ret < 0) {
        printf("EventloggerCatcherTest001 StartCreate is error ret == %d\n", ret);
        FAIL();
    }

    auto fd = JudgmentsFileSize(minQuantity, "EventloggerCatcherTest001");
    if (fd < 0) {
        printf("EventloggerCatcherTest001 JudgmentsFileSize is error ret == %d\n", fd);
        FAIL();
    }

    char readTmp[BUF_SIZE_256];
    ret = -1;
    while (read(fd, readTmp, BUF_SIZE_256)) {
        std::string tmp = readTmp;
        if (tmp.find("system/bin") != tmp.npos) {
            ret = 0;
            break;
        }
    }

    if (ret < 0) {
        printf("not find Key Messages\n");
        close(fd);
        FAIL();
    }
    close(fd);
}

/**
 * @tc.name: EventloggerCatcherTest002
 * @tc.desc: parse a correct config file and check result
 * @tc.type: FUNC
 * @tc.require: AR000FT62O
 */
HWTEST_F(EventloggerCatcherTest, EventloggerCatcherTest002, TestSize.Level3)
{
    /**
     * @tc.steps: step1. create event handler and events
     */
    printf("EventloggerCatcherTest002 start\n");

    int pid = -1;
    const int memSize = 1024 * 3;
    if ((pid = fork()) < 0) {
        printf("Fork error, err:%d", errno);
        FAIL();
    }

    // Creating a process with high usage
    if (pid == 0) {
        prctl(PR_SET_NAME, "EventlogTest02");
        prctl(PR_SET_PDEATHSIG, SIGKILL);
        printf("Fork pid == 0, child process\n");
        volatile int temp[memSize] = {0};
        while (true) {
            int i = 0;
            while (i < memSize) {
                temp[i] = i & 0x234567;
                i++;
            }
        }
    }

    sleep(6);
    constexpr int minQuantity = 500;
    auto ret = StartCreate("cmd:c", pid, "EventlogTest02", 0, minQuantity);

    if (ret < 0) {
        printf("EventloggerCatcherTest002 StartCreate is error ret == %d\n", ret);
        FAIL();
    }

    auto fd = JudgmentsFileSize(minQuantity, "EventloggerCatcherTest002");
    if (fd < 0) {
        printf("EventloggerCatcherTest002 JudgmentsFileSize is error ret == %d\n", fd);
        FAIL();
    }

    char readTmp[BUF_SIZE_256];
    ret = -1;
    while (read(fd, readTmp, BUF_SIZE_256)) {
        std::string tmp = readTmp;
        if (tmp.find("[cpuusage]") != tmp.npos) {
            ret = 0;
            break;
        }
    }

    if (ret < 0) {
        printf("not find Key Messages\n");
        close(fd);
        FAIL();
    }
    close(fd);
}

/**
 * @tc.name: EventloggerCatcherTest003
 * @tc.desc: parse a correct config file and check result
 * @tc.type: FUNC
 * @tc.require: AR000FT62O
 */
HWTEST_F(EventloggerCatcherTest, EventloggerCatcherTest003, TestSize.Level3)
{
    /**
     * @tc.steps: step1. create event handler and events
     */

    int pid = -1;
    const int memSize = 1024 * 3;
    if ((pid = fork()) < 0) {
        printf("Fork error, err:%d", errno);
        FAIL();
    }

    // Creating a process with high memory
    if (pid == 0) {
        prctl(PR_SET_NAME, "EventlogTest03");
        prctl(PR_SET_PDEATHSIG, SIGKILL);
        printf("Fork pid == 0, child process\n");
        volatile int temp[memSize] = {0};
        while (true) {
            int i = 0;
            while (i < memSize) {
                temp[i] = i & 0x234567;
                i++;
            }
        }
    }

    sleep(6);
    constexpr int minQuantity = 500;
    auto ret = StartCreate("cmd:m", pid, "EventlogTest03", 0, minQuantity);
    if (ret < 0) {
        printf("EventloggerCatcherTest003 is error ret == %d\n", ret);
        FAIL();
    }

    auto fd = JudgmentsFileSize(minQuantity, "EventloggerCatcherTest003");
    if (fd < 0) {
        printf("EventloggerCatcherTest003 JudgmentsFileSize is error ret == %d\n", fd);
        FAIL();
    }

    char readTmp[BUF_SIZE_256];
    ret = -1;
    while (read(fd, readTmp, BUF_SIZE_256)) {
        std::string tmp = readTmp;
        if (tmp.find("[memory]") != tmp.npos) {
            ret = 0;
            break;
        }
    }

    if (ret < 0) {
        printf("not find Key Messages\n");
        close(fd);
        FAIL();
    }
    close(fd);
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
static HWTEST_F(EventloggerCatcherTest, EventlogTask_002, TestSize.Level3)
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
    close(fd);
}

/**
 * @tc.name: EventlogTask
 * @tc.desc: test EventlogTask
 * @tc.type: FUNC
 */
static HWTEST_F(EventloggerCatcherTest, EventlogTask_003, TestSize.Level3)
{
    auto fd = open("/data/test/testFile", O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_MODE);
    if (fd < 0) {
        printf("Fail to create testFile. errno: %d\n", errno);
        FAIL();
    }
    SysEventCreator sysEventCreator("HIVIEWDFX", "EventlogTask", SysEventCreator::FAULT);
    std::shared_ptr<SysEvent> sysEvent = std::make_shared<SysEvent>("EventlogTask", nullptr, sysEventCreator);
    std::unique_ptr<EventLogTask> logTask = std::make_unique<EventLogTask>(fd, 1, sysEvent);
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
    printf("task size: %d\n", static_cast<int>(logTask->tasks_.size()));
    EXPECT_EQ(logTask->PeerBinderCapture("Test"), false);
    EXPECT_EQ(logTask->PeerBinderCapture("pb"), false);
    EXPECT_EQ(logTask->PeerBinderCapture("pb:1:a"), true);
    close(fd);
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
    peerBinderCatcher->ForkToDumpHiperf(pids);
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
    std::set<int> pids = peerBinderCatcher->GetBinderPeerPids(fd, 1);
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
    peerBinderCatcher->BinderInfoParser(fin, fd1, 1);
    std::set<int> pids = peerBinderCatcher->GetBinderPeerPids(fd, 1);
    EXPECT_TRUE(pids.empty());
    pids = peerBinderCatcher->GetBinderPeerPids(-1, 1);
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

    shellCatcher->Initialize("hidumper --mem", ShellCatcher::CATCHER_MEM, pid);
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
} // namesapce HiviewDFX
} // namespace OHOS
