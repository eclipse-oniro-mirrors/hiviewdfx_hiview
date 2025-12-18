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

#include "faultevent_listener.h"
#include "faultlog_database.h"
#include "hisysevent_manager.h"
#include "sys_event.h"
#include "sys_event_dao.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
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
} // namespace HiviewDFX
} // namespace OHOS
