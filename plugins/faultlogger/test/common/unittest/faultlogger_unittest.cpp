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
#include <sys/types.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "bundle_mgr_client.h"
#include "event.h"
#include "faultlog_util.h"
#include "faultlog_database.h"
#include "faultlogger.h"
#include "faultevent_listener.h"
#include "faultlog_formatter.h"
#include "faultlog_info_ohos.h"
#include "faultlog_query_result_ohos.h"
#include "faultlogger_adapter.h"
#include "faultlogger_service_ohos.h"
#include "file_util.h"
#include "hisysevent_manager.h"
#include "hiview_global.h"
#include "hiview_platform.h"
#include "ipc_skeleton.h"
#include "json/json.h"
#include "log_analyzer.h"
#include "sys_event.h"
#include "sys_event_dao.h"
#include "zip_helper.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace HiviewDFX {
static std::shared_ptr<FaultEventListener> faultEventListener = nullptr;

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
        GTEST_LOG_(INFO) << "========name:" << exception["name"];
        if (exception["name"] == "" || exception["name"] == "none") {
            matchCount--;
        }
        GTEST_LOG_(INFO) << "========message:" << exception["message"];
        if (exception["message"] == "" || exception["message"] == "none") {
            matchCount--;
        }
        GTEST_LOG_(INFO) << "========stack:" << exception["stack"];
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
        std::cout << "length:" << length << std::endl;
        std::string oldFileName = "/data/test_jsError_info";
        int count = CheckKeyWordsInFile(oldFileName, keywords, length, true);
        std::cout << "count:" << count << std::endl;
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

    static void CheckDeleteStackErrorMessage(std::string name)
    {
        std::string keywords[] = {"\"Cannot get SourceMap info, dump raw stack:"};
        int length = sizeof(keywords) / sizeof(keywords[0]);
        std::cout << "========length:" << length << std::endl;
        std::string oldFileName = "/data/test_jsError_info";
        int count = CheckKeyWordsInFile(oldFileName, keywords, length, true);
        std::cout << "========count:" << count << std::endl;
        ASSERT_NE(count, length) << "check delete stack error message failed";
    }
};

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
    int fd = TEMP_FAILURE_RETRY(open("/data/test/testFile", O_CREAT | O_WRONLY | O_TRUNC, 770));
    if (fd < 0) {
        printf("Fail to create test result file.\n");
        return;
    }

    std::vector<std::string> cmds;
    plugin->Dump(fd, cmds);
    cmds.push_back("Faultlogger");
    plugin->Dump(fd, cmds);
    cmds.push_back("-l");
    plugin->Dump(fd, cmds);
    cmds.push_back("-f");
    plugin->Dump(fd, cmds);
    cmds.push_back("cppcrash-ModuleName-10-20201209103823");
    plugin->Dump(fd, cmds);
    cmds.push_back("-d");
    plugin->Dump(fd, cmds);
    cmds.push_back("-t");
    plugin->Dump(fd, cmds);
    cmds.push_back("20201209103823");
    plugin->Dump(fd, cmds);
    cmds.push_back("-m");
    plugin->Dump(fd, cmds);
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
    if (fd < 0) {
        printf("Fail to create test result file.\n");
        return;
    }

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
    int fd = TEMP_FAILURE_RETRY(open("/data/test/testFile", O_CREAT | O_WRONLY | O_TRUNC, 770));
    if (fd < 0) {
        printf("Fail to create test result file.\n");
        return;
    }

    std::vector<std::vector<std::string>> cmds = {
        {"-t", "cppcrash--10-20201209103823"},
        {"-m", ""},
        {"-l", ""},
        {"-xx"}
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
    plugin->AddFaultLog(info);
    std::string timeStr = GetFormatedTime(info.time);
    std::string appName = GetApplicationNameById(info.id);
    if (appName.size() == 0) {
        appName = info.module;
    }
    std::string fileName = "/data/log/faultlog/faultlogger/cppcrash-" + appName + "-" +
        std::to_string(info.id) + "-" + timeStr;
    ASSERT_EQ(FileUtil::FileExists(fileName), true);
    ASSERT_GT(FileUtil::GetFileSize(fileName), 0ul);
    auto parsedInfo = plugin->GetFaultLogInfo(fileName);
    ASSERT_EQ(parsedInfo->module, appName);
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
    if (ret == 0) {
        GTEST_LOG_(INFO) << "remove /data/test_jsError_info failed";
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
HWTEST_F(FaultloggerUnittest, AddFaultLogTest001, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    FaultLogInfo info;
    plugin->hasInit_ = false;
    plugin->AddFaultLog(info);

    plugin->hasInit_ = true;
    info.faultLogType = -1;
    plugin->AddFaultLog(info);

    info.faultLogType = 8; // 8 : 8 is bigger than FaultLogType::ADDR_SANITIZER
    plugin->AddFaultLog(info);

    info.faultLogType = FaultLogType::CPP_CRASH;
    info.id = 1;
    info.module = "com.example.myapplication";
    info.time = 1607161163;
    info.pid = 7496;
    plugin->AddFaultLog(info);
    std::string timeStr = GetFormatedTime(info.time);
    std::string fileName = "/data/log/faultlog/faultlogger/cppcrash-com.example.myapplication-0-" + timeStr;
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
    auto plugin = GetFaultloggerInstance();
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
    plugin->AddPublicInfo(info);
    std::string timeStr = GetFormatedTime(info.time);
    std::string fileName = "/data/log/faultlog/faultlogger/cppcrash-com.example.myapplication-0-" + timeStr;
    ASSERT_EQ(FileUtil::FileExists(fileName), true);
}

/**
 * @tc.name: GetFreezeJsonCollectorTest001
 * @tc.desc: test GetFreezeJsonCollector
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, GetFreezeJsonCollectorTest001, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
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
    FreezeJsonUtil::FreezeJsonCollector collector = plugin->GetFreezeJsonCollector(info);
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
    if (ret == 0) {
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
 * @tc.name: GetListenerName
 * @tc.desc: Test calling GetListenerName Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, GetListenerName, testing::ext::TestSize.Level3)
{
    auto testPlugin = GetFaultloggerInstance();
    ASSERT_EQ(testPlugin->GetListenerName(), "FaultLogger");
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
 * @tc.name: FaultLogManager::SaveFaultInfoToRawDb
 * @tc.desc: Test calling SaveFaultInfoToRawDb Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogManagerTest001, testing::ext::TestSize.Level3)
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
    for (int i = 1; i < 7; i++) {
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
 * @tc.name: FaultloggerAdapter.StartService
 * @tc.desc: Test calling FaultloggerAdapter.StartService Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultloggerAdapterTest001, testing::ext::TestSize.Level3)
{
    FaultloggerAdapter::StartService(nullptr);
    ASSERT_EQ(FaultloggerServiceOhos::GetOrSetFaultlogger(nullptr), nullptr);

    Faultlogger faultlogger;
    FaultloggerAdapter::StartService(&faultlogger);
    ASSERT_EQ(FaultloggerServiceOhos::GetOrSetFaultlogger(nullptr), &faultlogger);
}

/**
 * @tc.name: FaultloggerServiceOhos.StartService
 * @tc.desc: Test calling FaultloggerServiceOhos.StartService Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultloggerServiceOhosTest001, testing::ext::TestSize.Level3)
{
    auto service = GetFaultloggerInstance();
    FaultloggerServiceOhos serviceOhos;
    FaultloggerServiceOhos::StartService(service.get());
    ASSERT_EQ(FaultloggerServiceOhos::GetOrSetFaultlogger(nullptr), service.get());
    FaultLogInfoOhos info;
    info.time = std::time(nullptr);
    info.pid = getpid();
    info.uid = 0;
    info.faultLogType = 2;
    info.module = "FaultloggerUnittest333";
    info.reason = "unittest for SaveFaultLogInfo";
    serviceOhos.AddFaultLog(info);
    auto list = serviceOhos.QuerySelfFaultLog(2, 10);
    ASSERT_NE(list, nullptr);
    info.time = std::time(nullptr);
    info.pid = getpid();
    info.uid = 10;
    info.faultLogType = 2;
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

    serviceOhos.Destroy();
}

/**
 * @tc.name: FaultloggerServiceOhos.Dump
 * @tc.desc: Test calling FaultloggerServiceOhos.Dump Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultloggerServiceOhosTest002, testing::ext::TestSize.Level3)
{
    auto service = GetFaultloggerInstance();
    FaultloggerServiceOhos serviceOhos;
    FaultloggerServiceOhos::StartService(service.get());
    ASSERT_EQ(FaultloggerServiceOhos::GetOrSetFaultlogger(nullptr), service.get());
    auto fd = TEMP_FAILURE_RETRY(open("/data/test/testFile2", O_CREAT | O_WRONLY | O_TRUNC, 770));
    if (fd < 0) {
        printf("Fail to create test result file.\n");
        return;
    }
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

/**
 * @tc.name: FaultLogQueryResultOhosTest001
 * @tc.desc: test HasNext and GetNext
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultLogQueryResultOhosTest001, testing::ext::TestSize.Level3)
{
    auto service = GetFaultloggerInstance();
    FaultloggerServiceOhos serviceOhos;
    FaultloggerServiceOhos::StartService(service.get());
    if (FaultloggerServiceOhos::GetOrSetFaultlogger(nullptr) != service.get()) {
        printf("FaultloggerServiceOhos start service error.\n");
        return;
    }
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

    void Destroy()
    {
    }

public:
    enum Code {
        DEFAULT = -1,
        ADD_FAULTLOG = 0,
        QUERY_SELF_FAULTLOG,
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
    std::string timeStr = GetFormatedTime(now);
    std::string content = "Pid:101\nUid:0\nProcess name:BootScanUnittest\nReason:unittest for StartBootScan\n"
        "Fault thread info:\nTid:101, Name:BootScanUnittest\n#00 xxxxxxx\n#01 xxxxxxx\n";
    ASSERT_TRUE(FileUtil::SaveStringToFile("/data/log/faultlog/temp/cppcrash-101-" + std::to_string(now), content));
    auto plugin = GetFaultloggerInstance();
    plugin->StartBootScan();
    //check faultlog file content
    std::string fileName = "/data/log/faultlog/faultlogger/cppcrash-BootScanUnittest-0-" + timeStr;
    ASSERT_TRUE(FileUtil::FileExists(fileName));
    ASSERT_GT(FileUtil::GetFileSize(fileName), 0ul);
    ASSERT_EQ(plugin->GetFaultLogInfo(fileName)->module, "BootScanUnittest");

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
    std::string timeStr = GetFormatedTime(now);
    std::string content = "Pid:102\nUid:0\nProcess name:BootScanUnittest\nReason:unittest for StartBootScan\n"
        "Fault thread info:\nTid:102, Name:BootScanUnittest\n";
    std::string fileName = "/data/log/faultlog/temp/cppcrash-102-" + std::to_string(now);
    ASSERT_TRUE(FileUtil::SaveStringToFile(fileName, content));
    auto plugin = GetFaultloggerInstance();
    plugin->StartBootScan();
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
    std::string timeStr = GetFormatedTime(now);
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
    plugin->StartBootScan();

    //check faultlog file content
    std::string fileName = "/data/log/faultlog/faultlogger/cppcrash-BootScanUnittest-0-" + timeStr;
    ASSERT_TRUE(FileUtil::FileExists(fileName));
    ASSERT_GT(FileUtil::GetFileSize(fileName), 0ul);
    auto info = plugin->GetFaultLogInfo(fileName);
    ASSERT_EQ(info->module, "BootScanUnittest");

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
    std::string timeStr = GetFormatedTime(now);
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
    plugin->StartBootScan();
    // check faultlog file content
    std::string fileName = "/data/log/faultlog/faultlogger/cppcrash-BootScanUnittest-0-" + timeStr;
    GTEST_LOG_(INFO) << "========fileName:" << fileName;
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
    GTEST_LOG_(INFO) << "========summaryHasAll========";
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
    GTEST_LOG_(INFO) << "========summaryNotFindSourcemap========";
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
    GTEST_LOG_(INFO) << "========summaryHasNoErrorCode========";
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
    GTEST_LOG_(INFO) << "========summaryHasNoSourceCode========";
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
    GTEST_LOG_(INFO) << "========summaryHasNoErrorCodeAndSourceCode========";
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
    GTEST_LOG_(INFO) << "========summaryHasNoStacktrace========";
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
    GTEST_LOG_(INFO) << "========summaryHasErrorNameAndErrorMessage========";
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
    GTEST_LOG_(INFO) << "========noKeyValue========";
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
    GTEST_LOG_(INFO) << "========noKeyValue========";
    ConstructJsErrorAppEventWithNoValue("", plugin);
    std::string oldFileName = "/data/test_jsError_info";
    ASSERT_TRUE(FileUtil::FileExists(oldFileName));
    auto ret = remove("/data/test_jsError_info");
    if (ret == 0) {
        GTEST_LOG_(INFO) << "remove /data/test_jsError_info failed";
    }
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
 * @tc.name: FaultloggerUnittest001
 * @tc.desc: test GetFaultLogInfo, QuerySelfFaultLog and GetMemoryStrByPid
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, FaultloggerUnittest001, testing::ext::TestSize.Level3)
{
    auto plugin = GetFaultloggerInstance();
    plugin->hasInit_ = false;
    plugin->GetFaultLogInfo("test");
    std::unique_ptr<FaultLogQueryResultInner> obj = plugin->QuerySelfFaultLog(1, 1, 1, 1);
    ASSERT_EQ(obj, nullptr);

    std::string str = plugin->GetMemoryStrByPid(-1);
    ASSERT_EQ(str, "");
    str = plugin->GetMemoryStrByPid(1);
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
    info.reason = "TSAN";
    std::string str = GetFaultLogName(info);
    ASSERT_EQ(str, "tsan-test-0-19700101080000");
    info.reason = "UBSAN";
    str = GetFaultLogName(info);
    ASSERT_EQ(str, "ubsan-test-0-19700101080000");
    info.reason = "GWP-ASAN";
    str = GetFaultLogName(info);
    ASSERT_EQ(str, "gwpasan-test-0-19700101080000");
    info.reason = "ASAN_stack-buffer-overflow";
    str = GetFaultLogName(info);
    ASSERT_EQ(str, "asan-test-0-19700101080000");
    info.reason = "TSANs";
    str = GetFaultLogName(info);
    ASSERT_EQ(str, "sanitizer-test-0-19700101080000");

    str = RegulateModuleNameIfNeed("");
    ASSERT_EQ(str, "");
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

    FaultLogInfoOhos info;
    faultloggerServiceOhos.AddFaultLog(info);
    sptr<IRemoteObject> res = faultloggerServiceOhos.QuerySelfFaultLog(1, 10);
    ASSERT_EQ(res, nullptr);
    faultloggerServiceOhos.Destroy();
}
} // namespace HiviewDFX
} // namespace OHOS
