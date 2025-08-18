/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#include "event_json_parser_test.h"

#include "event_json_parser.h"
#include "file_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr char FIRST_TEST_DOMAIN[] = "FIRST_TEST_DOMAIN";
constexpr char FIRST_TEST_NAME[] = "FIRST_TEST_NAME";
constexpr int FIRST_TEST_EVENT_TYPE = 4;
constexpr char SECOND_TEST_DOMAIN[] = "SECOND_TEST_DOMAIN";
constexpr char SECOND_TEST_NAME[] = "SECOND_TEST_NAME";
constexpr int SECOND_TEST_EVENT_TYPE = 1;
constexpr int PRIVACY_LEVEL_SECRET = 1;
constexpr int PRIVACY_LEVEL_SENSITIVE = 2;
constexpr char THIRD_TEST_NAME[] = "THIRD_TEST_NAME";
constexpr char FORTH_TEST_NAME[] = "FORTH_TEST_NAME";

void RenameDefFile(const std::string& srcFileName, const std::string& destFileName)
{
    std::string dir = "/data/system/hiview/";
    FileUtil::RenameFile(dir + srcFileName, dir + destFileName);
}

void RemoveConfigVerFile()
{
    FileUtil::RemoveFile("/data/system/hiview/unzip_configs/sys_event_def/hiview_config_version");
}

bool IsVectorContain(const std::vector<std::string>& list,
    const std::string& content)
{
    auto ret = std::find(list.begin(), list.end(), content);
    return ret != list.end();
}
}

void EventJsonParserTest::SetUpTestCase() {}

void EventJsonParserTest::TearDownTestCase() {}

void EventJsonParserTest::SetUp() {}

void EventJsonParserTest::TearDown() {}

/**
 * @tc.name: EventJsonParserInitTest001
 * @tc.desc: def file is not initialized
 * @tc.type: FUNC
 */
HWTEST_F(EventJsonParserTest, EventJsonParserInitTest001, testing::ext::TestSize.Level0)
{
    // must be the first test case.
    auto baseInfo = EventJsonParser::GetInstance()->GetDefinedBaseInfoByDomainName(FIRST_TEST_DOMAIN,
        FIRST_TEST_NAME);
    ASSERT_TRUE(baseInfo.level.empty());

    ExportEventList list;
    EventJsonParser::GetInstance()->GetAllCollectEvents(list, 0); // 0 means no filter
    ASSERT_EQ(list.size(), 0);
}

/**
 * @tc.name: EventJsonParserTest001
 * @tc.desc: use default value if event is not configured in hisysevent def file
 * @tc.type: FUNC
 * @tc.require: issueIAKF5E
 */
HWTEST_F(EventJsonParserTest, EventJsonParserTest001, testing::ext::TestSize.Level0)
{
    RenameDefFile("hisysevent_invalid.def", "hisysevent.def");
    RemoveConfigVerFile();
    EventJsonParser::GetInstance()->ReadDefFile(); // init to read def file

    EventJsonParser::GetInstance()->OnConfigUpdate();

    ASSERT_EQ(EventJsonParser::GetInstance()->GetTagByDomainAndName(FIRST_TEST_DOMAIN, FIRST_TEST_NAME), "");
    ASSERT_EQ(EventJsonParser::GetInstance()->GetTypeByDomainAndName(FIRST_TEST_DOMAIN, FIRST_TEST_NAME),
        INVALID_EVENT_TYPE);
    ASSERT_EQ(EventJsonParser::GetInstance()->GetPreserveByDomainAndName(FIRST_TEST_DOMAIN, FIRST_TEST_NAME), true);
    auto configBaseInfo = EventJsonParser::GetInstance()->GetDefinedBaseInfoByDomainName(FIRST_TEST_DOMAIN,
        FIRST_TEST_NAME);
    ASSERT_TRUE(configBaseInfo.keyConfig.preserve);
}

/**
 * @tc.name: EventJsonParserTest002
 * @tc.desc: hisysevent def file been updated
 * @tc.type: FUNC
 * @tc.require: IBIY90
 */
HWTEST_F(EventJsonParserTest, EventJsonParserTest002, testing::ext::TestSize.Level0)
{
    RenameDefFile("hisysevent_normal.def", "hisysevent.def");
    RemoveConfigVerFile();
    EventJsonParser::GetInstance()->ReadDefFile(); // init to read def file

    EventJsonParser::GetInstance()->OnConfigUpdate();

    BaseInfo configBaseInfo = EventJsonParser::GetInstance()->GetDefinedBaseInfoByDomainName(FIRST_TEST_DOMAIN,
        FIRST_TEST_NAME);
    ASSERT_EQ(configBaseInfo.keyConfig.privacy, PRIVACY_LEVEL_SECRET);

    ASSERT_EQ(EventJsonParser::GetInstance()->GetTagByDomainAndName(FIRST_TEST_DOMAIN, FIRST_TEST_NAME),
        "FIRST_TEST_CASE");
    ASSERT_EQ(EventJsonParser::GetInstance()->GetTypeByDomainAndName(FIRST_TEST_DOMAIN, FIRST_TEST_NAME),
        FIRST_TEST_EVENT_TYPE);
    ASSERT_EQ(EventJsonParser::GetInstance()->GetPreserveByDomainAndName(FIRST_TEST_DOMAIN, FIRST_TEST_NAME), false);
    configBaseInfo = EventJsonParser::GetInstance()->GetDefinedBaseInfoByDomainName(FIRST_TEST_DOMAIN,
        FIRST_TEST_NAME);
    ASSERT_FALSE(configBaseInfo.keyConfig.preserve);
    ASSERT_EQ(configBaseInfo.keyConfig.privacy, PRIVACY_LEVEL_SECRET);

    ASSERT_EQ(EventJsonParser::GetInstance()->GetTagByDomainAndName(SECOND_TEST_DOMAIN, SECOND_TEST_NAME),
        "SECOND_TEST_CASE");
    ASSERT_EQ(EventJsonParser::GetInstance()->GetTypeByDomainAndName(SECOND_TEST_DOMAIN, SECOND_TEST_NAME),
        SECOND_TEST_EVENT_TYPE);
    ASSERT_EQ(EventJsonParser::GetInstance()->GetPreserveByDomainAndName(SECOND_TEST_DOMAIN, SECOND_TEST_NAME), true);
    configBaseInfo = EventJsonParser::GetInstance()->GetDefinedBaseInfoByDomainName(SECOND_TEST_DOMAIN,
        SECOND_TEST_NAME);
    ASSERT_TRUE(configBaseInfo.keyConfig.preserve);
    ASSERT_EQ(configBaseInfo.keyConfig.privacy, DEFAULT_PRIVACY);

    // valid domain, invalid name
    configBaseInfo = EventJsonParser::GetInstance()->GetDefinedBaseInfoByDomainName(FIRST_TEST_DOMAIN, "INVALID_NAME");
    ASSERT_TRUE(configBaseInfo.level.empty());

    RenameDefFile("hisysevent_update.def", "hisysevent.def");
    RemoveConfigVerFile();
    EventJsonParser::GetInstance()->OnConfigUpdate();

    BaseInfo firstEventUpdated = EventJsonParser::GetInstance()->GetDefinedBaseInfoByDomainName(FIRST_TEST_DOMAIN,
        FIRST_TEST_NAME);
    ASSERT_EQ(firstEventUpdated.keyConfig.privacy, PRIVACY_LEVEL_SENSITIVE);
    BaseInfo thirdEventUpdated = EventJsonParser::GetInstance()->GetDefinedBaseInfoByDomainName("THIRD_TEST_DOMAIN",
        "THIRD_TEST_NAME");
    ASSERT_EQ(thirdEventUpdated.keyConfig.privacy, PRIVACY_LEVEL_SECRET);
}

/**
 * @tc.name: EventJsonParserTest003
 * @tc.desc: get all export events which is configured to export from def from by calling GetAllCollectEvents
 * @tc.type: FUNC
 * @tc.require: IBIY90
 */
HWTEST_F(EventJsonParserTest, EventJsonParserTest003, testing::ext::TestSize.Level0)
{
    RenameDefFile("hisysevent_with_collect.def", "hisysevent.def");
    RemoveConfigVerFile();
    EventJsonParser::GetInstance()->ReadDefFile(); // init to read def file

    EventJsonParser::GetInstance()->OnConfigUpdate();

    ExportEventList list;
    EventJsonParser::GetInstance()->GetAllCollectEvents(list, 0); // 0 means no filter
    ASSERT_EQ(list.size(), 2); // 2 is the expected length

    auto firstDomainDef = list.find(FIRST_TEST_DOMAIN);
    ASSERT_NE(firstDomainDef, list.end());
    ASSERT_FALSE(IsVectorContain(firstDomainDef->second, FIRST_TEST_NAME));
    ASSERT_TRUE(IsVectorContain(firstDomainDef->second, SECOND_TEST_NAME));
    ASSERT_TRUE(IsVectorContain(firstDomainDef->second, THIRD_TEST_NAME));
    ASSERT_FALSE(IsVectorContain(firstDomainDef->second, FORTH_TEST_NAME));

    auto secondDomainDef = list.find(SECOND_TEST_DOMAIN);
    ASSERT_NE(secondDomainDef, list.end());
    ASSERT_TRUE(IsVectorContain(secondDomainDef->second, FIRST_TEST_NAME));
    ASSERT_TRUE(IsVectorContain(secondDomainDef->second, SECOND_TEST_NAME));
    ASSERT_TRUE(IsVectorContain(secondDomainDef->second, THIRD_TEST_NAME));
    ASSERT_FALSE(IsVectorContain(secondDomainDef->second, FORTH_TEST_NAME));
}
} // namespace HiviewDFX
} // namespace OHOS
