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

#include "event_export_write_test.h"

#include <memory>

#include "cached_event_packager.h"
#include "event_export_util.h"
#include "event_write_strategy_factory.h"
#include "export_file_writer.h"
#include "export_json_file_builder.h"
#include "file_util.h"
#include "parameter.h"
#include "parameter_ex.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
const std::string MODULE_NAME = "TEST_MODULE";
const std::string EXPORT_DIR = "/data/test/";
const std::string TEST_DOMAIN = "TEST_DOMAIN";
const std::string TEST_EVENT_NAME = "TEST_NAME";
const EventVersion EVENT_VER {
    "SV0.0.1",
    "PV0.0.2",
};
constexpr int64_t MAX_FILE_SIZE = 2000000;
constexpr int32_t UID = 100;

std::string BuildEventStr()
{
    return R"~({"domain_":"TEST_DOMAIN","name_":"TEST_NAME","type_":4,"time_":1746415896989,"tz_":"+0000",
        "pid_":1,"tid_":1,"uid_":0,"log_":0,"id_":"17708195254699639651","PARAM1":"teecd","PARAM2":2317,
        "period_seq_":"","level_":"CRITICAL","seq_":76258})~";
}

std::shared_ptr<CachedEvent> BuildCacheEvent()
{
    std::string eventStr = BuildEventStr();
    return std::make_shared<CachedEvent>(EVENT_VER, TEST_DOMAIN, TEST_EVENT_NAME, eventStr, UID);
}

WriteStrategyParam BuildWriteStrategyParam()
{
    return WriteStrategyParam {
        MODULE_NAME,
        EXPORT_DIR,
        EVENT_VER,
        UID,
    };
}

void BuildCachedEventMap(CachedEventMap& cachedEventMap)
{
    cachedEventMap.emplace(TEST_DOMAIN, std::vector<std::pair<std::string, std::string>> {
        std::make_pair(TEST_EVENT_NAME, BuildEventStr()),
    });
}

void AssertWroteFiles(const std::string& srcFile, const std::string& destFile)
{
    ASSERT_NE(srcFile.find("U100"), std::string::npos);
    ASSERT_NE(destFile.find("U100"), std::string::npos);
    ASSERT_EQ(FileUtil::ExtractFileName(srcFile).size(), FileUtil::ExtractFileName(destFile).size());
}

std::string GenerateDevId()
{
    constexpr int32_t idLen = 65;
    char id[idLen] = {0};
    if (GetDevUdid(id, idLen) == 0) {
        return std::string(id);
    }
    return "";
}
}

void EventExportWriteTest::SetUpTestCase()
{
}

void EventExportWriteTest::TearDownTestCase()
{
}

void EventExportWriteTest::SetUp()
{
}

void EventExportWriteTest::TearDown()
{
}

/**
 * @tc.name: EventExportWriteTest001
 * @tc.desc: Test apis of CachedEventPackager
 * @tc.type: FUNC
 * @tc.require: issueIC607P
 */
HWTEST_F(EventExportWriteTest, EventExportWriteTest001, testing::ext::TestSize.Level3)
{
    CachedEventPackager packager(MODULE_NAME, EXPORT_DIR, EVENT_VER, UID, MAX_FILE_SIZE);
    std::string eventStr = BuildEventStr();
    ASSERT_TRUE(packager.AppendCachedEvent(TEST_DOMAIN, TEST_EVENT_NAME, eventStr));
    packager.ClearPackagedFileCache();
    ASSERT_TRUE(packager.AppendCachedEvent(TEST_DOMAIN, TEST_EVENT_NAME, eventStr));
    ASSERT_TRUE(packager.Package());
    packager.HandlePackagedFileCache();
    ASSERT_TRUE(packager.Package());
}

/**
 * @tc.name: EventExportWriteTest002
 * @tc.desc: Test apis of EventWriteStrategyFactory & WriteZipFileStrategy
 * @tc.type: FUNC
 * @tc.require: issueIC607P
 */
HWTEST_F(EventExportWriteTest, EventExportWriteTest002, testing::ext::TestSize.Level3)
{
    auto strategy = EventWriteStrategyFactory::GetWriteStrategy(StrategyType::ZIP_FILE);
    ASSERT_NE(strategy, nullptr);
    ASSERT_EQ(strategy->GetPackagerKey(BuildCacheEvent()), "SV0.0.1_PV0.0.2_100");
    auto strategyParam = BuildWriteStrategyParam();
    auto eventStr = BuildEventStr();
    ASSERT_TRUE(strategy->HandleWroteResult(strategyParam, eventStr, [this] (const std::string& srcPath,
        const std::string& destPath) {
        AssertWroteFiles(srcPath, destPath);
    }));
}

/**
 * @tc.name: EventExportWriteTest003
 * @tc.desc: Test apis of ExportJsonFileBuilder
 * @tc.type: FUNC
 * @tc.require: issueIC607P
 */
HWTEST_F(EventExportWriteTest, EventExportWriteTest003, testing::ext::TestSize.Level3)
{
    std::shared_ptr<ExportFileBaseBuilder> builder = std::make_shared<ExportJsonFileBuilder>(EVENT_VER);
    ASSERT_NE(builder, nullptr);
    CachedEventMap cachedEventMap;
    BuildCachedEventMap(cachedEventMap);
    std::string buildStr;
    ASSERT_TRUE(builder->Build(cachedEventMap, buildStr));
    ASSERT_GT(buildStr.size(), 0);
}


/**
 * @tc.name: EventExportWriteTest004
 * @tc.desc: Test apis of ExportFileWriter
 * @tc.type: FUNC
 * @tc.require: issueIC607P
 */
HWTEST_F(EventExportWriteTest, EventExportWriteTest004, testing::ext::TestSize.Level3)
{
    ExportFileWriter fileWriter;
    fileWriter.SetExportFileWroteListener([this] (const std::string& srcPath,
        const std::string& destPath) {
        AssertWroteFiles(srcPath, destPath);
    });
    CachedEventMap cachedEventMap;
    BuildCachedEventMap(cachedEventMap);
    auto strategyParam = BuildWriteStrategyParam();
    ASSERT_FALSE(fileWriter.Write(nullptr, cachedEventMap, strategyParam));
    ASSERT_TRUE(fileWriter.Write(std::make_shared<ExportJsonFileBuilder>(EVENT_VER), cachedEventMap,
        strategyParam));
}

/**
 * @tc.name: EventExportWriteTest005
 * @tc.desc: Test apis of EventExportUtil
 * @tc.type: FUNC
 * @tc.require: issueIC607P
 */
HWTEST_F(EventExportWriteTest, EventExportWriteTest005, testing::ext::TestSize.Level3)
{
    auto deviceId = EventExportUtil::GetDeviceId();
    if (Parameter::GetUserType() == Parameter::USER_TYPE_OVERSEA_COMMERCIAL) {
        ASSERT_EQ(deviceId, Parameter::GetString("persist.hiviewdfx.priv.packid", ""));
    } else {
        ASSERT_EQ(deviceId, GenerateDevId());
    }
}
} // namespace HiviewDFX
} // namespace OHOS