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
#include <cstddef>
#include <cstdint>
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
#include "faultlogger.h"
#include "faultevent_listener.h"
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
#include "sys_event.h"
#include "sys_event_dao.h"
#include "page_history_manager.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
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

    static void ConstructJsErrorAppEventWithNoValue(std::string summmay, std::shared_ptr<Faultlogger> plugin)
    {
        SysEventCreator sysEventCreator("AAFWK", "JSERROR", SysEventCreator::FAULT);
        sysEventCreator.SetKeyValue("SUMMARY", summmay);
        sysEventCreator.SetKeyValue("name_", "JS_ERROR");
        sysEventCreator.SetKeyValue("happenTime_", 1670248360359); // 1670248360359 : Simulate happenTime_ value
        sysEventCreator.SetKeyValue("TYPE", 3); // 3 : Simulate TYPE value
        sysEventCreator.SetKeyValue("VERSION", "1.0.0");
        sysEventCreator.SetKeyValue("PROCESS_LIFETIME", "1s");

        auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
        std::shared_ptr<Event> event = std::dynamic_pointer_cast<Event>(sysEvent);
        bool result = plugin->OnEvent(event);
        ASSERT_EQ(result, true);
    }
};

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
 * @tc.name: IsInterestedPipelineEvent
 * @tc.desc: Test calling IsInterestedPipelineEvent Func
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, IsInterestedPipelineEvent, testing::ext::TestSize.Level3)
{
    auto testPlugin = GetFaultloggerInstance();
    std::shared_ptr<Event> event = std::make_shared<Event>("test");
    event->SetEventName("PROCESS_EXIT");
    EXPECT_FALSE(testPlugin->IsInterestedPipelineEvent(event));
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
    ASSERT_EQ(ret, 0);

    args.emplace_back(u",");
    ret = servicOhos.Dump(0, args);
    ASSERT_EQ(ret, -1);

    FaultLogInfoOhos info;
    // Cover GetOrSetFaultlogger return nullptr
    servicOhos.AddFaultLog(info);

    const int32_t faultType = -1;
    const int32_t maxNum = 10;
    ASSERT_EQ(servicOhos.QuerySelfFaultLog(faultType, maxNum), nullptr);
    servicOhos.Destroy();
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
 * @tc.name: GetFormatedTimeHHMMSS001
 * @tc.desc: Test format time string
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, GetFormatedTimeHHMMSS001, testing::ext::TestSize.Level3)
{
    uint64_t ts = 1755067031234;
    auto timeStr = PageTraceNode::GetFormatedTimeHHMMSS(ts, true);
    std::string msStr = "14:37:11.234";
    ASSERT_EQ(timeStr, msStr);
}

/**
 * @tc.name: GetFormatedTimeHHMMSS002
 * @tc.desc: Test format time string
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, GetFormatedTimeHHMMSS002, testing::ext::TestSize.Level3)
{
    uint64_t ts = 1755067031;
    auto timeStr = PageTraceNode::GetFormatedTimeHHMMSS(ts, false);
    std::string msStr = "14:37:11";
    ASSERT_EQ(timeStr, msStr);
}

/**
 * @tc.name: GetFormatedTimeHHMMSS003
 * @tc.desc: Test format time string
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, GetFormatedTimeHHMMSS003, testing::ext::TestSize.Level3)
{
    uint64_t ts = 1755067031004;
    auto timeStr = PageTraceNode::GetFormatedTimeHHMMSS(ts, true);
    std::string msStr = "14:37:11.004";
    ASSERT_EQ(timeStr, msStr);
}

/**
 * @tc.name: PageTraceNode001
 * @tc.desc: Test PageTraceNode ToString
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, PageTraceNode001, testing::ext::TestSize.Level3)
{
    PageTraceNode node1(123, 1755067031234, "/test/pageUrl", "testName");
    std::string timeStr1 = "14:37:11.234 /test/pageUrl:testName";
    ASSERT_EQ(node1.ToString(), timeStr1);

    PageTraceNode node2(123, 1755067031234, "/test/pageUrl", "");
    std::string timeStr2 = "14:37:11.234 /test/pageUrl";
    ASSERT_EQ(node2.ToString(), timeStr2);

    PageTraceNode node3(123, 1755067031234, "", "testName");
    std::string timeStr3 = "14:37:11.234 :testName";
    ASSERT_EQ(node3.ToString(), timeStr3);
}

/**
 * @tc.name: PageTrace001
 * @tc.desc: Test PageTrace
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, PageTrace001, testing::ext::TestSize.Level3)
{
    PagesTrace pageTrace;
    pageTrace.AddPageTrace(PageTraceNode(123, 1755067031234, "/test/pageUrl", "testName"));
    ASSERT_EQ(pageTrace.pages_.size(), 1);
    ASSERT_TRUE(pageTrace.ToString(1).empty());
    ASSERT_FALSE(pageTrace.ToString(123).empty());
    std::string timeStr = "  14:37:11.234 /test/pageUrl:testName\n";
    ASSERT_EQ(pageTrace.ToString(123), timeStr);

    pageTrace.AddPageTrace(PageTraceNode(124, 1755067031234, "/test/pageUrl", "testName"));
    ASSERT_EQ(pageTrace.pages_.size(), 2);
    ASSERT_TRUE(pageTrace.ToString(1).empty());
    ASSERT_FALSE(pageTrace.ToString(123).empty());
    ASSERT_EQ(pageTrace.ToString(123), timeStr);

    pageTrace.AddPageTrace(PageTraceNode(123, 1755067031234, "", "enters foreground"));
    ASSERT_EQ(pageTrace.pages_.size(), 3);
    ASSERT_TRUE(pageTrace.ToString(1).empty());
    ASSERT_FALSE(pageTrace.ToString(123).empty());
    timeStr = "  14:37:11.234 :enters foreground\n" + timeStr;
    ASSERT_EQ(pageTrace.ToString(123), timeStr);

    pageTrace.AddPageTrace(PageTraceNode(123, 1755067031234, "", "leaves foreground"));
    ASSERT_EQ(pageTrace.pages_.size(), 4);
    ASSERT_TRUE(pageTrace.ToString(1).empty());
    ASSERT_FALSE(pageTrace.ToString(123).empty());
    timeStr = "  14:37:11.234 :leaves foreground\n" + timeStr;
    ASSERT_EQ(pageTrace.ToString(123), timeStr);

    for (size_t i = 0; i < pageTrace.MAX_PAGES_NUM; i++) {
        pageTrace.AddPageTrace(PageTraceNode(124, 1755067031234, "/test/pageUrl", "testName"));
    }
    ASSERT_EQ(pageTrace.pages_.size(), pageTrace.MAX_PAGES_NUM);
    ASSERT_TRUE(pageTrace.ToString(1).empty());
}

/**
 * @tc.name: PageHistoryManager001
 * @tc.desc: Test PageHistoryManager
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerUnittest, PageHistoryManager001, testing::ext::TestSize.Level3)
{
    auto& manager = PageHistoryManager::GetInstance();
    SysEventCreator sysEventCreator("AAFWK", "ABILITY_ONFOREGROUND", SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("BUNDLE_NAME", "process_1");
    sysEventCreator.SetKeyValue("name_", "ABILITY_ONFOREGROUND");

    auto sysEvent = std::make_shared<SysEvent>("test", nullptr, sysEventCreator);
    sysEvent->pid_ = 1234;
    sysEvent->happenTime_ = 1755067031234;
    manager.HandleEvent(*sysEvent);

    ASSERT_TRUE(manager.GetPageHistory("process_1", 1).empty());
    ASSERT_FALSE(manager.GetPageHistory("process_1", 1234).empty());
    std::string timeStr = "  14:37:11.234 :enters foreground\n";
    ASSERT_EQ(manager.GetPageHistory("process_1", 1234), timeStr);

    sysEvent->eventName_ = "INTERACTION_COMPLETED_LATENCY";
    sysEvent->SetEventValue("SCENE_ID", "ABILITY_OR_PAGE_SWITCH");
    sysEvent->SetEventValue("PAGE_URL", "/test/testPageUrl");
    sysEvent->SetEventValue("PAGE_NAME", "testPageName");
    manager.HandleEvent(*sysEvent);
    ASSERT_TRUE(manager.GetPageHistory("process_1", 1).empty());
    ASSERT_FALSE(manager.GetPageHistory("process_1", 1234).empty());
    timeStr = "  14:37:11.234 /test/testPageUrl:testPageName\n" + timeStr;
    ASSERT_EQ(manager.GetPageHistory("process_1", 1234), timeStr);

    sysEvent->eventName_ = "ABILITY_ONBACKGROUND";
    manager.HandleEvent(*sysEvent);
    ASSERT_TRUE(manager.GetPageHistory("process_1", 1).empty());
    ASSERT_FALSE(manager.GetPageHistory("process_1", 1234).empty());
    timeStr = "  14:37:11.234 :leaves foreground\n" + timeStr;
    ASSERT_EQ(manager.GetPageHistory("process_1", 1234), timeStr);

    for (size_t i = 1; i <= manager.recorder_.MAX_RECORDED_PROCESS_NUM; i++) {
        std::string process = "test_process_" + std::to_string(i);
        sysEvent->SetEventValue("BUNDLE_NAME", process);
        manager.HandleEvent(*sysEvent);
    }
    ASSERT_EQ(manager.recorder_.MAX_RECORDED_PROCESS_NUM, manager.recorder_.pagesList_.size());
    ASSERT_EQ(manager.recorder_.MAX_RECORDED_PROCESS_NUM, manager.recorder_.lruCache_.size());
}
} // namespace HiviewDFX
} // namespace OHOS
