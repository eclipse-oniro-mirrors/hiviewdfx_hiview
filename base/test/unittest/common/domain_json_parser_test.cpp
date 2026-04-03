/*
 * Copyright (c) 2026-2026 Huawei Device Co., Ltd.
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
#include "domain_json_parser_test.h"

#include "domain_json_parser.h"
#include "file_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr char FIRST_TEST_DOMAIN[] = "FIRST_TEST_DOMAIN";
constexpr char FIRST_TEST_NAME[] = "FIRST_TEST_NAME";
constexpr char SECOND_TEST_DOMAIN[] = "SECOND_TEST_DOMAIN";
constexpr char SECOND_TEST_NAME[] = "SECOND_TEST_NAME";

void RenameDefFile(const std::string& srcFileName, const std::string& destFileName)
{
    std::string dir = "/data/system/hiview/";
    FileUtil::RenameFile(dir + srcFileName, dir + destFileName);
}

void RemoveConfigVerFile()
{
    FileUtil::RemoveFile("/data/system/hiview/unzip_configs/sys_event_def/hiview_config_version");
}
}

void DomainJsonParserTest::SetUpTestCase() {}

void DomainJsonParserTest::TearDownTestCase() {}

void DomainJsonParserTest::SetUp() {}

void DomainJsonParserTest::TearDown() {}

/**
 * @tc.name: DomainJsonParserTest001
 * @tc.desc: parse the domain location info
 * @tc.type: FUNC
 * @tc.require: IBIY90
 */
HWTEST_F(DomainJsonParserTest, DomainJsonParserTest001, testing::ext::TestSize.Level0)
{
    RenameDefFile("hisysevent_normal.def", "hisysevent.def");
    RemoveConfigVerFile();
    std::unique_ptr<DomainJsonParser> domainJsonParser = std::make_unique<DomainJsonParser>();
    std::string defPath = "/data/system/hiview/hisysevent.def";
    domainJsonParser->CacheDomainJsonLocation(defPath);
    ASSERT_EQ(domainJsonParser->GetDomainLocationMap()->size(), 2);

    auto locationMap = domainJsonParser->GetDomainLocationMap();
    for (auto iter = locationMap->cbegin(); iter!=locationMap->cend(); iter++) {
        printf("key: %s, start: %d, length: %d \n", iter->first.c_str(), iter->second.startPos, iter->second.length);
    }

    Json::Value firstDomainJson;
    ASSERT_EQ(domainJsonParser->ParseDomainJsonFromFile(FIRST_TEST_DOMAIN, firstDomainJson), true);
    ASSERT_EQ(firstDomainJson.getMemberNames()[0], FIRST_TEST_NAME);

    Json::Value secondDomainJson;
    ASSERT_EQ(domainJsonParser->ParseDomainJsonFromFile(SECOND_TEST_DOMAIN, secondDomainJson), true);
    ASSERT_EQ(secondDomainJson.getMemberNames()[0], SECOND_TEST_NAME);

    Json::Value domainJson;
    ASSERT_EQ(domainJsonParser->ParseDomainJsonFromFile("INVALID_TEST_DOMAIN", domainJson), false);
}
} // namespace HiviewDFX
} // namespace OHOS
