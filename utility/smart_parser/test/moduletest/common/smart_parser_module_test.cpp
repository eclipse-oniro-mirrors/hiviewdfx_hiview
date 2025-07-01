/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "smart_parser_module_test.h"

#include <cstdio>
#include <ctime>
#include <errors.h>
#include <map>
#include <string>

#include "compose_rule.h"
#include "extract_rule.h"
#include "feature_analysis.h"
#include "file_util.h"
#include "log_util.h"
#include "smart_parser.h"
#include "string_util.h"

using namespace std;
namespace OHOS {
namespace HiviewDFX {
using namespace testing::ext;
static const std::string TEST_CONFIG = "/data/test/test_data/SmartParser/common/";
static const std::string TEST_COMPOSE_CONFIG = "test_compose_rule.json";
static const std::string TEST_EXTRACT_CONFIG = "test_extract_rule.json";

void SmartParserModuleTest::SetUpTestCase(void) {}

void SmartParserModuleTest::TearDownTestCase(void) {}

void SmartParserModuleTest::SetUp(void) {}

void SmartParserModuleTest::TearDown(void) {}

/**
 * @tc.name: SmartParserTest001
 * @tc.desc: process cpp_crash fault, this case match compose_rule.json and extract_rule.json.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  +
                            "/SmartParserTest001/cppcrash-com.ohos.launcher-20010025-19700324235211000.log";
    std::string trustStack = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest001/trace.txt";

    ASSERT_EQ(FileUtil::FileExists(faultFile), true);
    ASSERT_EQ(FileUtil::FileExists(trustStack), true);

    /**
     * @tc.steps: step2. smart parser process fault log
     */
    auto eventInfos = SmartParser::Analysis(faultFile, TEST_CONFIG, "CPP_CRASH");

    /**
     * @tc.steps: step3. check the result of eventinfo for fault.
     * @tc.expected: step3. equal to correct answer.
     */
    EXPECT_STREQ(eventInfos["PNAME"].c_str(), "com.ohos.launcher");
    EXPECT_EQ(eventInfos["END_STACK"].size() > 0, true);
    std::stringstream buff;
    LogUtil::ReadFileBuff(trustStack, buff);
    std::vector<std::string> trace;
    StringUtil::SplitStr(eventInfos["END_STACK"], LogUtil::SPLIT_PATTERN, trace, false, false);
    std::string line;
    size_t num = 0;
    while (getline(buff, line) && num < trace.size()) {
        EXPECT_STREQ(line.c_str(), trace[num++].c_str());
    }
}

/**
 * @tc.name: SmartParserTest002
 * @tc.desc: process JS_ERROR fault, this case match compose_rule.json and extract_rule.json.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  +
                            "/SmartParserTest002/jscrash-com.example.jsinject-20010041-19700424183123000.log";
    std::string trustStack = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest002/trace.txt";
    ASSERT_EQ(FileUtil::FileExists(faultFile), true);
    ASSERT_EQ(FileUtil::FileExists(trustStack), true);

    /**
     * @tc.steps: step2. smart parser process fault log
     */
    auto eventInfos = SmartParser::Analysis(faultFile, TEST_CONFIG, "JS_ERROR");

    /**
     * @tc.steps: step3. check the result of eventinfo for fault.
     * @tc.expected: step3. equal to correct answer.
     */
    EXPECT_STREQ(eventInfos["PNAME"].c_str(), "com.example.jsinject");
    EXPECT_EQ(eventInfos["END_STACK"].size() > 0, true);
    std::stringstream buff;
    LogUtil::ReadFileBuff(trustStack, buff);
    std::vector<std::string> trace;
    StringUtil::SplitStr(eventInfos["END_STACK"], LogUtil::SPLIT_PATTERN, trace, false, false);
    std::string line;
    size_t num = 0;
    while (getline(buff, line) && num < trace.size()) {
        EXPECT_STREQ(line.c_str(), trace[num++].c_str());
    }
}

/**
 * @tc.name: SmartParserTest003
 * @tc.desc: process freeze fault, this case match compose_rule.json and extract_rule.json.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  +
                            "/SmartParserTest003/appfreeze-com.example.jsinject-20010039-19700326211815000.log";
    std::string traceFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest003/trace.txt";
    ASSERT_EQ(FileUtil::FileExists(faultFile), true);
    ASSERT_EQ(FileUtil::FileExists(traceFile), true);

    /**
     * @tc.steps: step2. smart parser process crash fault log
     */
    auto eventInfos = SmartParser::Analysis(faultFile, TEST_CONFIG, "APP_FREEZE");

    /**
     * @tc.steps: step3. check the result of eventinfo for fault.
     * @tc.expected: step3. equal to correct answer.
     */
    EXPECT_EQ(eventInfos["END_STACK"].size() > 0, true);
    std::string content;
    bool isSuccess = FileUtil::LoadStringFromFile(traceFile, content);
    if (!isSuccess) {
        ASSERT_FALSE(isSuccess);
        printf("read logFile: %s failed", traceFile.c_str());
    } else {
        std::stringstream buff(content);
        std::vector<std::string> trace;
        StringUtil::SplitStr(eventInfos["END_STACK"], LogUtil::SPLIT_PATTERN, trace, false, false);
        std::string line;
        size_t num = 0;
        while (getline(buff, line) && num < trace.size()) {
            EXPECT_STREQ(trace[num++].c_str(), line.c_str());
        }
    }
}

/**
 * @tc.name: SmartParserTest004
 * @tc.desc: process PANIC fault, this case match compose_rule.json and extract_rule.json.
 *           1. fault log should can be read;
 *           2. compose_rule.json and extract_rule.json. should match the json file in perforce.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest004/last_kmsg";
    std::string traceFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest004/trace.txt";
    ASSERT_EQ(FileUtil::FileExists(faultFile), true);
    ASSERT_EQ(FileUtil::FileExists(traceFile), true);
    std::stringstream buff;
    LogUtil::ReadFileBuff(traceFile, buff);

    /**
     * @tc.steps: step2. smart parser process crash fault log
     */
    auto eventInfos = SmartParser::Analysis(faultFile, TEST_CONFIG, "PANIC");

    std::vector<std::string> trace;
    StringUtil::SplitStr(eventInfos["END_STACK"], LogUtil::SPLIT_PATTERN, trace, false, false);
    std::string line;
    size_t num = 0;
    while (getline(buff, line) && num < trace.size()) {
        EXPECT_STREQ(trace[num++].c_str(), line.c_str());
    }
}

/**
 * @tc.name: SmartParserTest005
 * @tc.desc: process HWWATCHDOG fault, this case match compose_rule.json and extract_rule.json.
 *           1. fault log should can be read;
 *           2. compose_rule.json and extract_rule.json. should match the json file in perforce.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest005, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest005/last_kmsg";
    std::string traceFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest005/trace.txt";
    ASSERT_EQ(FileUtil::FileExists(faultFile), true);
    ASSERT_EQ(FileUtil::FileExists(traceFile), true);
    std::stringstream buff;
    LogUtil::ReadFileBuff(traceFile, buff);

    /**
     * @tc.steps: step2. smart parser process crash fault log
     */
    auto eventInfos = SmartParser::Analysis(faultFile, TEST_CONFIG, "HWWATCHDOG");

    std::vector<std::string> trace;
    StringUtil::SplitStr(eventInfos["END_STACK"], LogUtil::SPLIT_PATTERN, trace, false, false);
    std::string line;
    size_t num = 0;
    while (getline(buff, line) && num < trace.size()) {
        EXPECT_STREQ(trace[num++].c_str(), line.c_str());
    }
}

/**
 * @tc.name: SmartParserTest006
 * @tc.desc: process HWWATCHDOG fault, this case match compose_rule.json and extract_rule.json.
 *           1. fault log should can be read;
 *           2. compose_rule.json and extract_rule.json. should match the json file in perforce.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest006, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest005/last_kmsg-1";
    ASSERT_EQ(FileUtil::FileExists(faultFile), false);

    /**
     * @tc.steps: step2. smart parser process crash fault log
     */
    auto eventInfos = SmartParser::Analysis(faultFile, TEST_CONFIG, "HWWATCHDOG");
    ASSERT_EQ(eventInfos.empty(), true);
}

/**
 * @tc.name: SmartParserTest007
 * @tc.desc: process test fault, this case match compose_rule.json and extract_rule.json.
 *           1. fault log should can be read;
 *           2. compose_rule.json and extract_rule.json. should match the json file in perforce.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest007, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    auto eventInfos = SmartParser::Analysis("", "/system/etc/hiview/reliability", "TEST");
    ASSERT_EQ(eventInfos.empty(), true);
    eventInfos = SmartParser::Analysis("", "", "TEST");
    ASSERT_EQ(eventInfos.empty(), true);
    eventInfos = SmartParser::Analysis("", "", "");
    ASSERT_EQ(eventInfos.empty(), true);
    eventInfos = SmartParser::Analysis("test", "test", "test");
    ASSERT_EQ(eventInfos.empty(), true);
}

/**
 * @tc.name: SmartParserTest008
 * @tc.desc: process RUST_PANIC fault, this case match compose_rule.json and extract_rule.json.
 *           1. fault log should can be read;
 *           2. compose_rule.json and extract_rule.json. should match the json file in perforce.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest008, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  +
        "/SmartParserTest006/rustpanic-rustpanic_maker-0-20230419222113000.log";
    std::string traceFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest006/trace.txt";
    ASSERT_EQ(FileUtil::FileExists(faultFile), true);
    ASSERT_EQ(FileUtil::FileExists(traceFile), true);
    std::stringstream buff;
    LogUtil::ReadFileBuff(traceFile, buff);

    /**
     * @tc.steps: step2. smart parser process crash fault log
     */
    auto eventInfos = SmartParser::Analysis(faultFile, TEST_CONFIG, "RUST_PANIC");
    ASSERT_EQ(!eventInfos.empty(), true);

    std::vector<std::string> trace;
    StringUtil::SplitStr(eventInfos["END_STACK"], LogUtil::SPLIT_PATTERN, trace, false, false);
    std::string line;
    size_t num = 0;
    while (getline(buff, line) && num < trace.size()) {
        EXPECT_STREQ(trace[num++].c_str(), line.c_str());
    }
}

/**
 * @tc.name: SmartParserTest009
 * @tc.desc: process PANIC fault, this case match test_compose_rule.json and test_extract_rule.json.
 *           1. fault log should can be read;
 *           2. test_compose_rule.json and test_extract_rule.json. should match the json file in perforce.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest009, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::map<std::string, FeatureSet> extract;
    std::list<std::pair<std::string, std::map<std::string, std::string>>> compose;
    std::map<std::string, std::vector<std::string>> segStatusCfg;
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest004/last_kmsg";
    std::string traceFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest004/trace.txt";

    ASSERT_EQ(FileUtil::FileExists(faultFile), true);
    ASSERT_EQ(FileUtil::FileExists(traceFile), true);
    std::stringstream buff;
    LogUtil::ReadFileBuff(traceFile, buff);

    /**
     * @tc.steps: step2. smart parser process crash fault log
     */
    ExtractRule extractRule;
    ComposeRule composeRule;
    std::string extractConfig = TEST_CONFIG + TEST_EXTRACT_CONFIG;
    extractRule.ParseExtractRule("PANIC", extractConfig, faultFile);
    extract = extractRule.GetExtractRule();
    segStatusCfg = extractRule.GetSegStatusCfg();
    std::string composeConfig = TEST_CONFIG + TEST_COMPOSE_CONFIG;
    composeRule.ParseComposeRule(composeConfig, "PANIC", extractRule.GetFeatureId());
    compose = composeRule.GetComposeRule();

    ASSERT_EQ(!extract.empty(), true);
    ASSERT_EQ(!segStatusCfg.empty(), true);
    ASSERT_EQ(!compose.empty(), true);
}

/**
 * @tc.name: SmartParserTest010
 * @tc.desc: process RUST_PANIC fault, this case match test_compose_rule.json and test_extract_rule.json.
 *           1. fault log should can be read;
 *           2. test_compose_rule.json and test_extract_rule.json. should match the json file in perforce.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest010, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::map<std::string, FeatureSet> extract;
    std::list<std::pair<std::string, std::map<std::string, std::string>>> compose;
    std::map<std::string, std::vector<std::string>> segStatusCfg;
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  +
        "/SmartParserTest006/rustpanic-rustpanic_maker-0-20230419222113000.log";
    std::string traceFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest006/trace.txt";

    ASSERT_EQ(FileUtil::FileExists(faultFile), true);
    ASSERT_EQ(FileUtil::FileExists(traceFile), true);
    std::stringstream buff;
    LogUtil::ReadFileBuff(traceFile, buff);

    /**
     * @tc.steps: step2. smart parser process crash fault log
     */
    ExtractRule extractRule;
    ComposeRule composeRule;
    std::string extractConfig = TEST_CONFIG + "/" + TEST_EXTRACT_CONFIG;
    extractRule.ParseExtractRule("RUST_PANIC", extractConfig, faultFile);
    extract = extractRule.GetExtractRule();
    segStatusCfg = extractRule.GetSegStatusCfg();
    std::string composeConfig = TEST_CONFIG + "/" + TEST_COMPOSE_CONFIG;
    composeRule.ParseComposeRule(composeConfig, "RUST_PANIC", extractRule.GetFeatureId());
    compose = composeRule.GetComposeRule();

    ASSERT_EQ(!extract.empty(), true);
    ASSERT_EQ(!segStatusCfg.empty(), true);
    ASSERT_EQ(!compose.empty(), true);
}

/**
 * @tc.name: SmartParserTest011
 * @tc.desc: process RUST_PANIC fault, this case match test_compose_rule.json and test_extract_rule.json.
 *           1. fault log should can be read;
 *           2. test_compose_rule.json and test_extract_rule.json. should match the json file in perforce.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest011, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::map<std::string, FeatureSet> extract;
    std::list<std::pair<std::string, std::map<std::string, std::string>>> compose;
    std::map<std::string, std::vector<std::string>> segStatusCfg;
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  +
        "/SmartParserTest007/rustpanic-rustpanic_maker-0-20230419222113000.log";
    std::string traceFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest007/trace.txt";

    ASSERT_EQ(FileUtil::FileExists(faultFile), true);
    ASSERT_EQ(FileUtil::FileExists(traceFile), true);
    std::stringstream buff;
    LogUtil::ReadFileBuff(traceFile, buff);

    /**
     * @tc.steps: step2. smart parser process crash fault log
     */
    ExtractRule extractRule;
    ComposeRule composeRule;
    std::string extractConfig = "/data/test/test_data/SmartParser/SmartParserTest007/" + TEST_EXTRACT_CONFIG;
    extractRule.ParseExtractRule("RUST_PANIC", extractConfig, faultFile);
    extract = extractRule.GetExtractRule();
    segStatusCfg = extractRule.GetSegStatusCfg();
    std::string composeConfig = "/data/test/test_data/SmartParser/SmartParserTest007/" + TEST_COMPOSE_CONFIG;
    composeRule.ParseComposeRule(composeConfig, "RUST_PANIC", extractRule.GetFeatureId());
    compose = composeRule.GetComposeRule();

    std::map<std::string, std::string> eventInfoMap;
    for (const auto& composeRules : compose) {
        FeatureAnalysis feature(extract[composeRules.first], composeRules.second, "RUST_PANIC");
        if (feature.AnalysisLog()) {
            auto result = feature.GetReasult();
            for (const auto& one : result) {
                eventInfoMap.emplace(one.first, one.second);
            }
        }
    }
}

/**
 * @tc.name: SmartParserTest012
 * @tc.desc: process PANIC fault, this case match test_compose_rule.json and test_extract_rule.json.
 *           1. fault log should can be read;
 *           2. test_compose_rule.json and test_extract_rule.json. should match the json file in perforce.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest012, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::map<std::string, FeatureSet> extract;
    std::list<std::pair<std::string, std::map<std::string, std::string>>> compose;
    std::map<std::string, std::vector<std::string>> segStatusCfg;
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest008/last_kmsg";
    std::string traceFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest008/trace.txt";
    ASSERT_EQ(FileUtil::FileExists(faultFile), true);
    ASSERT_EQ(FileUtil::FileExists(traceFile), true);
    std::stringstream buff;
    LogUtil::ReadFileBuff(traceFile, buff);

    /**
     * @tc.steps: step2. smart parser process crash fault log
     */
    ExtractRule extractRule;
    ComposeRule composeRule;
    std::string extractConfig = "/data/test/test_data/SmartParser/SmartParserTest008/" + TEST_EXTRACT_CONFIG;
    extractRule.ParseExtractRule("PANIC", extractConfig, faultFile);
    extract = extractRule.GetExtractRule();
    segStatusCfg = extractRule.GetSegStatusCfg();
    std::string composeConfig = "/data/test/test_data/SmartParser/SmartParserTest008/" + TEST_COMPOSE_CONFIG;
    composeRule.ParseComposeRule(composeConfig, "PANIC", extractRule.GetFeatureId());
    compose = composeRule.GetComposeRule();

    std::map<std::string, std::string> eventInfoMap;
    for (const auto& composeRules : compose) {
        FeatureAnalysis feature(extract[composeRules.first], composeRules.second, "PANIC");
        if (feature.AnalysisLog()) {
            auto result = feature.GetReasult();
            for (const auto& one : result) {
                eventInfoMap.emplace(one.first, one.second);
            }
        }
    }
}

/**
 * @tc.name: SmartParserTest013
 * @tc.desc: process RUST_PANIC fault, this case match test_compose_rule.json and test_extract_rule.json.
 *           1. fault log should can be read;
 *           2. test_compose_rule.json and test_extract_rule.json. should match the json file in perforce.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest013, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::map<std::string, FeatureSet> extract;
    std::list<std::pair<std::string, std::map<std::string, std::string>>> compose;
    std::map<std::string, std::vector<std::string>> segStatusCfg;
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  +
        "/SmartParserTest009/rustpanic-rustpanic_maker-0-20230419222113000.log";
    std::string traceFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest009/trace.txt";
    ASSERT_EQ(FileUtil::FileExists(faultFile), true);
    ASSERT_EQ(FileUtil::FileExists(traceFile), true);
    std::stringstream buff;
    LogUtil::ReadFileBuff(traceFile, buff);

    /**
     * @tc.steps: step2. smart parser process crash fault log
     */
    ExtractRule extractRule;
    ComposeRule composeRule;
    std::string extractConfig = "/data/test/test_data/SmartParser/SmartParserTest009/" + TEST_EXTRACT_CONFIG;
    extractRule.ParseExtractRule("RUST_PANIC", extractConfig, faultFile);
    extract = extractRule.GetExtractRule();
    segStatusCfg = extractRule.GetSegStatusCfg();
    std::string composeConfig = "/data/test/test_data/SmartParser/SmartParserTest009/" + TEST_COMPOSE_CONFIG;
    composeRule.ParseComposeRule(composeConfig, "RUST_PANIC", extractRule.GetFeatureId());
    compose = composeRule.GetComposeRule();

    std::map<std::string, std::string> eventInfoMap;
    for (const auto& composeRules : compose) {
        FeatureAnalysis feature(extract[composeRules.first], composeRules.second, "RUST_PANIC");
        if (feature.AnalysisLog()) {
            auto result = feature.GetReasult();
            for (const auto& one : result) {
                eventInfoMap.emplace(one.first, one.second);
            }
        }
    }
}

/**
 * @tc.name: SmartParserTest014
 * @tc.desc: process APP_FREEZE fault, this case match test_compose_rule.json and test_extract_rule.json.
 *           1. fault log should can be read;
 *           2. test_compose_rule.json and test_extract_rule.json. should match the json file in perforce.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest014, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::map<std::string, FeatureSet> extract;
    std::list<std::pair<std::string, std::map<std::string, std::string>>> compose;
    std::map<std::string, std::vector<std::string>> segStatusCfg;
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  +
        "/SmartParserTest010/appfreeze-com.example.jsinject-20010039-19700326211815000.log";
    std::string traceFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest010/trace.txt";
    ASSERT_EQ(FileUtil::FileExists(faultFile), true);
    ASSERT_EQ(FileUtil::FileExists(traceFile), true);
    std::stringstream buff;
    LogUtil::ReadFileBuff(traceFile, buff);

    /**
     * @tc.steps: step2. smart parser process crash fault log
     */
    ExtractRule extractRule;
    ComposeRule composeRule;
    std::string extractConfig = "/data/test/test_data/SmartParser/SmartParserTest010/" + TEST_EXTRACT_CONFIG;
    extractRule.ParseExtractRule("APP_FREEZE", extractConfig, faultFile);
    extract = extractRule.GetExtractRule();
    segStatusCfg = extractRule.GetSegStatusCfg();
    std::string composeConfig = "/data/test/test_data/SmartParser/SmartParserTest010/" + TEST_COMPOSE_CONFIG;
    composeRule.ParseComposeRule(composeConfig, "APP_FREEZE", extractRule.GetFeatureId());
    compose = composeRule.GetComposeRule();

    std::map<std::string, std::string> eventInfoMap;
    for (const auto& composeRules : compose) {
        FeatureAnalysis feature(extract[composeRules.first], composeRules.second, "APP_FREEZE");
        if (feature.AnalysisLog()) {
            auto result = feature.GetReasult();
            for (const auto& one : result) {
                eventInfoMap.emplace(one.first, one.second);
            }
        }
    }
}

/**
 * @tc.name: SmartParserTest015
 * @tc.desc: process APP_FREEZE fault, this case match test_compose_rule.json and test_extract_rule.json.
 *           1. fault log should can be read;
 *           2. test_compose_rule.json and test_extract_rule.json. should match the json file in perforce.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest015, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::map<std::string, FeatureSet> extract;
    std::list<std::pair<std::string, std::map<std::string, std::string>>> compose;
    std::map<std::string, std::vector<std::string>> segStatusCfg;
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  +
        "/SmartParserTest011/appfreeze-com.example.jsinject-20010039-19700326211815000.log";
    std::string traceFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest011/trace.txt";
    ASSERT_EQ(FileUtil::FileExists(faultFile), true);
    ASSERT_EQ(FileUtil::FileExists(traceFile), true);

    /**
     * @tc.steps: step2. smart parser process crash fault log
     */
    ExtractRule extractRule;
    ComposeRule composeRule;
    std::string extractConfig = "/data/test/test_data/SmartParser/SmartParserTest011/" + TEST_EXTRACT_CONFIG;
    extractRule.ParseExtractRule("APP_FREEZE", extractConfig, faultFile);
    extract = extractRule.GetExtractRule();
    segStatusCfg = extractRule.GetSegStatusCfg();
    std::string composeConfig = "/data/test/test_data/SmartParser/SmartParserTest011/" + TEST_COMPOSE_CONFIG;
    composeRule.ParseComposeRule(composeConfig, "APP_FREEZE", extractRule.GetFeatureId());
    compose = composeRule.GetComposeRule();

    std::map<std::string, std::string> eventInfoMap;
    for (const auto &composeRules : compose)
    {
        FeatureAnalysis feature(extract[composeRules.first], composeRules.second, "APP_FREEZE");
        if (feature.AnalysisLog())
        {
            auto result = feature.GetReasult();
            for (const auto &one : result)
            {
                eventInfoMap.emplace(one.first, one.second);
            }
            std::string writeLine(2049, 't');
            stringstream buffer(writeLine);
            feature.RawInfoPosition(buffer);

            stringstream bufferTwo(" test");
            feature.RawInfoPosition(bufferTwo);

            stringstream bufferThree("\t");
            feature.RawInfoPosition(bufferThree);

            bool segmentStart = true;
            feature.CheckStartSegment(segmentStart);
        }
    }
}

/**
 * @tc.name: SmartParserTest016
 * @tc.desc: process RUST_PANIC fault, this case match test_compose_rule.json and test_extract_rule.json.
 *           1. fault log should can be read;
 *           2. test_compose_rule.json and test_extract_rule.json. should match the json file in perforce.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuwei
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest016, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::map<std::string, FeatureSet> extract;
    std::list<std::pair<std::string, std::map<std::string, std::string>>> compose;
    std::map<std::string, std::vector<std::string>> segStatusCfg;
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  +
        "/SmartParserTest012/rustpanic-rustpanic_maker-0-20230419222113000.log";
    std::string traceFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest012/trace.txt";

    ASSERT_EQ(FileUtil::FileExists(faultFile), true);
    ASSERT_EQ(FileUtil::FileExists(traceFile), true);
    std::stringstream buff;
    LogUtil::ReadFileBuff(traceFile, buff);

    /**
     * @tc.steps: step2. smart parser process crash fault log
     */
    ExtractRule extractRule;
    ComposeRule composeRule;
    std::string extractConfig = "/data/test/test_data/SmartParser/SmartParserTest012/" + TEST_EXTRACT_CONFIG;
    extractRule.ParseExtractRule("RUST_PANIC", extractConfig, faultFile);
    extract = extractRule.GetExtractRule();
    segStatusCfg = extractRule.GetSegStatusCfg();
    std::string composeConfig = "/data/test/test_data/SmartParser/SmartParserTest012/" + TEST_COMPOSE_CONFIG;
    composeRule.ParseComposeRule(composeConfig, "RUST_PANIC", extractRule.GetFeatureId());
    compose = composeRule.GetComposeRule();

    std::map<std::string, std::string> eventInfoMap;
    for (const auto& composeRules : compose) {
        FeatureAnalysis feature(extract[composeRules.first], composeRules.second, "RUST_PANIC");
        if (feature.AnalysisLog()) {
            auto result = feature.GetReasult();
            for (const auto& one : result) {
                eventInfoMap.emplace(one.first, one.second);
            }
        }
    }
}

/**
 * @tc.name: SmartParserTest017
 * @tc.desc: process BOOTFAIL fault, this case match test_compose_rule.json and test_extract_rule.json.
 *           1. fault log should can be read;
 *           2. compose_rule.json and extract_rule.json. should match the json file in perforce.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: jincong
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest017, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest013/bootfail_info_0";
    std::string traceFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest013/trace.txt";
    ASSERT_EQ(FileUtil::FileExists(faultFile), true);
    ASSERT_EQ(FileUtil::FileExists(traceFile), true);
    std::stringstream buff;
    LogUtil::ReadFileBuff(traceFile, buff);

    /**
     * @tc.steps: step2. smart parser process crash fault log
     */
    auto eventInfos = SmartParser::Analysis(faultFile, TEST_CONFIG, "BOOTFAIL");

    std::vector<std::string> trace;
    StringUtil::SplitStr(eventInfos["END_STACK"], LogUtil::SPLIT_PATTERN, trace, false, false);
    std::string line;
    size_t num = 0;
    while (getline(buff, line) && num < trace.size()) {
        EXPECT_STREQ(trace[num++].c_str(), line.c_str());
    }
}

/**
 * @tc.name: SmartParserTest018
 * @tc.desc: process PANIC fault, this case match compose_rule.json and extract_rule.json.
 *           1. fault log should can be read;
 *           2. compose_rule.json and extract_rule.json. should match the json file in perforce.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: jincong
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest018, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest014/last_kmsg";
    std::string traceFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest014/trace.txt";
    ASSERT_EQ(FileUtil::FileExists(faultFile), true);
    ASSERT_EQ(FileUtil::FileExists(traceFile), true);
    std::stringstream buff;
    LogUtil::ReadFileBuff(traceFile, buff);

    /**
     * @tc.steps: step2. smart parser process crash fault log
     */
    auto eventInfos = SmartParser::Analysis(faultFile, TEST_CONFIG, "PANIC");

    std::vector<std::string> trace;
    StringUtil::SplitStr(eventInfos["END_STACK"], LogUtil::SPLIT_PATTERN, trace, false, false);
    std::string line;
    size_t num = 0;
    while (getline(buff, line) && num < trace.size()) {
        EXPECT_STREQ(trace[num++].c_str(), line.c_str());
    }
}

/**
 * @tc.name: SmartParserTest019
 * @tc.desc: process PANIC fault, this case match compose_rule.json and extract_rule.json.
 *           1. fault log should can be read;
 *           2. compose_rule.json and extract_rule.json. should match the json file in perforce.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: jincong
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest019, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest015/last_kmsg";
    std::string traceFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest015/trace.txt";
    ASSERT_EQ(FileUtil::FileExists(faultFile), true);
    ASSERT_EQ(FileUtil::FileExists(traceFile), true);
    std::stringstream buff;
    LogUtil::ReadFileBuff(traceFile, buff);

    /**
     * @tc.steps: step2. smart parser process crash fault log
     */
    auto eventInfos = SmartParser::Analysis(faultFile, TEST_CONFIG, "PANIC");

    std::vector<std::string> trace;
    StringUtil::SplitStr(eventInfos["END_STACK"], LogUtil::SPLIT_PATTERN, trace, false, false);
    std::string line;
    size_t num = 0;
    while (getline(buff, line) && num < trace.size()) {
        EXPECT_STREQ(trace[num++].c_str(), line.c_str());
    }
}

/**
 * @tc.name: SmartParserTest020
 * @tc.desc: process BOOTFAIL fault, this case match test_compose_rule.json and test_extract_rule.json.
 *           1. fault log should can be read;
 *           2. compose_rule.json and extract_rule.json. should match the json file in perforce.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: jincong
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest020, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest013/bootfail_info_x";
    ASSERT_EQ(FileUtil::FileExists(faultFile), false);

    /**
     * @tc.steps: step2. smart parser process crash fault log
     */
    auto eventInfos = SmartParser::Analysis(faultFile, TEST_CONFIG, "BOOTFAIL");
    ASSERT_EQ(eventInfos.empty(), true);
}

/**
 * @tc.name: SmartParserTest021
 * @tc.desc: process SENSORHUBCRASH fault, this case match test_compose_rule.json and test_extract_rule.json.
 *           1. fault log should can be read;
 *           2. compose_rule.json and extract_rule.json. should match the json file in perforce.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: jincong
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest021, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest016/history.log";
    ASSERT_EQ(FileUtil::FileExists(faultFile), true);

    /**
     * @tc.steps: step2. smart parser process crash fault log
     */
    auto eventInfos = SmartParser::Analysis(faultFile, TEST_CONFIG, "SENSORHUBCRASH");
    ASSERT_EQ(eventInfos.empty(), false);
}

/**
 * @tc.name: SmartParserTest022
 * @tc.desc: process MODEMCRASH fault, this case match test_compose_rule.json and test_extract_rule.json.
 *           1. fault log should can be read;
 *           2. compose_rule.json and extract_rule.json. should match the json file in perforce.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: jincong
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest022, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest017/reset.log";
    ASSERT_EQ(FileUtil::FileExists(faultFile), true);

    /**
     * @tc.steps: step2. smart parser process crash fault log
     */
    auto eventInfos = SmartParser::Analysis(faultFile, TEST_CONFIG, "MODEMCRASH");
    ASSERT_EQ(eventInfos.empty(), false);
}

/**
 * @tc.name: SmartParserTest023
 * @tc.desc: process PANIC fault, this case match test_compose_rule.json and test_extract_rule.json.
 *           1. fault log should can be read;
 *           2. compose_rule.json and extract_rule.json. should match the json file in perforce.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: jincong
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest023, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest018/last_kmsg";
    ASSERT_EQ(FileUtil::FileExists(faultFile), true);

    /**
     * @tc.steps: step2. smart parser process crash fault log
     */
    auto eventInfos = SmartParser::Analysis(faultFile, TEST_CONFIG, "PANIC");
    ASSERT_EQ(eventInfos.empty(), false);
}

/**
 * @tc.name: SmartParserTest024
 * @tc.desc: process DPACRASH fault, this case match test_compose_rule.json and test_extract_rule.json.
 *           1. fault log should can be read;
 *           2. compose_rule.json and extract_rule.json. should match the json file in perforce.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: jincong
 */
HWTEST_F(SmartParserModuleTest, SmartParserTest024, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set taskSheet fault log path and eventid.
     */
    std::string faultFile = std::string{LogUtil::SMART_PARSER_TEST_DIR}  + "/SmartParserTest017/reset.log";
    ASSERT_EQ(FileUtil::FileExists(faultFile), true);

    /**
     * @tc.steps: step2. smart parser process crash fault log
     */
    auto eventInfos = SmartParser::Analysis(faultFile, TEST_CONFIG, "DPACRASH");
    ASSERT_EQ(eventInfos.empty(), false);
}
}  // namespace HiviewDFX
}  // namespace OHOS
