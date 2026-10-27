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

#include "constants.h"
#include "faultevent_listener.h"
#include "faultlog_database.h"
#include "hisysevent_manager.h"
#include "sys_event.h"
#include "sys_event_dao.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
constexpr size_t MAX_PARAM_COUNT = 35;
/**
 * @tc.name: GetFaultInfoListTest001
 * @tc.desc: Test calling GetFaultInfoList Func
 * @tc.type: FUNC
 */
HWTEST(FaultlogDatabaseTest, GetFaultInfoListTest001, testing::ext::TestSize.Level3)
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
    std::list<FaultLogInfo> infoList = FaultLogDatabase::GetFaultInfoList("FaultloggerUnittest", 0, 2, 10);
    ASSERT_GT(infoList.size(), 0);

    FaultLogInfo info;
    bool ret = FaultLogDatabase::ParseFaultLogInfoFromJson(nullptr, info);
    ASSERT_EQ(ret, false);
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
 * @tc.name: SaveFaultLogInfoTest001
 * @tc.desc: Test calling SaveFaultLogInfo Func
 * @tc.type: FUNC
 */
HWTEST(FaultlogDatabaseTest, SaveFaultLogInfoTest001, testing::ext::TestSize.Level3)
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
    FaultLogDatabase::SaveFaultLogInfo(info);
    ASSERT_TRUE(faultEventListener->CheckKeyWords());
}

/**
 * @tc.name: FaultlogDatabaseUnittest001
 * @tc.desc: test RunSanitizerd
 * @tc.type: FUNC
 */
HWTEST(FaultlogDatabaseTest, FaultlogDatabaseUnittest001, testing::ext::TestSize.Level3)
{
    std::list<FaultLogInfo> queryResult = FaultLogDatabase::GetFaultInfoList("com.example.myapplication", 0, -1, 10);
    ASSERT_EQ(queryResult.size(), 0);
    queryResult = FaultLogDatabase::GetFaultInfoList("com.example.myapplication", 0, 8, 10);
    ASSERT_EQ(queryResult.size(), 0);
    queryResult = FaultLogDatabase::GetFaultInfoList("com.example.myapplication", 1, 2, 10);
    ASSERT_EQ(queryResult.size(), 0);
    queryResult = FaultLogDatabase::GetFaultInfoList("com.example.myapplication", 1, 0, 10);
    ASSERT_EQ(queryResult.size(), 0);

    FaultLogInfo info;
    info.faultLogType = FaultLogType::SYS_FREEZE;
    FaultLogDatabase::SaveFaultLogInfo(info);

    bool res = FaultLogDatabase::IsFaultExist(1, 1, -1);
    ASSERT_FALSE(res);
    res = FaultLogDatabase::IsFaultExist(1, 1, 8);
    ASSERT_FALSE(res);
}

/**
 * @tc.name: GetFaultInfoList001
 * @tc.desc: test RunSanitizerd
 * @tc.type: FUNC
 */
HWTEST(FaultlogDatabaseTest, GetFaultInfoList001, testing::ext::TestSize.Level3)
{
    std::list<FaultLogInfo> queryResult = FaultLogDatabase::GetFaultInfoList("com.example.myapplication", 0, -1, 10);
    ASSERT_EQ(queryResult.size(), 0);
    queryResult = FaultLogDatabase::GetFaultInfoList("com.example.myapplication", 0, 8, 10);
    ASSERT_EQ(queryResult.size(), 0);
    queryResult = FaultLogDatabase::GetFaultInfoList("com.example.myapplication", 1, 2, 10);
    ASSERT_EQ(queryResult.size(), 0);
    queryResult = FaultLogDatabase::GetFaultInfoList("com.example.myapplication", 1, 0, 10);
    ASSERT_EQ(queryResult.size(), 0);
}

/**
 * @tc.name: IsFaultExist001
 * @tc.desc: test RunSanitizerd
 * @tc.type: FUNC
 */
HWTEST(FaultlogDatabaseTest, IsFaultExist001, testing::ext::TestSize.Level3)
{
    FaultLogInfo info;
    info.faultLogType = FaultLogType::SYS_FREEZE;
    FaultLogDatabase::SaveFaultLogInfo(info);

    bool res = FaultLogDatabase::IsFaultExist(1, 1, -1);
    ASSERT_FALSE(res);
    res = FaultLogDatabase::IsFaultExist(1, 1, 8);
    ASSERT_FALSE(res);
}

/**
 * @tc.name: SaveSysFreezeStartTimeInfoTest001
 * @tc.desc: Verify sysfreeze HiSysEvent contains raw process and device start times
 * @tc.type: FUNC
 */
HWTEST(FaultlogDatabaseTest, SaveSysFreezeStartTimeInfoTest001, testing::ext::TestSize.Level1)
{
    constexpr int64_t processLifetime = 123456;
    constexpr int64_t deviceRunningTime = 654321;
    StartHisyseventListen("RELIABILITY", "SYS_FREEZE");
    std::vector<std::string> keyWords = {
        "LIFETIME", std::to_string(processLifetime),
        "DEVICE_RUNNING_TIME", std::to_string(deviceRunningTime),
    };
    faultEventListener->SetKeyWords(keyWords);
    FaultLogInfo info;
    info.time = std::time(nullptr);
    info.pid = getpid();
    info.id = getuid();
    info.faultLogType = FaultLogType::SYS_FREEZE;
    info.module = "FaultloggerUnittest";
    info.reason = "unit test";
    info.summary = "unit test";
    info.sectionMap[FaultKey::PROCESS_LIFETIME] = std::to_string(processLifetime);
    info.sectionMap[FaultKey::DEVICE_RUNNING_TIME] = std::to_string(deviceRunningTime);

    FaultLogDatabase::SaveFaultLogInfo(info);

    ASSERT_TRUE(faultEventListener->CheckKeyWords());
}

/**
 * @tc.name: SaveSysFreezeHostResourceWarningTest001
 * @tc.desc: Verify sysfreeze HiSysEvent contains HOST_RESOURCE_WARNING when set to TRUE
 * @tc.type: FUNC
 */
HWTEST(FaultlogDatabaseTest, SaveSysFreezeHostResourceWarningTest001, testing::ext::TestSize.Level1)
{
    StartHisyseventListen("RELIABILITY", "SYS_FREEZE");
    std::vector<std::string> keyWords = {
        "HOST_RESOURCE_WARNING", "TRUE",
    };
    faultEventListener->SetKeyWords(keyWords);
    FaultLogInfo info;
    info.time = std::time(nullptr);
    info.pid = getpid();
    info.id = getuid();
    info.faultLogType = FaultLogType::SYS_FREEZE;
    info.module = "FaultloggerUnittest";
    info.reason = "unit test";
    info.summary = "unit test";
    info.sectionMap[FaultKey::HOST_RESOURCE_WARNING] = "TRUE";

    FaultLogDatabase::SaveFaultLogInfo(info);

    ASSERT_TRUE(faultEventListener->CheckKeyWords());
}

/**
 * @tc.name: FaultLogDatabase::SaveFaultInfoToRawDb
 * @tc.desc: Test calling SaveFaultInfoToRawDb Func
 * @tc.type: FUNC
 */
HWTEST(FaultlogDatabaseTest, FaultLogManagerTest001, testing::ext::TestSize.Level0)
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
    FaultLogDatabase::SaveFaultLogInfo(info);
    ASSERT_TRUE(faultEventListener->CheckKeyWords());
}

/**
 * @tc.name: faultLogManager GetFaultInfoListTest001
 * @tc.desc: Test calling faultLogManager.GetFaultInfoList Func
 * @tc.type: FUNC
 */
HWTEST(FaultlogDatabaseTest, FaultLogManagerTest002, testing::ext::TestSize.Level3)
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

    auto list = FaultLogDatabase::GetFaultInfoList("FaultloggerUnittest", 0, 2, 10);
    ASSERT_GT(list.size(), 0);

    auto isProcessedFault2 = FaultLogDatabase::IsFaultExist(1854, 0, 2);
    ASSERT_EQ(isProcessedFault2, true);

    auto isProcessedFault3 = FaultLogDatabase::IsFaultExist(1855, 0, 2);
    ASSERT_EQ(isProcessedFault3, false);

    auto isProcessedFault4 = FaultLogDatabase::IsFaultExist(1855, 5, 2);
    ASSERT_EQ(isProcessedFault4, false);
}

/**
 * @tc.name: GetAppFreezeExtInfoFromFileName001
 * @tc.desc: Test calling GetAppFreezeExtInfoFromFileName Func
 * @tc.type: FUNC
 */
HWTEST(FaultlogDatabaseTest, GetAppFreezeExtInfoFromFileName001, testing::ext::TestSize.Level3)
{
    std::string jsonStr = R"~({"domain_":"RELIABILITY", "name_":"APP_FREEZE", "type_":1, "time_":1770875821914,
        "tz_":"+0800", "pid_":1854, "tid_":1854, "uid_":0, "FAULT_TYPE":"2", "PID":1854, "UID":20010039,
        "MODULE":"FaultloggerUnittest", "REASON":"unittest for SaveFaultLogInfo",
        "SUMMARY":"summary for SaveFaultLogInfo",
        "LOG_PATH":"/data/log/faultlog/faultlogger/appfreeze-com.example.jsinject-20010039-20260212135701914.log",
        "VERSION":"", "HAPPEN_TIME":1770875821914,
        "PNAME":"/", "FIRST_FRAME":"/", "SECOND_FRAME":"/", "LAST_FRAME":"/",
        "FINGERPRINT":"04c0d6f03c73da531f00eb112479a8a2f19f59fafba6a474dcbe455a13288f4d",
        "FREEZE_INFO_PATH":"/data/log/faultlog/freeze_ext/freeze-cpuinfo-ext-appfreeze-com.example.jsinject",
        "level_":"CRITICAL", "tag_":"STABILITY", "id_":"17165544771317691984", "info_":""})~";
    auto sysEvent = std::make_shared<SysEvent>("SysEventSource", nullptr, jsonStr);
    sysEvent->SetLevel("MINOR");
    sysEvent->SetEventSeq(449); // 449: test seq
    EventStore::SysEventDao::Insert(sysEvent);

    std::string filename1 = "appfreeze-com.example.jsinject-20010039-20260212135701914.log";
    auto extPath1 = FaultLogDatabase::GetAppFreezeExtInfoFromFileName(filename1);
    ASSERT_EQ(extPath1, "/data/log/faultlog/freeze_ext/freeze-cpuinfo-ext-appfreeze-com.example.jsinject");

    std::string filename2 = "appfreeze-com.example.jsinject-20010039-20260212135706914.log";
    auto extPath2 = FaultLogDatabase::GetAppFreezeExtInfoFromFileName(filename2);
    ASSERT_EQ(extPath2, "/data/log/faultlog/freeze_ext/freeze-cpuinfo-ext-appfreeze-com.example.jsinject");

    std::string filename3 = "appfreeze-com.example.jsinject-20010039-20260212135713914.log";
    auto extPath3 = FaultLogDatabase::GetAppFreezeExtInfoFromFileName(filename3);
    ASSERT_EQ(extPath3, "");
}

static HiSysEventParam* FindParamByName(HiSysEventParam* params, size_t count, const std::string& name)
{
    for (size_t i = 0; i < count; ++i) {
        if (std::string(params[i].name) == name) {
            return &params[i];
        }
    }
    return nullptr;
}

/**
 * @tc.name: GetInt64Value001
 * @tc.desc: Test GetInt64Value with a valid numeric string
 * @tc.type: FUNC
 */
HWTEST(FaultlogDatabaseTest, GetInt64Value001, testing::ext::TestSize.Level1)
{
    FaultLogInfo info;
    info.sectionMap[FaultKey::DEVICE_RUNNING_TIME] = "123456";
    EXPECT_EQ(FaultLogDatabase::GetInt64Value(info, FaultKey::DEVICE_RUNNING_TIME), 123456);
}

/**
 * @tc.name: GetInt64Value002
 * @tc.desc: Test GetInt64Value when the key does not exist
 * @tc.type: FUNC
 */
HWTEST(FaultlogDatabaseTest, GetInt64Value002, testing::ext::TestSize.Level2)
{
    FaultLogInfo info;
    EXPECT_EQ(FaultLogDatabase::GetInt64Value(info, FaultKey::DEVICE_RUNNING_TIME), 0);
}

/**
 * @tc.name: GetInt64Value003
 * @tc.desc: Test GetInt64Value with a non-numeric string
 * @tc.type: FUNC
 */
HWTEST(FaultlogDatabaseTest, GetInt64Value003, testing::ext::TestSize.Level2)
{
    FaultLogInfo info;
    info.sectionMap[FaultKey::DEVICE_RUNNING_TIME] = "invalid";
    EXPECT_EQ(FaultLogDatabase::GetInt64Value(info, FaultKey::DEVICE_RUNNING_TIME), 0);
}

/**
 * @tc.name: GetInt64Value004
 * @tc.desc: Test GetInt64Value with a negative value
 * @tc.type: FUNC
 */
HWTEST(FaultlogDatabaseTest, GetInt64Value004, testing::ext::TestSize.Level2)
{
    FaultLogInfo info;
    info.sectionMap[FaultKey::DEVICE_RUNNING_TIME] = "-999999";
    EXPECT_EQ(FaultLogDatabase::GetInt64Value(info, FaultKey::DEVICE_RUNNING_TIME), -999999);
}

/**
 * @tc.name: BuildSysEventParams001
 * @tc.desc: Test BuildSysEventParams for CPP_CRASH drops sysfreeze-only params
 * @tc.type: FUNC
 */
HWTEST(FaultlogDatabaseTest, BuildSysEventParams001, testing::ext::TestSize.Level1)
{
    FaultLogInfo info;
    info.faultLogType = FaultLogType::CPP_CRASH;
    info.pid = 1854;
    info.id = 0;
    info.module = "FaultloggerUnittest";
    info.reason = "SIGSEGV";
    info.summary = "summary for test";
    auto faultLogType = std::to_string(info.faultLogType);
    HiSysEventParam params[MAX_PARAM_COUNT];
    size_t paramCount = FaultLogDatabase::BuildSysEventParams(info, faultLogType, params);
    EXPECT_EQ(paramCount, static_cast<size_t>(31));
    auto* faultTypeParam = FindParamByName(params, paramCount, "FAULT_TYPE");
    ASSERT_NE(faultTypeParam, nullptr);
    EXPECT_EQ(std::string(faultTypeParam->v.s), faultLogType);
    auto* pidParam = FindParamByName(params, paramCount, "PID");
    ASSERT_NE(pidParam, nullptr);
    EXPECT_EQ(pidParam->v.i32, info.pid);
    EXPECT_EQ(FindParamByName(params, paramCount, "DEVICE_RUNNING_TIME"), nullptr);
    EXPECT_EQ(FindParamByName(params, paramCount, "HOST_RESOURCE_WARNING"), nullptr);
}

/**
 * @tc.name: BuildSysEventParams002
 * @tc.desc: Test BuildSysEventParams for SYS_FREEZE keeps start time params
 * @tc.type: FUNC
 */
HWTEST(FaultlogDatabaseTest, BuildSysEventParams002, testing::ext::TestSize.Level1)
{
    FaultLogInfo info;
    info.faultLogType = FaultLogType::SYS_FREEZE;
    info.sectionMap[FaultKey::PROCESS_LIFETIME] = "123456";
    info.sectionMap[FaultKey::DEVICE_RUNNING_TIME] = "654321";
    info.sectionMap[FaultKey::HOST_RESOURCE_WARNING] = "TRUE";
    auto faultLogType = std::to_string(info.faultLogType);
    HiSysEventParam params[MAX_PARAM_COUNT];
    size_t paramCount = FaultLogDatabase::BuildSysEventParams(info, faultLogType, params);
    EXPECT_EQ(paramCount, static_cast<size_t>(33));
    auto* lifeTimeParam = FindParamByName(params, paramCount, "LIFETIME");
    ASSERT_NE(lifeTimeParam, nullptr);
    EXPECT_EQ(lifeTimeParam->v.i64, 123456);
    auto* deviceRunningParam = FindParamByName(params, paramCount, "DEVICE_RUNNING_TIME");
    ASSERT_NE(deviceRunningParam, nullptr);
    EXPECT_EQ(deviceRunningParam->v.i64, 654321);
    auto* hostResourceWarningParam = FindParamByName(params, paramCount, "HOST_RESOURCE_WARNING");
    ASSERT_NE(hostResourceWarningParam, nullptr);
    EXPECT_EQ(std::string(hostResourceWarningParam->v.s), "TRUE");
}

/**
 * @tc.name: BuildSysEventParams003
 * @tc.desc: Test FG param value is 3 for cppcrash with SIGABRT and LastFatalMessage
 * @tc.type: FUNC
 */
HWTEST(FaultlogDatabaseTest, BuildSysEventParams003, testing::ext::TestSize.Level1)
{
    FaultLogInfo info;
    info.faultLogType = FaultLogType::CPP_CRASH;
    info.reason = "SIGABRT";
    info.summary = "LastFatalMessage: crash detail";
    info.sectionMap[FaultKey::IS_SIG_ACTION] = "Yes";
    auto faultLogType = std::to_string(info.faultLogType);
    HiSysEventParam params[MAX_PARAM_COUNT];
    size_t paramCount = FaultLogDatabase::BuildSysEventParams(info, faultLogType, params);
    auto* fgParam = FindParamByName(params, paramCount, "FG");
    ASSERT_NE(fgParam, nullptr);
    // IS_SIG_ACTION=Yes contributes 1, SIGABRT + LastFatalMessage contributes 2
    EXPECT_EQ(fgParam->v.i32, 3);
}

/**
 * @tc.name: BuildSysEventParams004
 * @tc.desc: Test FG param value is 1 for non-cppcrash with IS_SIG_ACTION=Yes
 * @tc.type: FUNC
 */
HWTEST(FaultlogDatabaseTest, BuildSysEventParams004, testing::ext::TestSize.Level2)
{
    FaultLogInfo info;
    info.faultLogType = FaultLogType::APP_FREEZE;
    info.reason = "WATCH";
    info.summary = "freeze summary";
    info.sectionMap[FaultKey::IS_SIG_ACTION] = "Yes";
    auto faultLogType = std::to_string(info.faultLogType);
    HiSysEventParam params[MAX_PARAM_COUNT];
    size_t paramCount = FaultLogDatabase::BuildSysEventParams(info, faultLogType, params);
    auto* fgParam = FindParamByName(params, paramCount, "FG");
    ASSERT_NE(fgParam, nullptr);
    // not CPP_CRASH, so only IS_SIG_ACTION contributes
    EXPECT_EQ(fgParam->v.i32, 1);
}
} // namespace HiviewDFX
} // namespace OHOS
