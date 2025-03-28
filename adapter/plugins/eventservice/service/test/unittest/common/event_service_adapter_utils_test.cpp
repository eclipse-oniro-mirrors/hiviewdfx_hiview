/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <codecvt>
#include <locale>
#include <string>

#include "ash_mem_utils.h"
#include "compliant_event_checker.h"
#include "file_util.h"
#include "hiview_global.h"
#include "parameter_ex.h"
#include "running_status_log_util.h"
#include "string_ex.h"
#include "sys_event_rule.h"
#include "sys_event_service_adapter.h"
#include "sys_event_service_ohos.h"

namespace OHOS::HiviewDFX {
namespace EventStore {
using namespace testing::ext;
namespace {
constexpr char TEST_LOG_DIR[] = "/data/test/EventServiceAdapterUtilsTestDir/";
constexpr char TETS_ASH_MEM_NAME[] = "TestSharedMemory";
constexpr int32_t TETS_ASH_MEM_SIZE = 1024 * 2; // 2K
constexpr int64_t TEST_SECURE_ENABALED_VAL = 1;

sptr<Ashmem> GetAshmem()
{
    auto ashmem = Ashmem::CreateAshmem(TETS_ASH_MEM_NAME, TETS_ASH_MEM_SIZE);
    if (ashmem == nullptr) {
        return nullptr;
    }
    if (!ashmem->MapReadAndWriteAshmem()) {
        return ashmem;
    }
    return ashmem;
}
}

class HiviewTestContext : public HiviewContext {
public:
    std::string GetHiViewDirectory(DirectoryType type __UNUSED)
    {
        return TEST_LOG_DIR;
    }
};

std::string GetDestDirWithSuffix(const std::string& dirSuffix)
{
    std::string workPath = HiviewGlobal::GetInstance()->GetHiViewDirectory(
        HiviewContext::DirectoryType::CONFIG_DIRECTORY);
    if (workPath.back() != '/') {
        workPath = workPath + "/";
    }
    workPath.append("/").append(dirSuffix);
    if (!FileUtil::FileExists(workPath)) {
        FileUtil::ForceCreateDirectory(workPath, FileUtil::FILE_PERM_770);
    }
    return workPath;
}

class EventServiceAdapterUtilsTest : public testing::Test {
public:
static void SetUpTestCase();
static void TearDownTestCase();
void SetUp();
void TearDown();
};

void EventServiceAdapterUtilsTest::SetUpTestCase()
{
}

void EventServiceAdapterUtilsTest::TearDownTestCase()
{
}

void EventServiceAdapterUtilsTest::SetUp()
{
}

void EventServiceAdapterUtilsTest::TearDown()
{
}

/**
 * @tc.name: EventServiceAdapterUtilsTest001
 * @tc.desc: test WriteBulkData of AshMemUtils
 * @tc.type: FUNC
 * @tc.require: issueIBT9BB
 */
HWTEST_F(EventServiceAdapterUtilsTest, EventServiceAdapterUtilsTest001, TestSize.Level1)
{
    MessageParcel msgParcel;
    std::vector<std::u16string> emptyRes;
    ASSERT_NE(AshMemUtils::WriteBulkData(msgParcel, emptyRes), nullptr);
    std::vector<std::u16string> normalRes = {
        Str8ToStr16(std::string("AshMemUtilsTest001")),
        Str8ToStr16(std::string("AshMemUtilsTest001")),
    };
    ASSERT_NE(AshMemUtils::WriteBulkData(msgParcel, normalRes), nullptr);
}

/**
 * @tc.name: EventServiceAdapterUtilsTest002
 * @tc.desc: test ReadBulkData & CloseAshmem of AshMemUtils
 * @tc.type: FUNC
 * @tc.require: issueIBT9BB
 */
HWTEST_F(EventServiceAdapterUtilsTest, EventServiceAdapterUtilsTest002, TestSize.Level1)
{
    MessageParcel msgParcel;
    std::vector<std::u16string> src = {
        Str8ToStr16(std::string("AshMemUtilsTest002")),
        Str8ToStr16(std::string("AshMemUtilsTest002")),
    };
    ASSERT_NE(AshMemUtils::WriteBulkData(msgParcel, src), nullptr);

    std::vector<std::u16string> dest;
    ASSERT_TRUE(AshMemUtils::ReadBulkData(msgParcel, dest));
    ASSERT_TRUE(src.size() == dest.size());
    ASSERT_TRUE(Str16ToStr8(dest[0]) == "AshMemUtilsTest002" && Str16ToStr8(dest[1]) == "AshMemUtilsTest002");
    AshMemUtils::CloseAshmem(nullptr);
    AshMemUtils::CloseAshmem(GetAshmem());
}

/**
 * @tc.name: EventServiceAdapterUtilsTest003
 * @tc.desc: test IsCompliantEvent of CompliantEventChecker
 * @tc.type: FUNC
 * @tc.require: issueIBT9BB
 */
HWTEST_F(EventServiceAdapterUtilsTest, EventServiceAdapterUtilsTest003, TestSize.Level1)
{
    bool enabled = (Parameter::GetInteger("const.secure", TEST_SECURE_ENABALED_VAL) == TEST_SECURE_ENABALED_VAL);
    CompliantEventChecker compliantEventChecker;
    ASSERT_TRUE(compliantEventChecker.IsCompliantEvent("AAFWK", "ABILITY_ONACTIVE"));
    ASSERT_TRUE(compliantEventChecker.IsCompliantEvent("ACE", "INTERACTION_COMPLETED_LATENCY"));
    ASSERT_TRUE(compliantEventChecker.IsCompliantEvent("AV_CODEC", "CODEC_START_INFO"));
    ASSERT_TRUE(compliantEventChecker.IsCompliantEvent("GRAPHIC", "INTERACTION_COMPLETED_LATENCY"));
    ASSERT_TRUE(compliantEventChecker.IsCompliantEvent("LOCATION", "GNSS_STATE"));
    ASSERT_TRUE(compliantEventChecker.IsCompliantEvent("PERFORMANCE", "ANY_EVENT"));
    ASSERT_TRUE(compliantEventChecker.IsCompliantEvent("RELIABILITY", "ANY_EVENT"));
    ASSERT_TRUE(compliantEventChecker.IsCompliantEvent("SANITIZER", "ADDR_SANITIZER"));
    ASSERT_TRUE(compliantEventChecker.IsCompliantEvent("WORKSCHEDULER", "WORK_ADD"));
    if (enabled) {
        ASSERT_FALSE(compliantEventChecker.IsCompliantEvent("AAFWK", "ANY_EVENT"));
        ASSERT_FALSE(compliantEventChecker.IsCompliantEvent("ACE", "ANY_EVENT"));
        ASSERT_FALSE(compliantEventChecker.IsCompliantEvent("AV_CODEC", "ANY_EVENT"));
        ASSERT_FALSE(compliantEventChecker.IsCompliantEvent("GRAPHIC", "ANY_EVENT"));
        ASSERT_FALSE(compliantEventChecker.IsCompliantEvent("LOCATION", "ANY_EVENT"));
        ASSERT_FALSE(compliantEventChecker.IsCompliantEvent("SANITIZER", "ANY_EVENT"));
        ASSERT_FALSE(compliantEventChecker.IsCompliantEvent("WORKSCHEDULER", "WORK_ADD"));
    } else {
        ASSERT_TRUE(compliantEventChecker.IsCompliantEvent("AAFWK", "ANY_EVENT"));
        ASSERT_TRUE(compliantEventChecker.IsCompliantEvent("ACE", "ANY_EVENT"));
        ASSERT_TRUE(compliantEventChecker.IsCompliantEvent("AV_CODEC", "ANY_EVENT"));
        ASSERT_TRUE(compliantEventChecker.IsCompliantEvent("GRAPHIC", "ANY_EVENT"));
        ASSERT_TRUE(compliantEventChecker.IsCompliantEvent("LOCATION", "ANY_EVENT"));
        ASSERT_TRUE(compliantEventChecker.IsCompliantEvent("SANITIZER", "ANY_EVENT"));
        ASSERT_TRUE(compliantEventChecker.IsCompliantEvent("WORKSCHEDULER", "WORK_ADD"));
    }
}

/**
 * @tc.name: RunningStatusLogUtilTest
 * @tc.desc: Test apis of RunningStatusLogUtil
 * @tc.type: FUNC
 * @tc.require: issueI62WJT
 */
HWTEST_F(EventServiceAdapterUtilsTest, RunningStatusLogUtilTest, testing::ext::TestSize.Level1)
{
    HiviewTestContext hiviewTestContext;
    HiviewGlobal::CreateInstance(hiviewTestContext);

    std::vector<OHOS::HiviewDFX::SysEventQueryRule> queryRules;
    std::vector<std::string> eventNames { "EVENT_NAME1", "EVENT_NAME2" };
    OHOS::HiviewDFX::SysEventQueryRule queryRule("DOMAIN", eventNames);
    RunningStatusLogUtil::LogTooManyQueryRules(queryRules);
    queryRules.emplace_back(queryRule);
    RunningStatusLogUtil::LogTooManyQueryRules(queryRules);

    vector<SysEventRule> sysEventRules;
    RunningStatusLogUtil::LogTooManyWatchRules(sysEventRules);
    sysEventRules.emplace_back("", "");
    RunningStatusLogUtil::LogTooManyWatchRules(sysEventRules);
    sysEventRules.emplace_back("TETS_TAG");
    RunningStatusLogUtil::LogTooManyWatchRules(sysEventRules);

    RunningStatusLogUtil::LogTooManyWatchers(30); // 30 is a test value

    RunningStatusLogUtil::LogTooManyEvents(1000); // 1000 is a test value
    std::vector<std::string> files;
    FileUtil::GetDirFiles(GetDestDirWithSuffix("sys_event_log"), files);
    ASSERT_GE(files.size(), 0);
}

/**
 * @tc.name: SysEventServiceAdapterTest001
 * @tc.desc: Test apis of SysEventServiceAdapterTest001
 * @tc.type: FUNC
 * @tc.require: issueIBT9BB
 */
HWTEST_F(EventServiceAdapterUtilsTest, SysEventServiceAdapterTest001, testing::ext::TestSize.Level1)
{
    SysEventServiceAdapter::StartService(nullptr);
    std::shared_ptr<SysEvent> event = nullptr;
    SysEventServiceAdapter::OnSysEvent(event);
    ASSERT_NE(OHOS::HiviewDFX::SysEventServiceOhos::GetInstance(), nullptr);
}
}
}