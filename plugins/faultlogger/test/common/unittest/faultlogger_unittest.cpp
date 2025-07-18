/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#include <string>
#include <vector>

#include <fcntl.h>
#include <fstream>
#include <gtest/gtest.h>
#include <regex>
#include "sys_event.h"
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>
#include <cctype>
#include <cstring>

#include "bundle_mgr_client.h"
#include "event.h"
#include "faultlog_util.h"
#include "faultlog_database.h"
#include "faultlogger.h"
#include "faultevent_listener.h"
#include "faultlog_formatter.h"
#include "faultlog_info_ohos.h"
#include "faultlog_query_result_ohos.h"
#include "faultlogger_service_ohos.h"
#include "file_util.h"
#include "hisysevent_manager.h"
#include "hiview_global.h"
#include "hiview_logger.h"
#include "hiview_platform.h"
#include "ipc_skeleton.h"
#include "json/json.h"
#include "log_analyzer.h"
#include "sys_event.h"
#include "sys_event_dao.h"
#include "faultlog_bundle_util.h"
#include "faultlog_sanitizer.h"
#include "faultlog_freeze.h"
#include "faultlog_processor_base.h"
#include "faultlog_events_processor.h"
#include "faultlog_processor_factory.h"
#include "faultlog_bootscan.h"
#include "faultlog_manager_service.h"
#include "faultlog_hilog_helper.h"
#include "faultlog_event_factory.h"
#include "faultlog_cppcrash.h"
#include "faultlog_jserror.h"
#include "faultlog_cjerror.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::FaultlogHilogHelper;
namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "FaultloggerUT");
static std::shared_ptr<FaultEventListener> faultEventListener = nullptr;
static std::map<int, std::string> fileNames_ = {};

static HiviewContext& InitHiviewContext()
{
    OHOS::HiviewDFX::HiviewPlatform &platform = HiviewPlatform::GetInstance();
    bool result = platform.InitEnvironment("/data/test/test_faultlogger_data/hiview_platform_config");
    printf("InitHiviewContext result:%d\n", result);
    return platform;
}

static HiviewContext& GetHiviewContext()
{
    static HiviewContext& hiviewContext = InitHiviewContext();
    return hiviewContext;
}

static void StartHisyseventListen(std::string domain, std::string eventName)
{
    faultEventListener = std::make_shared<FaultEventListener>();
    ListenerRule tagRule(domain, eventName, RuleType::WHOLE_WORD);
    std::vector<ListenerRule> sysRules = {tagRule};
    HiSysEventManager::AddListener(faultEventListener, sysRules);
}

static std::shared_ptr<Faultlogger> InitFaultloggerInstance()
{
    auto plugin = std::make_shared<Faultlogger>();
    plugin->SetName("Faultlogger");
    plugin->SetHandle(nullptr);
    plugin->SetHiviewContext(&GetHiviewContext());
    plugin->OnLoad();
    return plugin;
}

static std::shared_ptr<Faultlogger> GetFaultloggerInstance()
{
    static std::shared_ptr<Faultlogger> faultloggerInstance = InitFaultloggerInstance();
    return faultloggerInstance;
}

namespace {
auto g_fdDeleter = [] (int32_t *ptr) {
    if (*ptr > 0) {
        close(*ptr);
    }
    delete ptr;
};
}

class FaultloggerUnittest : public testing::Test {
public:
    void SetUp()
    {
        sleep(1);
        GetHiviewContext();
    };
    void TearDown() {};

    static void CheckSumarryParseResult(std::string& info, int& matchCount)
    {
        Json::Reader reader;
        Json::Value appEvent;
        if (!(reader.parse(info, appEvent))) {
            matchCount--;
        }
        auto exception = appEvent["exception"];
        if (exception["name"] == "" || exception["name"] == "none") {
            matchCount--;
        }
        if (exception["message"] == "" || exception["message"] == "none") {
            matchCount--;
        }
        if (exception["stack"] == "" || exception["stack"] == "none") {
            matchCount--;
        }
    }

    static int CheckKeyWordsInFile(const std::string& filePath, std::string *keywords, int length, bool isJsError)
    {
        std::ifstream file;
        file.open(filePath.c_str(), std::ios::in);
        std::ostringstream infoStream;
        infoStream << file.rdbuf();
        std::string info = infoStream.str();
        if (info.length() == 0) {
            std::cout << "file is empty, file:" << filePath << std::endl;
            return 0;
        }
        int matchCount = 0;
        for (int index = 0; index < length; index++) {
            if (info.find(keywords[index]) != std::string::npos) {
                matchCount++;
            } else {
                std::cout << "can not find keyword:" << keywords[index] << std::endl;
            }
        }
        if (isJsError) {
            CheckSumarryParseResult(info, matchCount);
        }
        file.close();
        return matchCount;
    }

    static void ConstructJsErrorAppEvent(std::string summmay, std::shared_ptr<Faultlogger> plugin)
    {
        SysEventCreator sysEventCreator("AAFWK", "JSERROR", SysEventCreator::FAULT);
        sysEventCreator.SetKeyValue("SUMMARY", summmay);
        sysEventCreator.SetKeyValue("name_", "JS_ERROR");
        sysEventCreator.SetKeyValue("happenTime_", 1670248360359); // 1670248360359 : Simulate happenTime_ value
        sysEventCreator.SetKeyValue("REASON", "TypeError");
        sysEventCreator.SetKeyValue("tz_", "+0800");
        sysEventCreator.SetKeyValue("pid_", 2413); // 2413 : Simulate pid_ value
        sysEventCreator.SetKeyValue("tid_", 2413); // 2413 : Simulate tid_ value
        sysEventCreator.SetKeyValue("what_", 3); // 3 : Simulate what_ value
        sysEventCreator.SetKeyValue("PACKAGE_NAME", "com.ohos.systemui");
        sysEventCreator.SetKeyValue("VERSION", "1.0.0");
        sysEventCreator.SetKeyValue("TYPE", 3); // 3 : Simulate TYPE value
        sysEventCreator.SetKeyValue("VERSION", "1.0.0");

        auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
        std::shared_ptr<Event> event = std::dynamic_pointer_cast<Event>(sysEvent);
        bool result = plugin->OnEvent(event);
        ASSERT_EQ(result, true);
    }

    static void ConstructJsErrorAppEventWithNoValue(std::string summmay, std::shared_ptr<Faultlogger> plugin)
    {
        SysEventCreator sysEventCreator("AAFWK", "JSERROR", SysEventCreator::FAULT);
        sysEventCreator.SetKeyValue("SUMMARY", summmay);
        sysEventCreator.SetKeyValue("name_", "JS_ERROR");
        sysEventCreator.SetKeyValue("happenTime_", 1670248360359); // 1670248360359 : Simulate happenTime_ value
        sysEventCreator.SetKeyValue("TYPE", 3); // 3 : Simulate TYPE value
        sysEventCreator.SetKeyValue("VERSION", "1.0.0");

        auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
        std::shared_ptr<Event> event = std::dynamic_pointer_cast<Event>(sysEvent);
        bool result = plugin->OnEvent(event);
        ASSERT_EQ(result, true);
    }

    static void CheckKeyWordsInJsErrorAppEventFile(std::string name)
    {
        std::string keywords[] = {
            "\"bundle_name\":", "\"bundle_version\":", "\"crash_type\":", "\"exception\":",
            "\"foreground\":", "\"hilog\":", "\"pid\":", "\"time\":", "\"uid\":", "\"uuid\":",
            "\"name\":", "\"message\":", "\"stack\":"
        };
        int length = sizeof(keywords) / sizeof(keywords[0]);
        std::string oldFileName = "/data/test_jsError_info";
        int count = CheckKeyWordsInFile(oldFileName, keywords, length, true);
        ASSERT_EQ(count, length) << "ReportJsErrorToAppEventTest001-" + name + " check keywords failed";
        if (FileUtil::FileExists(oldFileName)) {
            std::string newFileName = oldFileName + "_" + name;
            rename(oldFileName.c_str(), newFileName.c_str());
        }
        auto ret = remove("/data/test_jsError_info");
        if (ret == 0) {
            GTEST_LOG_(INFO) << "remove /data/test_jsError_info failed";
        }
    }

    static void ConstructCjErrorAppEvent(std::string summmay, std::shared_ptr<Faultlogger> plugin)
    {
        SysEventCreator sysEventCreator("CJ_RUNTIME", "CJERROR", SysEventCreator::FAULT);
        sysEventCreator.SetKeyValue("SUMMARY", summmay);
        sysEventCreator.SetKeyValue("name_", "CJ_ERROR");
        sysEventCreator.SetKeyValue("happenTime_", 1670248360359); // 1670248360359 : Simulate happenTime_ value
        sysEventCreator.SetKeyValue("REASON", "std.core:Exception");
        sysEventCreator.SetKeyValue("tz_", "+0800");
        sysEventCreator.SetKeyValue("pid_", 2413); // 2413 : Simulate pid_ value
        sysEventCreator.SetKeyValue("tid_", 2413); // 2413 : Simulate tid_ value
        sysEventCreator.SetKeyValue("what_", 3); // 3 : Simulate what_ value
        sysEventCreator.SetKeyValue("PACKAGE_NAME", "com.ohos.systemui");
        sysEventCreator.SetKeyValue("VERSION", "1.0.0");
        sysEventCreator.SetKeyValue("TYPE", 3); // 3 : Simulate TYPE value
        sysEventCreator.SetKeyValue("VERSION", "1.0.0");

        auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
        std::shared_ptr<Event> event = std::dynamic_pointer_cast<Event>(sysEvent);
        bool result = plugin->OnEvent(event);
        ASSERT_EQ(result, true);
    }

    static void CheckKeyWordsInCjErrorAppEventFile(std::string name)
    {
        std::string keywords[] = {
            "\"bundle_name\":", "\"bundle_version\":", "\"crash_type\":", "\"exception\":",
            "\"foreground\":", "\"hilog\":", "\"pid\":", "\"time\":", "\"uid\":", "\"uuid\":",
            "\"name\":", "\"message\":", "\"stack\":"
        };
        int length = sizeof(keywords) / sizeof(keywords[0]);
        std::cout << "length:" << length << std::endl;
        std::string oldFileName = "/data/test_cjError_info";
        int count = CheckKeyWordsInFile(oldFileName, keywords, length, false);
        std::cout << "count:" << count << std::endl;
        ASSERT_EQ(count, length) << "ReportCjErrorToAppEventTest001-" + name + " check keywords failed";
        if (FileUtil::FileExists(oldFileName)) {
            std::string newFileName = oldFileName + "_" + name;
            rename(oldFileName.c_str(), newFileName.c_str());
        }
        auto ret = remove("/data/test_cjError_info");
        if (ret == 0) {
            GTEST_LOG_(INFO) << "remove /data/test_cjError_info failed";
        }
    }

    static void CheckDeleteStackErrorMessage(std::string name)
    {
        std::string keywords[] = {"\"Cannot get SourceMap info, dump raw stack:"};
        int length = sizeof(keywords) / sizeof(keywords[0]);
        std::string oldFileName = "/data/test_jsError_info";
        int count = CheckKeyWordsInFile(oldFileName, keywords, length, true);
        ASSERT_NE(count, length) << "check delete stack error message failed";
    }
};

static const std::string APPFREEZE_FAULT_FILE = "/data/test/test_data/SmartParser/test_faultlogger_data/";

/**
 * @tc.name: dumpFileListTest001
 * @tc.desc: dump with cmds, check the result
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, dumpFileListTest001, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. add multiple cmds to faultlogger
     * @tc.expected: check the content size of the dump function
     */
    auto plugin = GetFaultloggerInstance();
    FaultLogManagerService faultlogManagerService(plugin->GetWorkLoop(), plugin->faultLogManager_);
    int fd = TEMP_FAILURE_RETRY(open("/data/test/testFile", O_CREAT | O_WRONLY | O_TRUNC, 770));
    bool isSuccess = fd >= 0;
    if (!isSuccess) {
        ASSERT_FALSE(isSuccess);
        printf("Fail to create test result file.\n");
    } else {
        std::vector<std::string> cmds;
        faultlogManagerService.Dump(fd, cmds);
        cmds.push_back("Faultlogger");
        faultlogManagerService.Dump(fd, cmds);
        cmds.push_back("-l");
        faultlogManagerService.Dump(fd, cmds);
        cmds.push_back("-f");
        faultlogManagerService.Dump(fd, cmds);
        cmds.push_back("cppcrash-ModuleName-10-20201209103823");
        faultlogManagerService.Dump(fd, cmds);
        cmds.push_back("-d");
        faultlogManagerService.Dump(fd, cmds);
        cmds.push_back("-t");
        faultlogManagerService.Dump(fd, cmds);
        cmds.push_back("20201209103823");
        faultlogManagerService.Dump(fd, cmds);
        cmds.push_back("-m");
        faultlogManagerService.Dump(fd, cmds);
        cmds.push_back("FAULTLOGGER");
        close(fd);
        fd = -1;

        std::string result;
        if (FileUtil::LoadStringFromFile("/data/test/testFile", result)) {
            ASSERT_GT(result.length(), 0uL);
        } else {
            FAIL();
        }
    }
}

/**
 * @tc.name: DumpTest002
 * @tc.desc: dump with cmds, check the result
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, DumpTest002, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. add multiple cmds to faultlogger
     * @tc.expected: check the content size of the dump function
     */
    auto plugin = GetFaultloggerInstance();
    int fd = TEMP_FAILURE_RETRY(open("/data/test/testFile", O_CREAT | O_WRONLY | O_TRUNC, 770));
    bool isSuccess = fd >= 0;
    if (!isSuccess) {
        ASSERT_FALSE(isSuccess);
        printf("Fail to create test result file.\n");
    } else {
        std::vector<std::vector<std::string>> cmds = {
            {"-f", "1cppcrash-10-20201209103823"},
            {"-f", "1cppcrash-ModuleName-10-20201209103823"},
            {"-f", "cppcrash--10-20201209103823"},
            {"-f", "cppcrash-ModuleName-a10-20201209103823"}
        };

        for (auto& cmd : cmds) {
            plugin->Dump(fd, cmd);
        }

        close(fd);
        fd = -1;

        std::string result;
        if (FileUtil::LoadStringFromFile("/data/test/testFile", result)) {
            ASSERT_GT(result.length(), 0uL);
        } else {
            FAIL();
        }
    }
}

/**
 * @tc.name: DumpTest003
 * @tc.desc: dump with cmds, check the result
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, DumpTest003, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. add multiple cmds to faultlogger
     * @tc.expected: check the content size of the dump function
     */
    auto plugin = GetFaultloggerInstance();
    FaultLogManagerService faultlogManagerService(plugin->GetWorkLoop(), plugin->faultLogManager_);
    int fd = TEMP_FAILURE_RETRY(open("/data/test/testFile", O_CREAT | O_WRONLY | O_TRUNC, 770));
    bool isSuccess = fd >= 0;
    if (!isSuccess) {
        ASSERT_FALSE(isSuccess);
        printf("Fail to create test result file.\n");
    } else {
        std::vector<std::vector<std::string>> cmds = {
            {"-t", "cppcrash--10-20201209103823"},
            {"-m", ""},
            {"-l", ""},
            {"-xx"}
        };

        for (auto& cmd : cmds) {
            faultlogManagerService.Dump(fd, cmd);
        }

        close(fd);
        fd = -1;

        std::string result;
        if (FileUtil::LoadStringFromFile("/data/test/testFile", result)) {
            ASSERT_GT(result.length(), 0uL);
        } else {
            FAIL();
        }
    }
}

/**
 * @tc.name: DumpTest004
 * @tc.desc: dump with cmds, check the result
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, DumpTest004, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. add multiple cmds to faultlogger
     * @tc.expected: check the content size of the dump function
     */

    auto plugin = GetFaultloggerInstance();
    FaultLogInfo info;
    info.time = 1607161163; // 1607161163 : analog value of time
    info.id = 10001;
    info.pid = 7496; // 7496 : analog value of pid
    info.faultLogType = 2; // 2 : CPP_CRASH
    info.module = "com.example.myapplication";
    FaultLogManagerService faultlogManagerService(plugin->GetWorkLoop(), plugin->faultLogManager_);
    faultlogManagerService.AddFaultLog(info);
    std::string timeStr = GetFormatedTimeWithMillsec(info.time);
    std::string appName = GetApplicationNameById(info.id);
    if (appName.size() == 0) {
        appName = info.module;
    }
    std::string fileName = "cppcrash-" + appName + "-" + std::to_string(info.id) + "-" + timeStr + ".log";
    auto path = "/data/log/faultlog/faultlogger/" + fileName;
    ASSERT_EQ(FileUtil::FileExists(path), true);

    int fd = TEMP_FAILURE_RETRY(open("/data/test/testFile", O_CREAT | O_WRONLY | O_TRUNC, 770));
    bool isSuccess = fd >= 0;
    if (!isSuccess) {
        ASSERT_FALSE(isSuccess);
        printf("Fail to create test result file.\n");
    } else {
        std::vector<std::vector<std::string>> cmds = {
            {"-LogSuffixWithMs", ""},
            {"-f", fileName}
        };

        for (auto& cmd : cmds) {
            faultlogManagerService.Dump(fd, cmd);
        }

        close(fd);
        fd = -1;
        std::string keywords[] = { "Device info", "Build info", "Fingerprint", "Module name" };
        int length = sizeof(keywords) / sizeof(keywords[0]);
        ASSERT_EQ(CheckKeyWordsInFile("/data/test/testFile", keywords, length, false), length);
    }
}

static void GenCppCrashLogTestCommon(int32_t uid, bool ifFileExist)
{
    int pipeFd[2] = {-1, -1};
    ASSERT_EQ(pipe(pipeFd), 0) << "create pipe failed";
    auto plugin = GetFaultloggerInstance();
    FaultLogInfo info;
    info.time = 1607161163; // 1607161163 : analog value of time
    info.id = uid;
    info.pid = 7496; // 7496 : analog value of pid
    info.faultLogType = 2; // 2 : CPP_CRASH
    info.module = "com.example.myapplication";
    info.sectionMap["APPVERSION"] = "1.0";
    info.sectionMap["FAULT_MESSAGE"] = "Nullpointer";
    info.sectionMap["TRACEID"] = "0x1646145645646";
    info.sectionMap["KEY_THREAD_INFO"] = "Test Thread Info";
    info.sectionMap["REASON"] = "TestReason";
    info.sectionMap["STACKTRACE"] = "#01 xxxxxx\n#02 xxxxxx\n";
    info.pipeFd.reset(new int32_t(pipeFd[0]), g_fdDeleter);
    std::string jsonInfo = R"~({"crash_type":"NativeCrash", "exception":{"frames":
        [{"buildId":"", "file":"/system/lib/ld-musl-arm.so.1", "offset":28, "pc":"000ac0a4", "symbol":"test_abc"},
        {"buildId":"12345abcde", "file":"/system/lib/chipset-pub-sdk/libeventhandler.z.so", "offset":278,
        "pc":"0000bef3", "symbol":"OHOS::AppExecFwk::EpollIoWaiter::WaitFor(std::__h::unique_lock<std::__h::mutex>&,
        long long)"}], "message":"", "signal":{"code":0, "signo":6}, "thread_name":"e.myapplication", "tid":1605},
        "pid":1605, "threads":[{"frames":[{"buildId":"", "file":"/system/lib/ld-musl-arm.so.1", "offset":72, "pc":
        "000c80b4", "symbol":"ioctl"}, {"buildId":"2349d05884359058d3009e1fe27b15fa", "file":
        "/system/lib/platformsdk/libipc_core.z.so", "offset":26, "pc":"0002cad7",
        "symbol":"OHOS::BinderConnector::WriteBinder(unsigned long, void*)"}], "thread_name":"OS_IPC_0_1607",
        "tid":1607}, {"frames":[{"buildId":"", "file":"/system/lib/ld-musl-arm.so.1", "offset":0, "pc":"000fdf4c",
        "symbol":""}, {"buildId":"", "file":"/system/lib/ld-musl-arm.so.1", "offset":628, "pc":"000ff7f4",
        "symbol":"__pthread_cond_timedwait_time64"}], "thread_name":"OS_SignalHandle", "tid":1608}],
        "time":1701863741296, "uid":20010043, "uuid":""})~";
    TEMP_FAILURE_RETRY(write(pipeFd[1], jsonInfo.c_str(), jsonInfo.size()));
    close(pipeFd[1]);
    FaultLogManagerService faultlogManagerService(plugin->GetWorkLoop(), plugin->faultLogManager_);
    faultlogManagerService.AddFaultLog(info);
    std::string timeStr = GetFormatedTimeWithMillsec(info.time);
    std::string appName = GetApplicationNameById(info.id);
    if (appName.size() == 0) {
        appName = info.module;
    }
    std::string fileName = "/data/log/faultlog/faultlogger/cppcrash-" + appName + "-" +
        std::to_string(info.id) + "-" + timeStr + ".log";
    ASSERT_EQ(FileUtil::FileExists(fileName), true);
    ASSERT_GT(FileUtil::GetFileSize(fileName), 0ul);
    // check appevent json info
    ASSERT_EQ(FileUtil::FileExists("/data/test_cppcrash_info_7496"), ifFileExist);
}

/**
 * @tc.name: genCppCrashLogTest001
 * @tc.desc: create cpp crash event and send it to faultlogger
 *           check info which send to appevent
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, GenCppCrashLogTest001, testing::ext::TestSize.Level3)
{
    GenCppCrashLogTestCommon(10001, true); // 10001 : analog value of user uid
    string keywords[] = { "\"time\":", "\"pid\":", "\"exception\":", "\"threads\":", "\"thread_name\":", "\"tid\":" };
    int length = sizeof(keywords) / sizeof(keywords[0]);
    ASSERT_EQ(CheckKeyWordsInFile("/data/test_cppcrash_info_7496", keywords, length, false), length);
    auto ret = remove("/data/test_cppcrash_info_7496");
    if (ret != 0) {
        GTEST_LOG_(INFO) << "remove /data/test_jsError_info failed. errno " << errno;
    }
}

/**
 * @tc.name: genCppCrashLogTest002
 * @tc.desc: create cpp crash event and send it to faultlogger
 *           check info which send to appevent
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, GenCppCrashLogTest002, testing::ext::TestSize.Level3)
{
    GenCppCrashLogTestCommon(0, false); // 0 : analog value of system uid
}

/**
 * @tc.name: AddFaultLogTest001
 * @tc.desc: create cpp crash event and send it to faultlogger
 *           check info which send to appevent
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, AddFaultLogTest001, testing::ext::TestSize.Level0)
{
    auto plugin = GetFaultloggerInstance();
    FaultLogManagerService faultlogManagerService(plugin->GetWorkLoop(), plugin->faultLogManager_);

    FaultLogInfo info;
    plugin->hasInit_ = false;
    faultlogManagerService.AddFaultLog(info);

    plugin->hasInit_ = true;
    info.faultLogType = -1;
    faultlogManagerService.AddFaultLog(info);

    info.faultLogType = 8; // 8 : 8 is bigger than FaultLogType::ADDR_SANITIZER
    faultlogManagerService.AddFaultLog(info);

    info.faultLogType = FaultLogType::CPP_CRASH;
    info.id = 1;
    info.module = "com.example.myapplication";
    info.time = 1607161163;
    info.pid = 7496;
    faultlogManagerService.AddFaultLog(info);
    std::string timeStr = GetFormatedTimeWithMillsec(info.time);
    std::string fileName = "/data/log/faultlog/faultlogger/cppcrash-com.example.myapplication-0-" + timeStr + ".log";
    ASSERT_EQ(FileUtil::FileExists(fileName), true);
}

/**
 * @tc.name: AddPublicInfoTest001
 * @tc.desc: create cpp crash event and send it to faultlogger
 *           check info which send to appevent
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, AddPublicInfoTest001, testing::ext::TestSize.Level3)
{
    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 0;
    info.pid = 7496;
    info.faultLogType = 1;
    info.module = "com.example.myapplication";
    info.sectionMap["APPVERSION"] = "1.0";
    info.sectionMap["FAULT_MESSAGE"] = "Nullpointer";
    info.sectionMap["TRACEID"] = "0x1646145645646";
    info.sectionMap["KEY_THREAD_INFO"] = "Test Thread Info";
    info.sectionMap["REASON"] = "TestReason";
    info.sectionMap["STACKTRACE"] = "#01 xxxxxx\n#02 xxxxxx\n";
    FaultLogEventsProcessor faultAddFault;
    faultAddFault.AddCommonInfo(info);
    std::string timeStr = GetFormatedTimeWithMillsec(info.time);
    std::string fileName = "/data/log/faultlog/faultlogger/cppcrash-com.example.myapplication-0-" + timeStr + ".log";
    ASSERT_EQ(FileUtil::FileExists(fileName), true);
}

/**
 * @tc.name: GetFreezeJsonCollectorTest001
 * @tc.desc: test GetFreezeJsonCollector
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, GetFreezeJsonCollectorTest001, testing::ext::TestSize.Level3)
{
    FaultLogInfo info;
    info.time = 20170805172159;
    info.id = 10006;
    info.pid = 1;
    info.faultLogType = 1;
    info.module = "com.example.myapplication";
    info.sectionMap["APPVERSION"] = "1.0";
    info.sectionMap["FAULT_MESSAGE"] = "Nullpointer";
    info.sectionMap["TRACEID"] = "0x1646145645646";
    info.sectionMap["KEY_THREAD_INFO"] = "Test Thread Info";
    info.sectionMap["REASON"] = "TestReason";
    info.sectionMap["STACKTRACE"] = "#01 xxxxxx\n#02 xxxxxx\n";
    FaultLogFreeze faultLogAppFreeze;
    FreezeJsonUtil::FreezeJsonCollector collector = faultLogAppFreeze.GetFreezeJsonCollector(info);
    ASSERT_EQ(collector.exception, "{}");
}

/**
 * @tc.name: genCppCrashtoAnalysisFaultlog
 * @tc.desc: create cpp crash event and check AnalysisFaultlog
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, genCppCrashtoAnalysisFaultlog001, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create a cpp crash event and pass it to faultlogger
     * @tc.expected: AnalysisFaultlog return expected result
     */
    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 0;
    info.pid = 7497;
    info.faultLogType = 2;
    info.module = "com.example.testapplication";
    info.reason = "TestReason";
    std::map<std::string, std::string> eventInfos;
    ASSERT_EQ(AnalysisFaultlog(info, eventInfos), false);
    ASSERT_EQ(!eventInfos["FINGERPRINT"].empty(), true);
}

/**
 * @tc.name: genJsCrashtoAnalysisFaultlog001
 * @tc.desc: create Js crash FaultLogInfo and check AnalysisFaultlog
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, genJsCrashtoAnalysisFaultlog001, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create Js crash FaultLogInfo
     * @tc.expected: AnalysisFaultlog return expected result
     */
    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 0;
    info.pid = 7497;
    info.faultLogType = 3;
    info.module = "com.example.testapplication";
    info.reason = "TestReason";
    std::map<std::string, std::string> eventInfos;
    ASSERT_EQ(AnalysisFaultlog(info, eventInfos), false);
    ASSERT_EQ(!eventInfos["FINGERPRINT"].empty(), true);
}

/**
 * @tc.name: genjserrorLogTest002
 * @tc.desc: create JS ERROR event and send it to faultlogger
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, genjserrorLogTest002, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create a jss_error event and pass it to faultlogger
     * @tc.expected: the calling is success and the file has been created
     */
    SysEventCreator sysEventCreator("AAFWK", "JSERROR", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("SUMMARY", "Error message:is not callable\nStacktrace:");
    sysEventCreator.SetKeyValue("name_", "JS_ERROR");
    sysEventCreator.SetKeyValue("happenTime_", 1670248360359);
    sysEventCreator.SetKeyValue("REASON", "TypeError");
    sysEventCreator.SetKeyValue("tz_", "+0800");
    sysEventCreator.SetKeyValue("pid_", 2413);
    sysEventCreator.SetKeyValue("tid_", 2413);
    sysEventCreator.SetKeyValue("what_", 3);
    sysEventCreator.SetKeyValue("PACKAGE_NAME", "com.ohos.systemui");
    sysEventCreator.SetKeyValue("VERSION", "1.0.0");
    sysEventCreator.SetKeyValue("TYPE", 3);
    sysEventCreator.SetKeyValue("VERSION", "1.0.0");

    auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    auto testPlugin = GetFaultloggerInstance();
    std::shared_ptr<Event> event = std::dynamic_pointer_cast<Event>(sysEvent);
    bool result = testPlugin->OnEvent(event);
    ASSERT_EQ(result, true);
    auto ret = remove("/data/test_jsError_info");
    if (ret < 0) {
        GTEST_LOG_(INFO) << "remove /data/test_jsError_info failed";
    }
}

/**
 * @tc.name: IsInterestedPipelineEvent
 * @tc.desc: Test calling IsInterestedPipelineEvent Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, IsInterestedPipelineEvent, testing::ext::TestSize.Level3)
{
    auto testPlugin = GetFaultloggerInstance();
    std::shared_ptr<Event> event = std::make_shared<Event>("test");
    event->SetEventName("PROCESS_EXIT");
    EXPECT_TRUE(testPlugin->IsInterestedPipelineEvent(event));
    event->SetEventName("JS_ERROR");
    EXPECT_TRUE(testPlugin->IsInterestedPipelineEvent(event));
    event->SetEventName("RUST_PANIC");
    EXPECT_TRUE(testPlugin->IsInterestedPipelineEvent(event));
    event->SetEventName("ADDR_SANITIZER");
    EXPECT_TRUE(testPlugin->IsInterestedPipelineEvent(event));
    event->SetEventName("OTHERS");
    EXPECT_FALSE(testPlugin->IsInterestedPipelineEvent(event));
};

/**
 * @tc.name: CanProcessEvent
 * @tc.desc: Test calling CanProcessEvent Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, CanProcessEvent, testing::ext::TestSize.Level3)
{
    auto testPlugin = GetFaultloggerInstance();
    std::shared_ptr<Event> event = std::make_shared<Event>("test");
    ASSERT_TRUE(testPlugin->CanProcessEvent(event));
};

/**
 * @tc.name: ReadyToLoad
 * @tc.desc: Test calling ReadyToLoad Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, ReadyToLoad, testing::ext::TestSize.Level3)
{
    auto testPlugin = GetFaultloggerInstance();
    ASSERT_TRUE(testPlugin->ReadyToLoad());
};

/**
 * @tc.name: SaveFaultLogInfoTest001
 * @tc.desc: Test calling SaveFaultLogInfo Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, SaveFaultLogInfoTest001, testing::ext::TestSize.Level3)
{
    StartHisyseventListen("RELIABILITY", "CPP_CRASH");
    time_t now = std::time(nullptr);
    std::vector<std::string> keyWords = { std::to_string(now) };
    faultEventListener->SetKeyWords(keyWords);
    FaultLogDatabase *faultLogDb = new FaultLogDatabase(GetHiviewContext().GetSharedWorkLoop());
    FaultLogInfo info;
    info.time = now;
    info.pid = getpid();
    info.id = 0;
    info.faultLogType = 2;
    info.logPath = "/data/log/faultlog/faultlogger/cppcrash-SaveFaultLogInfoTest001-20020100-20250501090923033.log";
    info.module = "FaultloggerUnittest";
    info.reason = "unittest for SaveFaultLogInfo";
    info.summary = "summary for SaveFaultLogInfo";
    info.sectionMap["APPVERSION"] = "1.0";
    info.sectionMap["FAULT_MESSAGE"] = "abort";
    info.sectionMap["TRACEID"] = "0x1646145645646";
    info.sectionMap["KEY_THREAD_INFO"] = "Test Thread Info";
    info.sectionMap["REASON"] = "TestReason";
    info.sectionMap["STACKTRACE"] = "#01 xxxxxx\n#02 xxxxxx\n";
    faultLogDb->SaveFaultLogInfo(info);
    ASSERT_TRUE(faultEventListener->CheckKeyWords());
}

/**
 * @tc.name: GetFaultInfoListTest001
 * @tc.desc: Test calling GetFaultInfoList Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, GetFaultInfoListTest001, testing::ext::TestSize.Level3)
{
    std::string jsonStr = R"~({"domain_":"RELIABILITY", "name_":"CPP_CRASH", "type_":1, "time_":1501973701070, "tz_":
    "+0800", "pid_":1854, "tid_":1854, "uid_":0, "FAULT_TYPE":"2", "PID":1854, "UID":0, "MODULE":"FaultloggerUnittest",
    "REASON":"unittest for SaveFaultLogInfo", "SUMMARY":"summary for SaveFaultLogInfo", "LOG_PATH":"", "VERSION":"",
    "HAPPEN_TIME":"1501973701", "PNAME":"/", "FIRST_FRAME":"/", "SECOND_FRAME":"/", "LAST_FRAME":"/", "FINGERPRINT":
    "04c0d6f03c73da531f00eb112479a8a2f19f59fafba6a474dcbe455a13288f4d", "level_":"CRITICAL", "tag_":"STABILITY", "id_":
    "17165544771317691984", "info_":""})~";
    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, jsonStr);
    sysEvent->SetLevel("MINOR");
    sysEvent->SetEventSeq(447); // 447: test seq
    EventStore::SysEventDao::Insert(sysEvent);
    FaultLogDatabase *faultLogDb = new FaultLogDatabase(GetHiviewContext().GetSharedWorkLoop());
    std::list<FaultLogInfo> infoList = faultLogDb->GetFaultInfoList("FaultloggerUnittest", 0, 2, 10);
    ASSERT_GT(infoList.size(), 0);

    FaultLogInfo info;
    bool ret = faultLogDb->ParseFaultLogInfoFromJson(nullptr, info);
    ASSERT_EQ(ret, false);
}

/**
 * @tc.name: FaultLogManager::CreateTempFaultLogFile
 * @tc.desc: Test calling CreateTempFaultLogFile Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultlogManager001, testing::ext::TestSize.Level3)
{
    std::unique_ptr<FaultLogManager> faultLogManager = std::make_unique<FaultLogManager>(nullptr);
    faultLogManager->Init();
    int fd = faultLogManager->CreateTempFaultLogFile(1607161345, 0, 2, "FaultloggerUnittest");
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
HWTEST_F(FaultloggerUnittest, FaultlogManager002, testing::ext::TestSize.Level3)
{
    std::unique_ptr<FaultLogManager> faultLogManager = std::make_unique<FaultLogManager>(nullptr);
    std::list<std::string> infoVec = {"1", "2", "3", "4", "5"};
    faultLogManager->ReduceLogFileListSize(infoVec, 1);
    ASSERT_EQ(infoVec.size(), 1);
}

/**
 * @tc.name: FaultLogManager::GetFaultLogFileList
 * @tc.desc: Test calling GetFaultLogFileList Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, GetFaultLogFileList001, testing::ext::TestSize.Level3)
{
    std::unique_ptr<FaultLogManager> faultLogManager = std::make_unique<FaultLogManager>(nullptr);
    faultLogManager->Init();
    std::list<std::string> fileList = faultLogManager->GetFaultLogFileList("FaultloggerUnittest", 1607161344, 0, 2, 1);
    ASSERT_EQ(fileList.size(), 1);
}

/**
 * @tc.name: FaultLogManager::WriteFaultLogToFile
 * @tc.desc: Test calling WriteFaultLogToFile Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, WriteFaultLogToFile001, testing::ext::TestSize.Level3)
{
    std::unique_ptr<FaultLogManager> faultLogManager = std::make_unique<FaultLogManager>(nullptr);
    faultLogManager->Init();
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
HWTEST_F(FaultloggerUnittest, GetFaultLogContent001, testing::ext::TestSize.Level3)
{
    std::unique_ptr<FaultLogManager> faultLogManager = std::make_unique<FaultLogManager>(nullptr);
    faultLogManager->Init();
    FaultLogInfo info {
        .time = 1607161345,
        .id = 0,
        .faultLogType = 2,
        .module = "FaultloggerUnittest"
    };
    std::string fileName = GetFaultLogName(info);
    std::string content;
    ASSERT_TRUE(faultLogManager->GetFaultLogContent(fileName, content));
    ASSERT_EQ(content, "testContent");
}

/**
 * @tc.name: FaultLogManager::GetFaultLogName
 * @tc.desc: Test calling GetFaultLogName Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, GetFaultLogContent002, testing::ext::TestSize.Level3)
{
    std::unique_ptr<FaultLogManager> faultLogManager = std::make_unique<FaultLogManager>(nullptr);
    faultLogManager->Init();
    FaultLogInfo info {
        .time = 1607161345,
        .id = 0,
        .faultLogType = FaultLogType::ADDR_SANITIZER,
        .module = "FaultloggerUnittest"
    };
    info.sanitizerType = "ASAN";
    std::string fileName = GetFaultLogName(info);
    ASSERT_EQ(fileName, "asan-FaultloggerUnittest-0-20201205174225345.log");
    info.sanitizerType = "HWASAN";
    fileName = GetFaultLogName(info);
    ASSERT_EQ(fileName, "hwasan-FaultloggerUnittest-0-20201205174225345.log");
    string type = "sanitizer";
    ASSERT_EQ(GetLogTypeByName(type), FaultLogType::ADDR_SANITIZER);
    type = "cjerror";
    ASSERT_EQ(GetLogTypeByName(type), FaultLogType::CJ_ERROR);
}

/**
 * @tc.name: FaultLogManager::GetDebugSignalTempLogName
 * @tc.desc: Test calling GetDebugSignalTempLogName Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, GetDebugSignalTempLogName001, testing::ext::TestSize.Level3)
{
    std::unique_ptr<FaultLogManager> faultLogManager = std::make_unique<FaultLogManager>(nullptr);
    faultLogManager->Init();
    FaultLogInfo info {
        .time = 1607161345,
        .id = 0,
        .faultLogType = FaultLogType::ADDR_SANITIZER,
        .module = "FaultloggerUnittest"
    };
    string fileName = GetDebugSignalTempLogName(info);
    ASSERT_EQ(fileName, "/data/log/faultlog/temp/stacktrace-0-1607161345");
    fileName = GetSanitizerTempLogName(info.pid, std::to_string(info.time));
    ASSERT_EQ(fileName, "/data/log/faultlog/temp/sanitizer-0-1607161345");
    string str;
    ASSERT_EQ(GetThreadStack(str, 0), "");
}

/**
 * @tc.name: FaultLogManager::SaveFaultInfoToRawDb
 * @tc.desc: Test calling SaveFaultInfoToRawDb Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogManagerTest001, testing::ext::TestSize.Level0)
{
    StartHisyseventListen("RELIABILITY", "CPP_CRASH");
    time_t now = std::time(nullptr);
    std::vector<std::string> keyWords = { std::to_string(now) };
    faultEventListener->SetKeyWords(keyWords);
    FaultLogInfo info;
    info.time = now;
    info.pid = getpid();
    info.id = 0;
    info.faultLogType = 2;
    info.logPath = "/data/log/faultlog/faultlogger/cppcrash-FaultLogManagerTest001-20020100-20250501090923033.log";
    info.module = "FaultloggerUnittest1111";
    info.reason = "unittest for SaveFaultLogInfo";
    info.summary = "summary for SaveFaultLogInfo";
    info.sectionMap["APPVERSION"] = "1.0";
    info.sectionMap["FAULT_MESSAGE"] = "abort";
    info.sectionMap["TRACEID"] = "0x1646145645646";
    info.sectionMap["KEY_THREAD_INFO"] = "Test Thread Info";
    info.sectionMap["REASON"] = "TestReason";
    info.sectionMap["STACKTRACE"] = "#01 xxxxxx\n#02 xxxxxx\n";
    std::unique_ptr<FaultLogManager> faultLogManager =
        std::make_unique<FaultLogManager>(GetHiviewContext().GetSharedWorkLoop());
    faultLogManager->Init();
    faultLogManager->SaveFaultInfoToRawDb(info);
    ASSERT_TRUE(faultEventListener->CheckKeyWords());
}

std::string GetTargetFileName(int32_t faultLogType, int64_t time)
{
    fileNames_.clear();
    fileNames_ = {
        {1, "Unknown"},
        {2, "cppcrash"}, // 2 : faultLogType to cppcrash
        {3, "jscrash"}, // 3 : faultLogType to jscrash
        {4, "appfreeze"}, // 4 : faultLogType to appfreeze
        {5, "sysfreeze"}, // 5 : faultLogType to sysfreeze
        {6, "syswarning"}, // 6 : faultLogType to syswarning
        {7, "rustpanic"}, // 7 : faultLogType to rustpanic
        {8, "sanitizer"}, // 8 : faultLogType to sanitizer
    };
    std::string fileName = fileNames_[faultLogType];
    return fileName + "-FaultloggerUnittest1111-0-" + GetFormatedTimeWithMillsec(time) + ".log";
}

/**
 * @tc.name: FaultLogManager::SaveFaultLogToFile
 * @tc.desc: Test calling SaveFaultLogToFile Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogManagerTest003, testing::ext::TestSize.Level3)
{
    FaultLogInfo info;
    std::unique_ptr<FaultLogManager> faultLogManager = std::make_unique<FaultLogManager>(nullptr);
    faultLogManager->Init();
    for (int i = 1; i <= fileNames_.size(); i++) {
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

        std::string fileName = faultLogManager->SaveFaultLogToFile(info);
        if (fileName.find("FaultloggerUnittest1111") == std::string::npos) {
            FAIL();
        }
        std::string targetFileName = GetTargetFileName(i, info.time);
        ASSERT_EQ(fileName, targetFileName);
    }
}

/**
 * @tc.name: faultLogManager GetFaultInfoListTest001
 * @tc.desc: Test calling faultLogManager.GetFaultInfoList Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogManagerTest002, testing::ext::TestSize.Level3)
{
    std::string jsonStr = R"~({"domain_":"RELIABILITY", "name_":"CPP_CRASH", "type_":1, "time_":1501973701070,
        "tz_":"+0800", "pid_":1854, "tid_":1854, "uid_":0, "FAULT_TYPE":"2", "PID":1854, "UID":0,
        "MODULE":"FaultloggerUnittest", "REASON":"unittest for SaveFaultLogInfo",
        "SUMMARY":"summary for SaveFaultLogInfo", "LOG_PATH":"", "VERSION":"", "HAPPEN_TIME":"1501973701",
        "PNAME":"/", "FIRST_FRAME":"/", "SECOND_FRAME":"/", "LAST_FRAME":"/",
        "FINGERPRINT":"04c0d6f03c73da531f00eb112479a8a2f19f59fafba6a474dcbe455a13288f4d",
        "level_":"CRITICAL", "tag_":"STABILITY", "id_":"17165544771317691984", "info_":""})~";
    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, jsonStr);
    sysEvent->SetLevel("MINOR");
    sysEvent->SetEventSeq(448); // 448: test seq
    EventStore::SysEventDao::Insert(sysEvent);

    std::unique_ptr<FaultLogManager> faultLogManager = std::make_unique<FaultLogManager>(nullptr);
    auto isProcessedFault1 = faultLogManager->IsProcessedFault(1854, 0, 2);
    ASSERT_EQ(isProcessedFault1, false);

    faultLogManager->Init();

    auto list = faultLogManager->GetFaultInfoList("FaultloggerUnittest", 0, 2, 10);
    ASSERT_GT(list.size(), 0);

    auto isProcessedFault2 = faultLogManager->IsProcessedFault(1854, 0, 2);
    ASSERT_EQ(isProcessedFault2, true);

    auto isProcessedFault3 = faultLogManager->IsProcessedFault(1855, 0, 2);
    ASSERT_EQ(isProcessedFault3, false);

    auto isProcessedFault4 = faultLogManager->IsProcessedFault(1855, 5, 2);
    ASSERT_EQ(isProcessedFault4, false);
}

/**
 * @tc.name: FaultLogManager::GetFaultLogFilePathTest001
 * @tc.desc: Test calling GetFaultLogFilePath Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, GetFaultLogFilePathTest001, testing::ext::TestSize.Level3)
{
    std::unique_ptr<FaultLogManager> faultLogManager = std::make_unique<FaultLogManager>(nullptr);
    faultLogManager->Init();
    std::string fileName = "com.freeze.test001-202506023.log";

    std::string faultLogFilePath = faultLogManager->GetFaultLogFilePath(FaultLogType::APP_FREEZE, fileName);
    ASSERT_EQ(faultLogFilePath, "/data/log/faultlog/faultlogger/com.freeze.test001-202506023.log");

    faultLogFilePath = faultLogManager->GetFaultLogFilePath(FaultLogType::SYS_WARNING, fileName);
    ASSERT_EQ(faultLogFilePath, "/data/log/warninglog/com.freeze.test001-202506023.log");
}

/**
 * @tc.name: FaultLogManager::GetFaultLogFileFdTest001
 * @tc.desc: Test calling GetFaultLogFileFd Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, GetFaultLogFileFdTest001, testing::ext::TestSize.Level3)
{
    std::unique_ptr<FaultLogManager> faultLogManager = std::make_unique<FaultLogManager>(nullptr);
    int faultLogFileFd = -1;
    faultLogManager->Init();
    std::string fileName = "com.freeze.test001-202506023.log";

    faultLogFileFd = faultLogManager->GetFaultLogFileFd(FaultLogType::APP_FREEZE, fileName);
    ASSERT_TRUE(faultLogFileFd > 0);
    close(faultLogFileFd);

    faultLogFileFd = faultLogManager->GetFaultLogFileFd(FaultLogType::SYS_WARNING, fileName);
    ASSERT_TRUE(faultLogFileFd > 0);
    close(faultLogFileFd);
}

/**
 * @tc.name: FaultLogUtilTest001
 * @tc.desc: check ExtractInfoFromFileName Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogUtilTest001, testing::ext::TestSize.Level3)
{
    std::string filename = "appfreeze-com.ohos.systemui-10006-20170805172159";
    auto info = ExtractInfoFromFileName(filename);
    ASSERT_EQ(info.pid, 0);
    ASSERT_EQ(info.faultLogType, FaultLogType::APP_FREEZE); // 4 : APP_FREEZE
    ASSERT_EQ(info.module, "com.ohos.systemui");
    ASSERT_EQ(info.id, 10006); // 10006 : test uid
}

/**
 * @tc.name: FaultLogUtilTest002
 * @tc.desc: check ExtractInfoFromTempFile Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogUtilTest002, testing::ext::TestSize.Level3)
{
    std::string filename = "appfreeze-10006-20170805172159";
    auto info = ExtractInfoFromTempFile(filename);
    ASSERT_EQ(info.faultLogType, FaultLogType::APP_FREEZE); // 4 : APP_FREEZE
    ASSERT_EQ(info.pid, 10006); // 10006 : test uid

    std::string filename3 = "jscrash-10006-20170805172159";
    auto info3 = ExtractInfoFromTempFile(filename3);
    ASSERT_EQ(info3.faultLogType, FaultLogType::JS_CRASH); // 3 : JS_CRASH
    ASSERT_EQ(info3.pid, 10006); // 10006 : test uid

    std::string filename4 = "cppcrash-10006-20170805172159";
    auto info4 = ExtractInfoFromTempFile(filename4);
    ASSERT_EQ(info4.faultLogType, FaultLogType::CPP_CRASH); // 2 : CPP_CRASH
    ASSERT_EQ(info4.pid, 10006); // 10006 : test uid

    std::string filename5 = "all-10006-20170805172159";
    auto info5 = ExtractInfoFromTempFile(filename5);
    ASSERT_EQ(info5.faultLogType, FaultLogType::ALL); // 0 : ALL
    ASSERT_EQ(info5.pid, 10006); // 10006 : test uid

    std::string filename6 = "other-10006-20170805172159";
    auto info6 = ExtractInfoFromTempFile(filename6);
    ASSERT_EQ(info6.faultLogType, -1); // -1 : other
    ASSERT_EQ(info6.pid, 10006); // 10006 : test uid
}

/**
 * @tc.name: FaultLogUtilTest003
 * @tc.desc: check ExtractInfoFromFileName Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogUtilTest003, testing::ext::TestSize.Level3)
{
    std::string filename = "appfreeze";
    auto info = ExtractInfoFromFileName(filename);
    ASSERT_EQ(info.pid, 0);
    ASSERT_EQ(info.time, 0);
}

/**
 * @tc.name: FaultLogUtilTest004
 * @tc.desc: test GetThreadStack
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogUtilTest004, testing::ext::TestSize.Level3)
{
    std::string path;
    auto stack = GetThreadStack(path, 0);
    ASSERT_TRUE(stack.empty());

    path = "/data/log/faultlog/faultlogger/appfreeze-com.example.jsinject-20010039-19700326211815.tmp";
    const int thread1 = 3443;
    stack = GetThreadStack(path, thread1);
    ASSERT_FALSE(stack.empty());

    const int thread2 = 3444;
    stack = GetThreadStack(path, thread2);
    ASSERT_FALSE(stack.empty());
}

/**
 * @tc.name: FaultLogUtilTest005
 * @tc.desc: test GetFaultNameByType
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogUtilTest005, testing::ext::TestSize.Level3)
{
    ASSERT_EQ(GetFaultNameByType(FaultLogType::JS_CRASH, true), "jscrash");
    ASSERT_EQ(GetFaultNameByType(FaultLogType::JS_CRASH, false), "JS_ERROR");

    ASSERT_EQ(GetFaultNameByType(FaultLogType::CPP_CRASH, true), "cppcrash");
    ASSERT_EQ(GetFaultNameByType(FaultLogType::CPP_CRASH, false), "CPP_CRASH");

    ASSERT_EQ(GetFaultNameByType(FaultLogType::APP_FREEZE, true), "appfreeze");
    ASSERT_EQ(GetFaultNameByType(FaultLogType::APP_FREEZE, false), "APP_FREEZE");

    ASSERT_EQ(GetFaultNameByType(FaultLogType::SYS_FREEZE, true), "sysfreeze");
    ASSERT_EQ(GetFaultNameByType(FaultLogType::SYS_FREEZE, false), "SYS_FREEZE");

    ASSERT_EQ(GetFaultNameByType(FaultLogType::SYS_WARNING, true), "syswarning");
    ASSERT_EQ(GetFaultNameByType(FaultLogType::SYS_WARNING, false), "SYS_WARNING");

    ASSERT_EQ(GetFaultNameByType(FaultLogType::RUST_PANIC, true), "rustpanic");
    ASSERT_EQ(GetFaultNameByType(FaultLogType::RUST_PANIC, false), "RUST_PANIC");

    ASSERT_EQ(GetFaultNameByType(FaultLogType::ADDR_SANITIZER, true), "sanitizer");
    ASSERT_EQ(GetFaultNameByType(FaultLogType::ADDR_SANITIZER, false), "ADDR_SANITIZER");

    ASSERT_EQ(GetFaultNameByType(FaultLogType::CJ_ERROR, true), "cjerror");
    ASSERT_EQ(GetFaultNameByType(FaultLogType::CJ_ERROR, false), "CJ_ERROR");

    ASSERT_EQ(GetFaultNameByType(FaultLogType::ALL, true), "Unknown");
    ASSERT_EQ(GetFaultNameByType(FaultLogType::ALL, false), "Unknown");
}

/**
 * @tc.name: FaultLogUtilTest006
 * @tc.desc: test GetLogTypeByName
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogUtilTest006, testing::ext::TestSize.Level3)
{
    ASSERT_EQ(GetLogTypeByName("jscrash"), FaultLogType::JS_CRASH);
    ASSERT_EQ(GetLogTypeByName("cppcrash"), FaultLogType::CPP_CRASH);
    ASSERT_EQ(GetLogTypeByName("appfreeze"), FaultLogType::APP_FREEZE);
    ASSERT_EQ(GetLogTypeByName("sysfreeze"), FaultLogType::SYS_FREEZE);
    ASSERT_EQ(GetLogTypeByName("syswarning"), FaultLogType::SYS_WARNING);
    ASSERT_EQ(GetLogTypeByName("sanitizer"), FaultLogType::ADDR_SANITIZER);
    ASSERT_EQ(GetLogTypeByName("cjerror"), FaultLogType::CJ_ERROR);
    ASSERT_EQ(GetLogTypeByName("all"), FaultLogType::ALL);
    ASSERT_EQ(GetLogTypeByName("ALL"), FaultLogType::ALL);
    ASSERT_EQ(GetLogTypeByName("Unknown"), -1);
}

/**
 * @tc.name: FaultLogUtilTest007
 * @tc.desc: test GetDebugSignalTempLogName
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogUtilTest007, testing::ext::TestSize.Level3)
{
    FaultLogInfo info;
    info.pid = 123;
    info.time = 123456789;
    auto fileName = GetDebugSignalTempLogName(info);
    ASSERT_EQ(fileName, "/data/log/faultlog/temp/stacktrace-123-123456789");
}

/**
 * @tc.name: FaultloggerAdapter.StartService
 * @tc.desc: Test calling FaultloggerAdapter.StartService Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultloggerAdapterTest001, testing::ext::TestSize.Level3)
{
    FaultloggerServiceOhos servicOhos;
    std::vector<std::u16string> args;
    args.emplace_back(u"-c _test.c");
    auto ret = servicOhos.Dump(0, args);
    ASSERT_EQ(ret, -1);

    args.emplace_back(u",");
    servicOhos.Dump(0, args);
    ASSERT_EQ(ret, -1);

    FaultLogInfoOhos info;
    // Cover GetOrSetFaultlogger return nullptr
    servicOhos.AddFaultLog(info);

    const int32_t faultType = 2;
    const int32_t maxNum = 10;
    ASSERT_EQ(servicOhos.QuerySelfFaultLog(faultType, maxNum), nullptr);
    servicOhos.Destroy();
}

/**
 * @tc.name: FaultloggerServiceOhos.StartService
 * @tc.desc: Test calling FaultloggerServiceOhos.StartService Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultloggerServiceOhosTest001, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    auto faultManagerService = std::make_shared<FaultLogManagerService>(plugin->GetWorkLoop(),
        plugin->faultLogManager_);
    FaultloggerServiceOhos serviceOhos;
    FaultloggerServiceOhos::StartService(faultManagerService);
    ASSERT_EQ(FaultloggerServiceOhos::GetOrSetFaultlogger(nullptr), faultManagerService);
    FaultLogInfoOhos info;
    info.time = std::time(nullptr);
    info.pid = getpid();
    info.uid = 0;
    info.faultLogType = 2;
    int fds[2] = {-1, -1}; // 2: one read pipe, one write pipe
    ASSERT_EQ(pipe(fds), 0) << "create pipe failed";
    info.pipeFd = fds[0];
    info.module = "FaultloggerUnittest333";
    info.reason = "unittest for SaveFaultLogInfo";
    serviceOhos.AddFaultLog(info);
    close(fds[1]);
    auto list = serviceOhos.QuerySelfFaultLog(2, 10);
    ASSERT_NE(list, nullptr);
    info.time = std::time(nullptr);
    info.pid = getpid();
    info.uid = 10;
    info.faultLogType = 2;
    info.pipeFd = 0;
    info.module = "FaultloggerUnittest333";
    info.reason = "unittest for SaveFaultLogInfo";
    serviceOhos.AddFaultLog(info);
    list = serviceOhos.QuerySelfFaultLog(2, 10);
    ASSERT_EQ(list, nullptr);
    info.time = std::time(nullptr);
    info.pid = getpid();
    info.uid = 0;
    info.faultLogType = 2;
    info.module = "FaultloggerUnittest333";
    info.reason = "unittest for SaveFaultLogInfo";
    serviceOhos.AddFaultLog(info);
    list = serviceOhos.QuerySelfFaultLog(8, 10);
    ASSERT_EQ(list, nullptr);
    serviceOhos.EnableGwpAsanGrayscale(false, 1000, 2000, 5);
    serviceOhos.DisableGwpAsanGrayscale();
    ASSERT_TRUE(serviceOhos.GetGwpAsanGrayscaleState() >= 0);
    serviceOhos.Destroy();
}

/**
 * @tc.name: FaultloggerServiceOhos.Dump
 * @tc.desc: Test calling FaultloggerServiceOhos.Dump Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultloggerServiceOhosTest002, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    auto faultManagerService = std::make_shared<FaultLogManagerService>(plugin->GetWorkLoop(),
        plugin->faultLogManager_);
    FaultloggerServiceOhos serviceOhos;
    FaultloggerServiceOhos::StartService(faultManagerService);
    ASSERT_EQ(FaultloggerServiceOhos::GetOrSetFaultlogger(nullptr), faultManagerService);
    auto fd = TEMP_FAILURE_RETRY(open("/data/test/testFile2", O_CREAT | O_WRONLY | O_TRUNC, 770));
    bool isSuccess = fd >= 0;
    if (!isSuccess) {
        ASSERT_FALSE(isSuccess);
        printf("Fail to create test result file.\n");
    } else {
        std::vector<std::u16string>args;
        args.push_back(u"Faultlogger");
        args.push_back(u"-l");
        serviceOhos.Dump(fd, args);
        args.push_back(u"&@#");
        ASSERT_EQ(serviceOhos.Dump(fd, args), -1);
        close(fd);
        fd = -1;
        std::string result;
        if (FileUtil::LoadStringFromFile("/data/test/testFile2", result)) {
            ASSERT_GT(result.length(), 0uL);
        } else {
            FAIL();
        }
        serviceOhos.Destroy();
    }
}

/**
 * @tc.name: FaultLogQueryResultOhosTest001
 * @tc.desc: test HasNext and GetNext
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogQueryResultOhosTest001, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    auto faultManagerService = std::make_shared<FaultLogManagerService>(plugin->GetWorkLoop(),
        plugin->faultLogManager_);
    FaultloggerServiceOhos serviceOhos;
    FaultloggerServiceOhos::StartService(faultManagerService);
    bool isSuccess = FaultloggerServiceOhos::GetOrSetFaultlogger(nullptr) == faultManagerService;
    if (!isSuccess) {
        ASSERT_FALSE(isSuccess);
        printf("FaultloggerServiceOhos start service error.\n");
    } else {
        auto remoteObject = serviceOhos.QuerySelfFaultLog(FaultLogType::CPP_CRASH, 10); // 10 : maxNum
        auto result = iface_cast<FaultLogQueryResultOhos>(remoteObject);
        ASSERT_NE(result, nullptr);
        if (result != nullptr) {
            while (result->HasNext()) {
                result->GetNext();
            }
        }
        auto getNextRes = result->GetNext();
        ASSERT_NE(result, nullptr);

        result->result_ = nullptr;
        bool hasNext = result->HasNext();
        ASSERT_FALSE(hasNext);
        getNextRes = result->GetNext();
        ASSERT_NE(result, nullptr);
    }
}

class TestFaultLogQueryResultStub : public FaultLogQueryResultStub {
public:
    TestFaultLogQueryResultStub() {}
    virtual ~TestFaultLogQueryResultStub() {}

    bool HasNext()
    {
        return false;
    }

    sptr<FaultLogInfoOhos> GetNext()
    {
        return nullptr;
    }

public:
    enum Code {
        DEFAULT = -1,
        HASNEXT = 0,
        GETNEXT,
    };
};

/**
 * @tc.name: FaultLogQueryResultStubTest001
 * @tc.desc: test OnRemoteRequest
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogQueryResultStubTest001, testing::ext::TestSize.Level3)
{
    TestFaultLogQueryResultStub faultLogQueryResultStub;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int ret = faultLogQueryResultStub.OnRemoteRequest(TestFaultLogQueryResultStub::Code::HASNEXT, data, reply, option);
    ASSERT_EQ(ret, -1);
    data.WriteInterfaceToken(FaultLogQueryResultStub::GetDescriptor());
    ret = faultLogQueryResultStub.OnRemoteRequest(TestFaultLogQueryResultStub::Code::HASNEXT, data, reply, option);
    ASSERT_EQ(ret, 0);
    data.WriteInterfaceToken(FaultLogQueryResultStub::GetDescriptor());
    ret = faultLogQueryResultStub.OnRemoteRequest(TestFaultLogQueryResultStub::Code::GETNEXT, data, reply, option);
    ASSERT_EQ(ret, -1);
    data.WriteInterfaceToken(FaultLogQueryResultStub::GetDescriptor());
    ret = faultLogQueryResultStub.OnRemoteRequest(TestFaultLogQueryResultStub::Code::DEFAULT, data, reply, option);
    ASSERT_EQ(ret, 305); // 305 : method not exist
}

class TestFaultLoggerServiceStub : public FaultLoggerServiceStub {
public:
    TestFaultLoggerServiceStub() {}
    virtual ~TestFaultLoggerServiceStub() {}

    void AddFaultLog(const FaultLogInfoOhos& info)
    {
    }

    sptr<IRemoteObject> QuerySelfFaultLog(int32_t faultType, int32_t maxNum)
    {
        return nullptr;
    }

    bool EnableGwpAsanGrayscale(bool alwaysEnabled, double sampleRate,
        double maxSimutaneousAllocations, int32_t duration)
    {
        return false;
    }

    void DisableGwpAsanGrayscale()
    {
    }

    uint32_t GetGwpAsanGrayscaleState()
    {
        return 0;
    }

    void Destroy()
    {
    }

public:
    enum Code {
        DEFAULT = -1,
        ADD_FAULTLOG = 0,
        QUERY_SELF_FAULTLOG,
        ENABLE_GWP_ASAN_GRAYSALE,
        DISABLE_GWP_ASAN_GRAYSALE,
        GET_GWP_ASAN_GRAYSALE,
        DESTROY,
    };
};

/**
 * @tc.name: FaultLoggerServiceStubTest001
 * @tc.desc: test OnRemoteRequest
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLoggerServiceStubTest001, testing::ext::TestSize.Level3)
{
    TestFaultLoggerServiceStub faultLoggerServiceStub;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int ret = faultLoggerServiceStub.OnRemoteRequest(TestFaultLoggerServiceStub::Code::ADD_FAULTLOG,
        data, reply, option);
    ASSERT_EQ(ret, -1);
    data.WriteInterfaceToken(FaultLoggerServiceStub::GetDescriptor());
    ret = faultLoggerServiceStub.OnRemoteRequest(TestFaultLoggerServiceStub::Code::ADD_FAULTLOG,
        data, reply, option);
    ASSERT_EQ(ret, 3); // 3 : ERR_FLATTEN_OBJECT
    data.WriteInterfaceToken(FaultLoggerServiceStub::GetDescriptor());
    ret = faultLoggerServiceStub.OnRemoteRequest(TestFaultLoggerServiceStub::Code::QUERY_SELF_FAULTLOG,
        data, reply, option);
    ASSERT_EQ(ret, -1);
    data.WriteInterfaceToken(FaultLoggerServiceStub::GetDescriptor());
    ret = faultLoggerServiceStub.OnRemoteRequest(TestFaultLoggerServiceStub::Code::DESTROY,
        data, reply, option);
    ASSERT_EQ(ret, 0);
    data.WriteInterfaceToken(FaultLoggerServiceStub::GetDescriptor());
    ret = faultLoggerServiceStub.OnRemoteRequest(TestFaultLoggerServiceStub::Code::DEFAULT,
        data, reply, option);
    ASSERT_EQ(ret, 305); // 305 : method not exist
}

/**
 * @tc.name: FaultloggerTest001
 * @tc.desc: Test calling Faultlogger.StartBootScan Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultloggerTest001, testing::ext::TestSize.Level3)
{
    StartHisyseventListen("RELIABILITY", "CPP_CRASH");
    time_t now = time(nullptr);
    std::vector<std::string> keyWords = { std::to_string(now) };
    faultEventListener->SetKeyWords(keyWords);
    std::string timeStr = GetFormatedTimeWithMillsec(now);
    std::string content = "Pid:101\nUid:0\nProcess name:BootScanUnittest\nReason:unittest for StartBootScan\n"
        "Fault thread info:\nTid:101, Name:BootScanUnittest\n#00 xxxxxxx\n#01 xxxxxxx\n";
    ASSERT_TRUE(FileUtil::SaveStringToFile("/data/log/faultlog/temp/cppcrash-101-" + std::to_string(now), content));
    auto plugin = GetFaultloggerInstance();
    FaultLogBootScan faultloggerListener(plugin->GetWorkLoop(), plugin->faultLogManager_);
    faultloggerListener.StartBootScan();

    // check faultlog file content
    std::string fileName = "/data/log/faultlog/faultlogger/cppcrash-BootScanUnittest-0-" + timeStr + ".log";
    ASSERT_TRUE(FileUtil::FileExists(fileName));
    ASSERT_GT(FileUtil::GetFileSize(fileName), 0ul);

    // check event database
    ASSERT_TRUE(faultEventListener->CheckKeyWords());
}

/**
 * @tc.name: FaultloggerTest002
 * @tc.desc: Test calling Faultlogger.StartBootScan Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultloggerTest002, testing::ext::TestSize.Level3)
{
    StartHisyseventListen("RELIABILITY", "CPP_CRASH_NO_LOG");
    std::vector<std::string> keyWords = { "BootScanUnittest" };
    faultEventListener->SetKeyWords(keyWords);
    time_t now = time(nullptr);
    std::string content = "Pid:102\nUid:0\nProcess name:BootScanUnittest\nReason:unittest for StartBootScan\n"
        "Fault thread info:\nTid:102, Name:BootScanUnittest\n";
    std::string fileName = "/data/log/faultlog/temp/cppcrash-102-" + std::to_string(now);
    ASSERT_TRUE(FileUtil::SaveStringToFile(fileName, content));
    auto plugin = GetFaultloggerInstance();
    FaultLogBootScan faultloggerListener(plugin->GetWorkLoop(), plugin->faultLogManager_);
    faultloggerListener.StartBootScan();
    ASSERT_FALSE(FileUtil::FileExists(fileName));

    // check event database
    ASSERT_TRUE(faultEventListener->CheckKeyWords());
}

/**
 * @tc.name: FaultloggerTest003
 * @tc.desc: Test calling Faultlogger.StartBootScan Func, for full log
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultloggerTest003, testing::ext::TestSize.Level3)
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
    auto plugin = GetFaultloggerInstance();
    FaultLogBootScan faultloggerListener(plugin->GetWorkLoop(), plugin->faultLogManager_);
    faultloggerListener.StartBootScan();

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
}

/**
 * @tc.name: FaultloggerTest004
 * @tc.desc: Test calling Faultlogger.StartBootScan Func, for full cpp crash log limit
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultloggerTest004, testing::ext::TestSize.Level3)
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
    auto plugin = GetFaultloggerInstance();
    FaultLogBootScan faultloggerListener(plugin->GetWorkLoop(), plugin->faultLogManager_);
    faultloggerListener.StartBootScan();
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
}

/**
 * @tc.name: ReportJsErrorToAppEventTest001
 * @tc.desc: create JS ERROR event and send it to hiappevent
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, ReportJsErrorToAppEventTest001, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    // has Error name、Error message、Error code、SourceCode、Stacktrace
    std::string summaryHasAll = R"~(Error name:summaryHasAll TypeError
Error message:Obj is not a Valid object
Error code:get BLO
SourceCode:CKSSvalue() {new Error("TestError");}
Stacktrace:
    at anonymous(entry/src/main/ets/pages/index.ets:76:10)
    at anonymous2(entry/src/main/ets/pages/index.ets:76:10)
    at anonymous3(entry/src/main/ets/pages/index.ets:76:10)
)~";
    ConstructJsErrorAppEvent(summaryHasAll, plugin);
    CheckKeyWordsInJsErrorAppEventFile("summaryHasAll");
}

/**
 * @tc.name: ReportJsErrorToAppEventTest002
 * @tc.desc: create JS ERROR event and send it to hiappevent
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, ReportJsErrorToAppEventTest002, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    // has Error name、Error message、Error code、SourceCode、Stacktrace
    std::string summaryNotFindSourcemap = R"~(Error name:summaryNotFindSourcemap Error
Error message:BussinessError 2501000: Operation failed.
Error code:2501000
Stacktrace:
Cannot get SourceMap info, dump raw stack:
  at anonymous(entry/src/main/ets/pages/index.ets:76:10)
  at anonymous2(entry/src/main/ets/pages/index.ets:76:10)
  at anonymous3(entry/src/main/ets/pages/index.ets:76:10)
)~";
    ConstructJsErrorAppEvent(summaryNotFindSourcemap, plugin);
    CheckDeleteStackErrorMessage("summaryNotFindSourcemap");
    CheckKeyWordsInJsErrorAppEventFile("summaryNotFindSourcemap");
}

/**
 * @tc.name: ReportJsErrorToAppEventTest003
 * @tc.desc: create JS ERROR event and send it to hiappevent
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, ReportJsErrorToAppEventTest003, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    // has Error name、Error message、SourceCode、Stacktrace
    std::string summaryHasNoErrorCode = R"~(Error name:summaryHasNoErrorCode TypeError
Error message:Obj is not a Valid object
SourceCode:CKSSvalue() {new Error("TestError");}
Stacktrace:
    at anonymous(entry/src/main/ets/pages/index.ets:76:10)
    at anonymous2(entry/src/main/ets/pages/index.ets:76:10)
    at anonymous3(entry/src/main/ets/pages/index.ets:76:10)
)~";
    ConstructJsErrorAppEvent(summaryHasNoErrorCode, plugin);
    CheckKeyWordsInJsErrorAppEventFile("summaryHasNoErrorCode");
}

/**
 * @tc.name: ReportJsErrorToAppEventTest004
 * @tc.desc: create JS ERROR event and send it to hiappevent
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, ReportJsErrorToAppEventTest004, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    // has Error name、Error message、Error code、Stacktrace
    std::string summaryHasNoSourceCode = R"~(Error name:summaryHasNoSourceCode TypeError
Error message:Obj is not a Valid object
Error code:get BLO
Stacktrace:
    at anonymous(entry/src/main/ets/pages/index.ets:76:10)
    at anonymous2(entry/src/main/ets/pages/index.ets:76:10)
    at anonymous3(entry/src/main/ets/pages/index.ets:76:10)
)~";
    ConstructJsErrorAppEvent(summaryHasNoSourceCode, plugin);
    CheckKeyWordsInJsErrorAppEventFile("summaryHasNoSourceCode");
}

/**
 * @tc.name: ReportJsErrorToAppEventTest005
 * @tc.desc: create JS ERROR event and send it to hiappevent
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, ReportJsErrorToAppEventTest005, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    // has Error name、Error message、Stacktrace
    std::string summaryHasNoErrorCodeAndSourceCode = R"~(Error name:summaryHasNoErrorCodeAndSourceCode TypeError
Error message:Obj is not a Valid object
Stacktrace:
    at anonymous(entry/src/main/ets/pages/index.ets:76:10)
    at anonymous2(entry/src/main/ets/pages/index.ets:76:10)
    at anonymous3(entry/src/main/ets/pages/index.ets:76:10)
)~";
    ConstructJsErrorAppEvent(summaryHasNoErrorCodeAndSourceCode, plugin);
    CheckKeyWordsInJsErrorAppEventFile("summaryHasNoErrorCodeAndSourceCode");
}

/**
 * @tc.name: ReportJsErrorToAppEventTest006
 * @tc.desc: create JS ERROR event and send it to hiappevent
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, ReportJsErrorToAppEventTest006, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    // has Error name、Error message、Error code、SourceCode
    std::string summaryHasNoStacktrace = R"~(Error name:summaryHasNoStacktrace TypeError
Error message:Obj is not a Valid object
Error code:get BLO
SourceCode:CKSSvalue() {new Error("TestError");}
Stacktrace:
)~";
    ConstructJsErrorAppEvent(summaryHasNoStacktrace, plugin);
    CheckKeyWordsInJsErrorAppEventFile("summaryHasNoStacktrace");
}

/**
 * @tc.name: ReportJsErrorToAppEventTest007
 * @tc.desc: create JS ERROR event and send it to hiappevent
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, ReportJsErrorToAppEventTest007, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    // has Error name、Error message
    std::string summaryHasErrorNameAndErrorMessage = R"~(Error name:summaryHasErrorNameAndErrorMessage TypeError
Error message:Obj is not a Valid object
Stacktrace:
)~";
    ConstructJsErrorAppEvent(summaryHasErrorNameAndErrorMessage, plugin);
    CheckKeyWordsInJsErrorAppEventFile("summaryHasErrorNameAndErrorMessage");
}

/**
 * @tc.name: ReportJsErrorToAppEventTest008
 * @tc.desc: create JS ERROR event and send it to hiappevent
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, ReportJsErrorToAppEventTest008, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    // has Error name、Error message
    std::string noKeyValue = R"~(Error name:summaryHasErrorNameAndErrorMessage TypeError
Error message:Obj is not a Valid object
Stacktrace:
)~";
    ConstructJsErrorAppEventWithNoValue(noKeyValue, plugin);
    CheckKeyWordsInJsErrorAppEventFile("noKeyValue");
}

/**
 * @tc.name: ReportJsErrorToAppEventTest009
 * @tc.desc: create JS ERROR event and send it to hiappevent
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, ReportJsErrorToAppEventTest009, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    ConstructJsErrorAppEventWithNoValue("", plugin);
    std::string oldFileName = "/data/test_jsError_info";
    ASSERT_TRUE(FileUtil::FileExists(oldFileName));
    auto ret = remove("/data/test_jsError_info");
    if (ret != 0) {
        GTEST_LOG_(INFO) << "remove /data/test_jsError_info failed";
    }
}

/**
 * @tc.name: ReportCjErrorToAppEventTest001
 * @tc.desc: create CJ ERROR event and send it to hiappevent
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, ReportCjErrorToAppEventTest001, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    // has Error name、Error message、Error code、SourceCode、Stacktrace
    std::string summary = R"~(Uncaught exception was found.
Exception info: throwing foo exception
Stacktrace:
    at anonymous(entry/src/main/ets/pages/index.cj:20)
    at anonymous2(entry/src/main/ets/pages/index.cj:33)
    at anonymous3(entry/src/main/ets/pages/index.cj:77)
)~";
    ConstructCjErrorAppEvent(summary, plugin);
    CheckKeyWordsInCjErrorAppEventFile("summary");
}

bool SendSysEvent(SysEventCreator sysEventCreator)
{
    auto plugin = GetFaultloggerInstance();
    auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    std::shared_ptr<Event> event = std::dynamic_pointer_cast<Event>(sysEvent);
    return plugin->OnEvent(event);
}

/**
 * @tc.name: OnEventTest001
 * @tc.desc: create JS ERROR event and send it to hiappevent
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, OnEventTest001, testing::ext::TestSize.Level3)
{
    {
        SysEventCreator sysEventCreator("AAFWK", "JSERROR", SysEventCreator::FAULT);
        sysEventCreator.SetKeyValue("name_", "JS_ERRORS");
        auto result = SendSysEvent(sysEventCreator);
        ASSERT_EQ(result, true);
    }
    {
        SysEventCreator sysEventCreator("AAFWK", "CPPCRASH", SysEventCreator::FAULT);
        sysEventCreator.SetKeyValue("name_", "RUST_PANIC");
        auto result = SendSysEvent(sysEventCreator);
        ASSERT_EQ(result, true);
    }
}

/**
 * @tc.name: AppFreezeCrashLogTest001
 * @tc.desc: test AddFaultLog, check F1/F2/F3
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, AppFreezeCrashLogTest001, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    FaultLogManagerService faultManagerService(plugin->GetWorkLoop(), plugin->faultLogManager_);
    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 20010039;
    info.pid = 7497;
    info.faultLogType = FaultLogType::APP_FREEZE;
    info.module = "com.example.jsinject";
    info.logPath = APPFREEZE_FAULT_FILE + "AppFreezeCrashLogTest001/" +
        "appfreeze-com.example.jsinject-20010039-19700326211815.tmp";
    faultManagerService.AddFaultLog(info);

    const std::string firstFrame = "/system/lib64/libeventhandler.z.so"
        "(OHOS::AppExecFwk::NoneIoWaiter::WaitFor(std::__1::unique_lock<std::__1::mutex>&, long)+204";
    ASSERT_EQ(info.sectionMap["FIRST_FRAME"], firstFrame);
    const std::string secondFrame = "/system/lib64/libeventhandler.z.so"
        "(OHOS::AppExecFwk::EventQueue::WaitUntilLocked"
        "(std::__1::chrono::time_point<std::__1::chrono::steady_clock, "
        "std::__1::chrono::duration<long long, std::__1::ratio<1l, 1000000000l> > > const&, "
        "std::__1::unique_lock<std::__1::mutex>&)+96";
    ASSERT_EQ(info.sectionMap["SECOND_FRAME"], secondFrame);
}

/**
 * @tc.name: AppFreezeCrashLogTest002
 * @tc.desc: test AddFaultLog, add TERMINAL_THREAD_STACK, check F1/F2/F3
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, AppFreezeCrashLogTest002, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    FaultLogManagerService faultManagerService(plugin->GetWorkLoop(), plugin->faultLogManager_);
    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 20010039;
    info.pid = 7497;
    info.faultLogType = FaultLogType::APP_FREEZE;
    info.module = "com.example.jsinject";
    info.logPath = APPFREEZE_FAULT_FILE + "AppFreezeCrashLogTest002/" +
        "appfreeze-com.example.jsinject-20010039-19700326211815.tmp";
    std::string binderSatck = "#00 pc 000000000006ca3c /system/lib64/libc.so(syscall+28)\n"
        "#01 pc 0000000000070cc4 "
        "/system/lib64/libc.so(__futex_wait_ex(void volatile*, bool, int, bool, timespec const*)+144)\n"
        "#02 pc 00000000000cf228 /system/lib64/libc.so(pthread_cond_wait+64)\n"
        "#03 pc 000000000051b55c /system/lib64/libGLES_mali.so\n"
        "#04 pc 00000000000cfce0 /system/lib64/libc.so(__pthread_start(void*)+40)\n"
        "#05 pc 0000000000072028 /system/lib64/libc.so(__start_thread+68)";
    info.sectionMap["TERMINAL_THREAD_STACK"] = binderSatck;
    faultManagerService.AddFaultLog(info);
    ASSERT_EQ(info.sectionMap["FIRST_FRAME"], "/system/lib64/libGLES_mali.so");
    ASSERT_TRUE(info.sectionMap["SECOND_FRAME"].empty());
    ASSERT_TRUE(info.sectionMap["LAST_FRAME"].empty());
}

/**
 * @tc.name: AppFreezeCrashLogTest003
 * @tc.desc: test AddFaultLog, add TERMINAL_THREAD_STACK("\n"), check F1/F2/F3
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, AppFreezeCrashLogTest003, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    FaultLogManagerService faultManagerService(plugin->GetWorkLoop(), plugin->faultLogManager_);
    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 20010039;
    info.pid = 7497;
    info.faultLogType = FaultLogType::APP_FREEZE;
    info.module = "com.example.jsinject";
    info.logPath = APPFREEZE_FAULT_FILE + "AppFreezeCrashLogTest003/" +
        "appfreeze-com.example.jsinject-20010039-19700326211815.tmp";
    std::string binderSatck = "#00 pc 000000000006ca3c /system/lib64/libc.so(syscall+28)\\n"
        "#01 pc 0000000000070cc4 "
        "/system/lib64/libc.so(__futex_wait_ex(void volatile*, bool, int, bool, timespec const*)+144)\\n"
        "#02 pc 00000000000cf228 /system/lib64/libc.so(pthread_cond_wait+64)\\n"
        "#03 pc 000000000051b55c /system/lib64/libGLES_mali.so\\n"
        "#04 pc 00000000000cfce0 /system/lib64/libc.so(__pthread_start(void*)+40)\\n"
        "#05 pc 0000000000072028 /system/lib64/libc.so(__start_thread+68)";
    info.sectionMap["TERMINAL_THREAD_STACK"] = binderSatck;
    faultManagerService.AddFaultLog(info);
    ASSERT_EQ(info.sectionMap["FIRST_FRAME"], "/system/lib64/libGLES_mali.so");
    ASSERT_TRUE(info.sectionMap["SECOND_FRAME"].empty());
    ASSERT_TRUE(info.sectionMap["LAST_FRAME"].empty());
}

/**
 * @tc.name: FaultloggerUnittest001
 * @tc.desc: test IsValidPath
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultloggerUnittest001, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    ASSERT_NE(plugin, nullptr);
    plugin->hasInit_ = true;
    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 20010039;
    info.pid = 7497;
    info.faultLogType = FaultLogType::APP_FREEZE;
    info.module = "com.example.jsinject";
    info.logPath = "/proc/self/status";
    FaultLogManagerService faultManagerService(plugin->GetWorkLoop(), plugin->faultLogManager_);
    faultManagerService.AddFaultLog(info);
    ASSERT_TRUE(info.sectionMap.empty());

    info.logPath = "/proc/self/test";
    faultManagerService.AddFaultLog(info);
    ASSERT_TRUE(info.sectionMap.empty());
}

/**
 * @tc.name: FaultloggerUnittest002
 * @tc.desc: test QuerySelfFaultLog and GetMemoryStrByPid
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultloggerUnittest002, testing::ext::TestSize.Level3)
{
    FaultLogFreeze faultAppFreeze;
    std::string str = faultAppFreeze.GetMemoryStrByPid(-1);
    ASSERT_EQ(str, "");
    str = faultAppFreeze.GetMemoryStrByPid(1);
    ASSERT_NE(str, "");
}

/**
 * @tc.name: FaultlogDatabaseUnittest001
 * @tc.desc: test RunSanitizerd
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultlogDatabaseUnittest001, testing::ext::TestSize.Level3)
{
    FaultLogDatabase *faultLogDb = new FaultLogDatabase(GetHiviewContext().GetSharedWorkLoop());
    std::list<FaultLogInfo> queryResult = faultLogDb->GetFaultInfoList("com.example.myapplication", 0, -1, 10);
    ASSERT_EQ(queryResult.size(), 0);
    queryResult = faultLogDb->GetFaultInfoList("com.example.myapplication", 0, 8, 10);
    ASSERT_EQ(queryResult.size(), 0);
    queryResult = faultLogDb->GetFaultInfoList("com.example.myapplication", 1, 2, 10);
    ASSERT_EQ(queryResult.size(), 0);
    queryResult = faultLogDb->GetFaultInfoList("com.example.myapplication", 1, 0, 10);
    ASSERT_EQ(queryResult.size(), 0);

    FaultLogInfo info;
    info.faultLogType = FaultLogType::SYS_FREEZE;
    faultLogDb->eventLoop_ = nullptr;
    faultLogDb->SaveFaultLogInfo(info);

    bool res = faultLogDb->IsFaultExist(1, 1, -1);
    ASSERT_FALSE(res);
    res = faultLogDb->IsFaultExist(1, 1, 8);
    ASSERT_FALSE(res);
}

/**
 * @tc.name: FaultlogUtilUnittest001
 * @tc.desc: test RunSanitizerd
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultlogUtilUnittest001, testing::ext::TestSize.Level3)
{
    std::string result = GetFaultNameByType(FaultLogType::ADDR_SANITIZER, false);
    ASSERT_EQ(result, "ADDR_SANITIZER");

    FaultLogInfo info;
    info.module = "test/test";
    info.faultLogType = FaultLogType::ADDR_SANITIZER;
    info.sanitizerType = "TSAN";
    std::string str = GetFaultLogName(info);
    ASSERT_EQ(str, "tsan-test-0-19700101080000000.log");
    info.sanitizerType = "UBSAN";
    str = GetFaultLogName(info);
    ASSERT_EQ(str, "ubsan-test-0-19700101080000000.log");
    info.sanitizerType = "GWP-ASAN";
    str = GetFaultLogName(info);
    ASSERT_EQ(str, "gwpasan-test-0-19700101080000000.log");
    info.sanitizerType = "HWASAN";
    str = GetFaultLogName(info);
    ASSERT_EQ(str, "hwasan-test-0-19700101080000000.log");
    info.sanitizerType = "ASAN";
    str = GetFaultLogName(info);
    ASSERT_EQ(str, "asan-test-0-19700101080000000.log");
    info.sanitizerType = "GWP-ASANS";
    str = GetFaultLogName(info);
    ASSERT_EQ(str, "sanitizer-test-0-19700101080000000.log");

    str = RegulateModuleNameIfNeed("");
    ASSERT_EQ(str, "");
}

/**
 * @tc.name: FaultlogUtilUnittest002
 * @tc.desc: test GetFaultNameByType
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultlogUtilUnittest002, testing::ext::TestSize.Level3)
{
    std::string result = GetFaultNameByType(FaultLogType::SYS_FREEZE, false);
    ASSERT_EQ(result, "SYS_FREEZE");
    result = GetFaultNameByType(FaultLogType::SYS_WARNING, false);
    ASSERT_EQ(result, "SYS_WARNING");
    result = GetFaultNameByType(FaultLogType::CJ_ERROR, false);
    ASSERT_EQ(result, "CJ_ERROR");
}

/**
 * @tc.name: FaultloggerServiceOhosUnittest001
 * @tc.desc: test RunSanitizerd
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultloggerServiceOhosUnittest001, testing::ext::TestSize.Level3)
{
    FaultloggerServiceOhos faultloggerServiceOhos;
    std::vector<std::u16string> args;
    args.push_back(u"*m");
    int32_t result = faultloggerServiceOhos.Dump(1, args);
    ASSERT_EQ(result, -1);
    faultloggerServiceOhos.Destroy();
}

/**
 * @tc.name: ReadHilogUnittest001
 * @tc.desc: Faultlogger::ReadHilog
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, ReadHilogUnittest001, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. write log to hilog.
     */
    HIVIEW_LOGI("write log to hilog");

    /**
     * @tc.steps: step2. Create a pipe.
     */
    int fds[2] = {-1, -1}; // 2: one read pipe, one write pipe
    int ret = pipe(fds);
    ASSERT_EQ(ret, 0) << "Failed to create pipe for get log.";

    /**
     * @tc.steps: step3. ReadHilog.
     */
    int32_t pid = getpid();
    int childPid = fork();
    ASSERT_GE(childPid, 0);
    if (childPid == 0) {
        syscall(SYS_close, fds[0]);


        int rc = DoGetHilogProcess(pid, fds[1]);
        syscall(SYS_close, fds[1]);
        _exit(rc);
    } else {
        syscall(SYS_close, fds[1]);
        // read log from fds[0]
        HIVIEW_LOGI("read hilog start");
        std::string log = ReadHilogTimeout(fds[0]);
        syscall(SYS_close, fds[0]);
        ASSERT_TRUE(!log.empty());
    }
    waitpid(childPid, nullptr, 0);
}

/**
 * @tc.name: ReadHilogUnittest002
 * @tc.desc: Faultlogger::ReadHilog
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, ReadHilogUnittest002, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. write log to hilog.
     */
    HIVIEW_LOGI("write log to hilog");

    /**
     * @tc.steps: step2. Create a pipe.
     */
    int fds[2] = {-1, -1}; // 2: one read pipe, one write pipe
    int ret = pipe(fds);
    ASSERT_EQ(ret, 0) << "Failed to create pipe for get log.";

    /**
     * @tc.steps: step3. ReadHilog.
     */
    int32_t pid = getpid();
    int childPid = fork();
    ASSERT_GE(childPid, 0);
    if (childPid == 0) {
        syscall(SYS_close, fds[0]);
        sleep(7); // Delay for 7 seconds, causing the read end to timeout and exit

        int rc = DoGetHilogProcess(pid, fds[1]);
        syscall(SYS_close, fds[1]);
        _exit(rc);
    } else {
        syscall(SYS_close, fds[1]);
        // read log from fds[0]
        HIVIEW_LOGI("read hilog start");
        std::string log = ReadHilogTimeout(fds[0]);
        syscall(SYS_close, fds[0]);
        ASSERT_TRUE(log.empty());
    }
    waitpid(childPid, nullptr, 0);
}

/**
 * @tc.name: ReadHilogUnittest003
 * @tc.desc: Faultlogger::ReadHilog
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, ReadHilogUnittest003, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. write log to hilog.
     */
    HIVIEW_LOGI("write log to hilog");

    /**
     * @tc.steps: step2. Create a pipe.
     */
    int fds[2] = {-1, -1}; // 2: one read pipe, one write pipe
    int ret = pipe(fds);
    ASSERT_EQ(ret, 0) << "Failed to create pipe for get log.";

    /**
     * @tc.steps: step3. ReadHilog.
     */
    int32_t pid = getpid();
    int childPid = fork();
    ASSERT_GE(childPid, 0);
    if (childPid == 0) {
        syscall(SYS_close, fds[0]);
        syscall(SYS_close, fds[1]);
        _exit(0);
    } else {
        syscall(SYS_close, fds[1]);
        // read log from fds[0]
        HIVIEW_LOGI("read hilog start");
        std::string log = ReadHilogTimeout(fds[0]);
        syscall(SYS_close, fds[0]);
        ASSERT_TRUE(log.empty());
    }
    waitpid(childPid, nullptr, 0);
}

/**
 * @tc.name: FaultlogLimit001
 * @tc.desc: Test calling DoFaultLogLimit Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultlogLimit001, testing::ext::TestSize.Level3)
{
    time_t now = time(nullptr);
    std::vector<std::string> keyWords = { std::to_string(now) };
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
    for (int i = 0; i < 100000; i++) {
        content += fillMapsContent;
    }
    content += "HiLog:\n";
    for (int i = 0; i < 10000; i++) {
        content += fillMapsContent;
    }

    std::string filePath = "/data/log/faultlog/temp/cppcrash-114-" + std::to_string(now);
    ASSERT_TRUE(FileUtil::SaveStringToFile(filePath, content));
    FaultLogCppCrash faultCppCrash;
    faultCppCrash.DoFaultLogLimit(filePath, 1);

    filePath = "/data/log/faultlog/temp/cppcrash-115-" + std::to_string(now);
    content = "hello";
    ASSERT_TRUE(FileUtil::SaveStringToFile(filePath, content));
    faultCppCrash.DoFaultLogLimit(filePath, 1);

    FaultLogInfo info;
    std::string stack = "adad";
    faultCppCrash.FillStackInfo(info, stack);

    std::string tempCont = "adbc";
    faultCppCrash.RemoveHiLogSection(tempCont);
    faultCppCrash.TruncateLogIfExceedsLimit(tempCont);
}

/**
 * @tc.name: FaultLogManagerService001
 * @tc.desc: Test calling querySelfFaultLog Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogManagerService001, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    FaultLogManagerService faultManagerService(plugin->GetWorkLoop(), plugin->faultLogManager_);
    faultManagerService.QuerySelfFaultLog(100001, 0, 10, 101);
    faultManagerService.QuerySelfFaultLog(100001, 0, 4, 101);
    auto ret = faultManagerService.QuerySelfFaultLog(100001, 0, -1, 101);
    ASSERT_TRUE(ret == nullptr);
}

/**
 * @tc.name: FaultLogManagerService002
 * @tc.desc: Test calling GwpAsanGrayscal Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogManagerService002, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    FaultLogManagerService faultManagerService(plugin->GetWorkLoop(), plugin->faultLogManager_);
    faultManagerService.EnableGwpAsanGrayscale(false, 1000, 2000, 5, static_cast<int64_t>(getuid()));
    faultManagerService.EnableGwpAsanGrayscale(true, 2523, 2000, 5, static_cast<int64_t>(getuid()));
    faultManagerService.DisableGwpAsanGrayscale(static_cast<int64_t>(getuid()));
    ASSERT_TRUE(faultManagerService.GetGwpAsanGrayscaleState(static_cast<int64_t>(getuid())) >= 0);
}

/**
 * @tc.name: FaultloggerListener001
 * @tc.desc: Test calling FaultloggerListener Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultloggerListener001, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    FaultLogBootScan faultloggerListener(plugin->GetWorkLoop(), plugin->faultLogManager_);
    std::string fileName = "/data/log/faultlog/temp/freeze-114-";

    bool ret = faultloggerListener.IsCrashType(fileName);
    ASSERT_EQ(ret, false);

    time_t now = time(nullptr);
    faultloggerListener.IsInValidTime(fileName, now);
    ASSERT_TRUE(faultloggerListener.IsInValidTime(fileName, 0));

    FaultLogBootScan faultloggerListenerEmpty(nullptr, plugin->faultLogManager_);
    faultloggerListenerEmpty.AddBootScanEvent();

    Event msg("hello");
    faultloggerListenerEmpty.OnUnorderedEvent(msg);
}

/**
 * @tc.name: FaultLogCjError001
 * @tc.desc: Test cj reportToAppEvent Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogCjError001, testing::ext::TestSize.Level3)
{
    FaultLogCjError cjError;
    FaultLogInfo info;
    info.reportToAppEvent = false;
    bool ret = cjError.ReportToAppEvent(nullptr, info);
    EXPECT_EQ(ret, false);
}

bool CheckProcessName(const std::string &dirName, const std::string &procName)
{
    std::string path = "/proc/" + dirName + "/comm";
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::string comm;
    std::getline(file, comm);
    file.close();

    return comm == procName;
}

pid_t GetPidByProcessName(const std::string &procName)
{
    DIR *procDir = opendir("/proc");
    if (procDir == nullptr) {
        perror("opendir /proc");
        return -1;
    }

    pid_t pid = -1;
    struct dirent *entry;
    while ((entry = readdir(procDir)) != nullptr) {
        if (entry->d_type != DT_DIR) {
            continue;
        }

        std::string dirName(entry->d_name);
        if (!std::all_of(dirName.begin(), dirName.end(), ::isdigit)) {
            continue;
        }

        if (CheckProcessName(dirName, procName)) {
            pid = std::stoi(dirName);
            break;
        }
    }

    closedir(procDir);
    return pid;
}

/**
 * @tc.name: FaultLogAppFreeze001
 * @tc.desc: Test faultAppFreeze GetFreezeHilogByPid Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogAppFreeze001, testing::ext::TestSize.Level3)
{
    FaultLogFreeze faultAppFreeze;
    faultAppFreeze.GetFreezeHilogByPid(1);
    std::string processName = "faultloggerd";
    auto pid = GetPidByProcessName(processName);
    auto hilog = faultAppFreeze.GetFreezeHilogByPid(pid);
    ASSERT_TRUE(!hilog.empty());
}
/**
 * @tc.name: FaultLogAppFreeze002
 * @tc.desc: Test faultAppFreeze ReportEventToAppEvent Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogAppFreeze002, testing::ext::TestSize.Level3)
{
    FaultLogFreeze faultAppFreeze;
    FaultLogInfo info;
    info.reportToAppEvent = false;
    bool ret = faultAppFreeze.ReportEventToAppEvent(info);

    std::string logPath = "1111.txt";
    faultAppFreeze.DoFaultLogLimit(logPath, 2);

    ASSERT_FALSE(ret);
}

/**
 * @tc.name: FaultLogJsError001
 * @tc.desc: Test jsError ReportToAppEvent Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogJsError001, testing::ext::TestSize.Level3)
{
    FaultLogJsError jserror;

    FaultLogInfo info;
    info.reportToAppEvent = false;
    bool ret = jserror.ReportToAppEvent(nullptr, info);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name: FaultLogSanitizer001
 * @tc.desc: Test cjError ReportToAppEvent Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogSanitizer001, testing::ext::TestSize.Level3)
{
    std::string summmay = "adaf";
    SysEventCreator sysEventCreator("CJ_RUNTIME", "CJERROR", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("SUMMARY", summmay);
    sysEventCreator.SetKeyValue("name_", "CJ_ERROR");
    sysEventCreator.SetKeyValue("happenTime_", 1670248360359); // 1670248360359 : Simulate happenTime_ value
    sysEventCreator.SetKeyValue("REASON", "std.core:Exception");
    sysEventCreator.SetKeyValue("tz_", "+0800");
    sysEventCreator.SetKeyValue("pid_", 2413); // 2413 : Simulate pid_ value
    sysEventCreator.SetKeyValue("tid_", 2413); // 2413 : Simulate tid_ value
    sysEventCreator.SetKeyValue("what_", 3); // 3 : Simulate what_ value
    sysEventCreator.SetKeyValue("PACKAGE_NAME", "com.ohos.systemui");
    sysEventCreator.SetKeyValue("VERSION", "1.0.0");
    sysEventCreator.SetKeyValue("TYPE", 3); // 3 : Simulate TYPE value
    sysEventCreator.SetKeyValue("VERSION", "1.0.0");

    auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    FaultLogSanitizer san;
    FaultLogInfo info;
    info.reportToAppEvent = false;
    bool ret = san.ReportToAppEvent(sysEvent, info);
    EXPECT_EQ(ret, false);

    sysEventCreator.SetKeyValue("LOG_PATH", "1.0.0");
    sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    info.reportToAppEvent = true;
    ret = san.ReportToAppEvent(sysEvent, info);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: FaultLogSanitizer002
 * @tc.desc: Test ParseSanitizerEasyEvent Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogSanitizer002, testing::ext::TestSize.Level3)
{
    FaultLogSanitizer sanitizer;
    auto runTest = [&sanitizer](const std::string& input,
        const std::unordered_map<std::string, std::string>& expected) {
        SysEventCreator sysEventCreator("RELIABILITY", "ADDR_SANITIZER", SysEventCreator::FAULT);
        sysEventCreator.SetKeyValue("DATA", input);
 
        auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
        sanitizer.ParseSanitizerEasyEvent(*sysEvent);
 
        for (const auto& [key, val] : expected) {
            EXPECT_EQ(sysEvent->GetEventValue(key), val);
        }
    };
    runTest("FAULT_TYPE:8;MODULE:debugsanitizer;SUMMARY:debug text with ; and :",
            {{"FAULT_TYPE", "8"},
             {"MODULE", "debugsanitizer"},
             {"SUMMARY", "debug text with ; and :"}});
    runTest("FAULT_TYPE;MODULE:debugsanitizer:2;SUMMARY:debug text with ; and :",
            {{"FAULT_TYPE", ""},
             {"MODULE", "debugsanitizer:2"},
             {"SUMMARY", "debug text with ; and :"}});
    runTest("SUMMARY:only summary",
            {{"SUMMARY", "only summary"}});
    runTest("FAULT_TYPE:;SUMMARY:only summary",
            {{"FAULT_TYPE", ""},
             {"SUMMARY", "only summary"}});
}

/**
 * @tc.name: EventHandlerStrategyFactory001
 * @tc.desc: Test CreateFaultLogEvent Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, EventHandlerStrategyFactory001, testing::ext::TestSize.Level3)
{
    FaultLogEventFactory factory;
    std::string eventName = "ADDR_SANITIZER";
    auto fault = factory.CreateFaultLogEvent(eventName);
    ASSERT_NE(fault, nullptr);
    fault  = factory.CreateFaultLogEvent("abcd");
    ASSERT_EQ(fault, nullptr);
}

/**
 * @tc.name: GetFaultloggerInstance001
 * @tc.desc: Test onEvent and IsInterestedPipelineEvent
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, GetFaultloggerInstance001, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    std::shared_ptr<Event> event = nullptr;
    bool ret = plugin->OnEvent(event);
    ASSERT_EQ(ret, false);
    ret = plugin->IsInterestedPipelineEvent(event);
    ASSERT_EQ(ret, false);
}

/**
 * @tc.name: ExtractSubMoudleName001
 * @tc.desc: Test ExtractSubMoudleName func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, ExtractSubMoudleName001, testing::ext::TestSize.Level3)
{
    std::string moduleName = "com.ohos.sceneboard:";
    ASSERT_FALSE(ExtractSubMoudleName(moduleName));
    std::string endName = "";
    ASSERT_EQ(endName, moduleName);
}

/**
 * @tc.name: ExtractSubMoudleName002
 * @tc.desc: Test ExtractSubMoudleName func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, ExtractSubMoudleName002, testing::ext::TestSize.Level3)
{
    std::string moduleName = "com.ohos.sceneboard:123ExtractSubMoudleName/123ExtractSubMoudleName_321";
    ASSERT_TRUE(ExtractSubMoudleName(moduleName));
    std::string endName = "ExtractSubMoudleName_123ExtractSubMoudleName";
    ASSERT_EQ(endName, moduleName);
}

/**
 * @tc.name: ExtractSubMoudleName003
 * @tc.desc: Test ExtractSubMoudleName func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, ExtractSubMoudleName003, testing::ext::TestSize.Level3)
{
    std::string moduleName = "com.ohos.test";
    ASSERT_FALSE(ExtractSubMoudleName(moduleName));
    std::string moduleName2 = "com.ohos.sceneboard:test:1";
    ASSERT_TRUE(ExtractSubMoudleName(moduleName2));
    std::string endName = "test";
    ASSERT_EQ(endName, moduleName2);
}

/**
 * @tc.name: FaultLogBootScan001
 * @tc.desc: Test big file
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogBootScan001, testing::ext::TestSize.Level3)
{
    std::string path = "/data/test/test_data/FaultLogBootScan001/cppcrash-197-1502809621426";
    ASSERT_FALSE(FaultLogBootScan::IsCrashTempBigFile(path));
}

/**
 * @tc.name: FaultLogBootScan002
 * @tc.desc: Test big file, file size exceeds limit
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogBootScan002, testing::ext::TestSize.Level3)
{
    std::string path = "/data/test/test_data/FaultLogBootScan002/cppcrash-197-1502809621426";
    std::ofstream file(path, std::ios::app);
    ASSERT_TRUE(file.is_open());
    file << std::setfill('0') << std::setw(1024 * 1024 * 5) << 0;
    file.close();

    ASSERT_TRUE(FaultLogBootScan::IsCrashTempBigFile(path));
}
} // namespace HiviewDFX
} // namespace OHOS
