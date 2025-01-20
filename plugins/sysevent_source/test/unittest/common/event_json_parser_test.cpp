/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr char TEST_DEF_FILE_PATH[] = "/data/test/hiview/sys_def_parser/hisysevent.def";
constexpr char INVALID_TEST_DEF_FILE_PATH[] = "/data/test/hiview/sys_def_parser/hisysevent_.def";
constexpr char UPDATED_DEF_FILE_PATH[] = "/data/test/hiview/sys_def_parser/hisysevent_update.def";
constexpr char FIRST_TEST_DOMAIN[] = "FIRST_TEST_DOMAIN";
constexpr char FIRST_TEST_NAME[] = "FIRST_TEST_NAME";
constexpr int FIRST_TEST_EVENT_TYPE = 4;
constexpr char SECOND_TEST_DOMAIN[] = "SECOND_TEST_DOMAIN";
constexpr char SECOND_TEST_NAME[] = "SECOND_TEST_NAME";
constexpr int SECOND_TEST_EVENT_TYPE = 1;
constexpr int PRIVACY_LEVEL_SECRET = 1;
constexpr int PRIVACY_LEVEL_SENSITIVE = 2;
}

void EventJsonParserTest::SetUpTestCase() {}

void EventJsonParserTest::TearDownTestCase() {}

void EventJsonParserTest::SetUp() {}

void EventJsonParserTest::TearDown() {}

/**
 * @tc.name: EventJsonParserTest001
 * @tc.desc: parse a event and check Json info
 * @tc.type: FUNC
 * @tc.require: issueIAKF5E
 */
HWTEST_F(EventJsonParserTest, EventJsonParserTest001, testing::ext::TestSize.Level0)
{
    EventJsonParser jsonParser(INVALID_TEST_DEF_FILE_PATH);
    ASSERT_EQ(jsonParser.GetTagByDomainAndName(FIRST_TEST_DOMAIN, FIRST_TEST_NAME), "");
    ASSERT_EQ(jsonParser.GetTypeByDomainAndName(FIRST_TEST_DOMAIN, FIRST_TEST_NAME), INVALID_EVENT_TYPE);
    ASSERT_EQ(jsonParser.GetPreserveByDomainAndName(FIRST_TEST_DOMAIN, FIRST_TEST_NAME), true);
    auto configBaseInfo = jsonParser.GetDefinedBaseInfoByDomainName(FIRST_TEST_DOMAIN, FIRST_TEST_NAME);
    ASSERT_TRUE(configBaseInfo.preserve);

    jsonParser.ReadDefFile(TEST_DEF_FILE_PATH);
    ASSERT_EQ(jsonParser.GetTagByDomainAndName(FIRST_TEST_DOMAIN, FIRST_TEST_NAME), "FIRST_TEST_CASE");
    ASSERT_EQ(jsonParser.GetTypeByDomainAndName(FIRST_TEST_DOMAIN, FIRST_TEST_NAME), FIRST_TEST_EVENT_TYPE);
    ASSERT_EQ(jsonParser.GetPreserveByDomainAndName(FIRST_TEST_DOMAIN, FIRST_TEST_NAME), false);
    configBaseInfo = jsonParser.GetDefinedBaseInfoByDomainName(FIRST_TEST_DOMAIN, FIRST_TEST_NAME);
    ASSERT_FALSE(configBaseInfo.preserve);
    ASSERT_EQ(configBaseInfo.privacy, PRIVACY_LEVEL_SECRET);

    ASSERT_EQ(jsonParser.GetTagByDomainAndName(SECOND_TEST_DOMAIN, SECOND_TEST_NAME), "SECOND_TEST_CASE");
    ASSERT_EQ(jsonParser.GetTypeByDomainAndName(SECOND_TEST_DOMAIN, SECOND_TEST_NAME), SECOND_TEST_EVENT_TYPE);
    ASSERT_EQ(jsonParser.GetPreserveByDomainAndName(SECOND_TEST_DOMAIN, SECOND_TEST_NAME), true);
    configBaseInfo = jsonParser.GetDefinedBaseInfoByDomainName(SECOND_TEST_DOMAIN, SECOND_TEST_NAME);
    ASSERT_TRUE(configBaseInfo.preserve);
    ASSERT_EQ(configBaseInfo.privacy, DEFAULT_PRIVACY);
}

/**
 * @tc.name: EventJsonParserTest002
 * @tc.desc: hisysevent def file been updated
 * @tc.type: FUNC
 * @tc.require: IBIY90
 */
HWTEST_F(EventJsonParserTest, EventJsonParserTest002, testing::ext::TestSize.Level0)
{
    EventJsonParser jsonParser(TEST_DEF_FILE_PATH);
    BaseInfo configBaseInfo = jsonParser.GetDefinedBaseInfoByDomainName(FIRST_TEST_DOMAIN, FIRST_TEST_NAME);
    ASSERT_EQ(configBaseInfo.privacy, PRIVACY_LEVEL_SECRET);

    jsonParser.OnConfigUpdate(UPDATED_DEF_FILE_PATH);
    BaseInfo firstEventUpdated = jsonParser.GetDefinedBaseInfoByDomainName(FIRST_TEST_DOMAIN, FIRST_TEST_NAME);
    ASSERT_EQ(firstEventUpdated.privacy, PRIVACY_LEVEL_SENSITIVE);
    BaseInfo thirdEventUpdated = jsonParser.GetDefinedBaseInfoByDomainName("THIRD_TEST_DOMAIN", "THIRD_TEST_NAME");
    ASSERT_EQ(thirdEventUpdated.privacy, PRIVACY_LEVEL_SECRET);
}
} // namespace HiviewDFX
} // namespace OHOS
