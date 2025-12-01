/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "dispatch_rule_parser_test.h"

#include "dispatch_rule_parser.h"
#include "file_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
    const std::string TEST_FILE_DIR = "/data/test/dispatch_rule_parser_test/";
    constexpr size_t TYPE_SIZE = 1;
    constexpr size_t TAG_SIZE = 1;
    constexpr size_t EVENT_SIZE = 2;
    const std::string TEST_JSON = "{\
    \"domains\":[\
    {\
        \"domain\":\"WINDOWMANAGER\",\
        \"include\":[\"ANIMATION_CALLBACK_TIMEOUT\"]\
    },\
    {\
        \"domain\": \"COMMON\",\
        \"exclude\": [\"COMMON1\"]\
    }\
    ],\
    \"events\":[\"PANIC\", \"HWWATCHDOG\"],\
    \"types\":[\"FAULT\"],\
    \"tags\":[\"PowerStatus\"]\
    }";
    const std::string TEST_JSON_NON = "{\
        \"non\":[\"type\", \"value\"]\
    }";

    const std::string TEST_JSON_ERROR_TYPE_KEY = "{\
    \"domains\": \"domain\",\
    \"events\":\"PANIC\",\
    \"types\":\"PANIC\",\
    \"tags\":\"PANIC\"\
}";

    const std::string TEST_JSON_ERROR_TYPE_VALUE = "{\
    \"domains\": [\
    {\
        \"domain\": 1,\
        \"include\": [\"ANIMATION_CALLBACK_TIMEOUT\"]\
    },\
    {\
        \"domain\": 1,\
        \"exclude\": 1\
    }\
    ],\
    \"events\":[1, 2],\
    \"types\":[1],\
    \"tags\":[1]\
    }";

    const std::string TEST_JSON_ALL_EVENTS_OF_DOMAIN = R"~({"domains":[{"domain":"HIVIEWDFX"}]})~";
    const std::string TEST_JSON_INVALID_DOMAIN = R"~({"domains":[{"domain":"HIVIEWDFX", "test":"invalid"}]})~";

    const std::string TEST_JSON_NOJSON = "not json";
}
void DispatchRuleParserTest::SetUpTestCase()
{
    if (!FileUtil::FileExists(TEST_FILE_DIR)) {
        FileUtil::ForceCreateDirectory(TEST_FILE_DIR, FileUtil::FILE_PERM_770);
    }
    FileUtil::SaveStringToFile(TEST_FILE_DIR + "test_json", TEST_JSON, true);
    FileUtil::SaveStringToFile(TEST_FILE_DIR + "test_json_non", TEST_JSON_NON, true);
    FileUtil::SaveStringToFile(TEST_FILE_DIR + "test_json_nojson", TEST_JSON_NOJSON, true);
    FileUtil::SaveStringToFile(TEST_FILE_DIR + "test_json_error_type_key", TEST_JSON_ERROR_TYPE_KEY, true);
    FileUtil::SaveStringToFile(TEST_FILE_DIR + "test_json_error_type_value", TEST_JSON_ERROR_TYPE_VALUE, true);
    (void)FileUtil::SaveStringToFile(TEST_FILE_DIR + "test_json_all_event", TEST_JSON_ALL_EVENTS_OF_DOMAIN, true);
    (void)FileUtil::SaveStringToFile(TEST_FILE_DIR + "test_json_invalid_domain", TEST_JSON_INVALID_DOMAIN, true);
}

void DispatchRuleParserTest::TearDownTestCase()
{
}

void DispatchRuleParserTest::SetUp()
{
}

void DispatchRuleParserTest::TearDown()
{
}

/**
 * @tc.name: DispatchRuleParser001
 * @tc.desc: Test the api of DispatchRuleParser.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DispatchRuleParserTest, DispatchRuleParser001, testing::ext::TestSize.Level0)
{
    DispatchRuleParser ruleParser(TEST_FILE_DIR + "test_json");
    auto rules = ruleParser.GetRule();
    ASSERT_NE(rules, nullptr);
    ASSERT_EQ(rules->typeList.size(), TYPE_SIZE);
    ASSERT_EQ(rules->tagList.size(), TAG_SIZE);
    ASSERT_EQ(rules->eventList.size(), EVENT_SIZE);
    ASSERT_EQ(rules->FindEvent("WINDOWMANAGER", "ANIMATION_CALLBACK_TIMEOUT"), true);
    ASSERT_EQ(rules->FindEvent("COMMON", "COMMON1"), false);
}

/**
 * @tc.name: DispatchRuleParser002
 * @tc.desc: Test the api of DispatchRuleParser for empty json.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DispatchRuleParserTest, DispatchRuleParser002, testing::ext::TestSize.Level0)
{
    DispatchRuleParser ruleParser(TEST_FILE_DIR + "test_json_non");
    auto rules = ruleParser.GetRule();
    ASSERT_NE(rules, nullptr);
    ASSERT_EQ(rules->typeList.size(), 0);
    ASSERT_EQ(rules->tagList.size(), 0);
    ASSERT_EQ(rules->eventList.size(), 0);
    ASSERT_EQ(rules->domainRuleMap.size(), 0);
}

/**
 * @tc.name: DispatchRuleParser003
 * @tc.desc: Test the api of DispatchRuleParser for error json.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DispatchRuleParserTest, DispatchRuleParser003, testing::ext::TestSize.Level0)
{
    DispatchRuleParser ruleParser(TEST_FILE_DIR + "test_json_nojson");
    auto rules = ruleParser.GetRule();
    ASSERT_EQ(rules, nullptr);
}

/**
 * @tc.name: DispatchRuleParser004
 * @tc.desc: Test the api of DispatchRuleParser for error type key.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DispatchRuleParserTest, DispatchRuleParser004, testing::ext::TestSize.Level0)
{
    DispatchRuleParser ruleParser(TEST_FILE_DIR + "test_json_error_type_key");
    auto rules = ruleParser.GetRule();
    ASSERT_NE(rules, nullptr);
    ASSERT_EQ(rules->typeList.size(), 0);
    ASSERT_EQ(rules->tagList.size(), 0);
    ASSERT_EQ(rules->eventList.size(), 0);
    ASSERT_EQ(rules->domainRuleMap.size(), 0);
}

/**
 * @tc.name: DispatchRuleParser005
 * @tc.desc: Test the api of DispatchRuleParser for error type value.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(DispatchRuleParserTest, DispatchRuleParser005, testing::ext::TestSize.Level0)
{
    DispatchRuleParser ruleParser(TEST_FILE_DIR + "test_json_error_type_value");
    auto rules = ruleParser.GetRule();
    ASSERT_NE(rules, nullptr);
    ASSERT_EQ(rules->typeList.size(), 0);
    ASSERT_EQ(rules->tagList.size(), 0);
    ASSERT_EQ(rules->eventList.size(), 0);
    ASSERT_EQ(rules->domainRuleMap.size(), 0);
}

/**
 * @tc.name: DispatchRuleParser006
 * @tc.desc: Test the api of DispatchRuleParser for all events of the domain.
 * @tc.type: FUNC
 */
HWTEST_F(DispatchRuleParserTest, DispatchRuleParser006, testing::ext::TestSize.Level0)
{
    DispatchRuleParser ruleParser(TEST_FILE_DIR + "test_json_all_event");
    auto rules = ruleParser.GetRule();
    ASSERT_NE(rules, nullptr);
    ASSERT_EQ(rules->domainRuleMap.size(), 1); // 1 domain: HIVIEWDFX
    ASSERT_TRUE(rules->FindEvent("HIVIEWDFX", "APP_USAGE"));
    ASSERT_TRUE(rules->FindEvent("HIVIEWDFX", "SYS_USAGE"));
    ASSERT_FALSE(rules->FindEvent("HIVIEWDFX_UE", "SYS_USAGE"));
}

/**
 * @tc.name: DispatchRuleParser007
 * @tc.desc: Test the api of DispatchRuleParser for invalid domain config.
 * @tc.type: FUNC
 */
HWTEST_F(DispatchRuleParserTest, DispatchRuleParser007, testing::ext::TestSize.Level0)
{
    DispatchRuleParser ruleParser(TEST_FILE_DIR + "test_json_invalid_domain");
    auto rules = ruleParser.GetRule();
    ASSERT_NE(rules, nullptr);
    ASSERT_EQ(rules->domainRuleMap.size(), 1); // 1 domain: HIVIEWDFX
    ASSERT_FALSE(rules->FindEvent("HIVIEWDFX", "APP_USAGE"));
}
} // namespace HiviewDFX
} // namespace OHOS
