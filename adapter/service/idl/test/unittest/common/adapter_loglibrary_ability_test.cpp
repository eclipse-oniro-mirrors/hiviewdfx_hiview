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

#include "adapter_loglibrary_ability_test.h"

#include <string>
#include <vector>

#include "adapter_loglibrary_test_tools.h"
#include "file_util.h"
#include "hiview_service_ability.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
const std::string DEST_PATH = "/data/log/logpack/betaclub/";
const std::string SOURCE_PATH = "/data/log/logpack/remotelog/";
const std::string NON_LOG_TYPE = "NONTYPE";
const std::string LOG_TYPE = "REMOTELOG";
}
void AdapterLoglibraryAbilityTest::SetUpTestCase() {}

void AdapterLoglibraryAbilityTest::TearDownTestCase() {}

void AdapterLoglibraryAbilityTest::SetUp()
{
    AdapterLoglibraryTestTools::ApplyPermissionAccess();
    AdapterLoglibraryTestTools::CreateDir(SOURCE_PATH);
    AdapterLoglibraryTestTools::CreateDir(DEST_PATH);
    if (service == nullptr) {
        service = new HiviewService();
        HiviewServiceAbility::GetOrSetHiviewService(service);
    }
}

void AdapterLoglibraryAbilityTest::TearDown()
{
    AdapterLoglibraryTestTools::RemoveDir(DEST_PATH);
    AdapterLoglibraryTestTools::RemoveDir(SOURCE_PATH);
    if (service != nullptr) {
        delete service;
        service = nullptr;
    }
}

HWTEST_F(AdapterLoglibraryAbilityTest, LoglibraryAbilityListTest001, testing::ext::TestSize.Level1)
{
    std::string fileName = "AbilityList.txt";
    FileUtil::SaveStringToFile(SOURCE_PATH + fileName, "AbilityListtestcontent", true);
    std::vector<HiviewFileInfo> fileInfos;
    HiviewServiceAbility ability;
    int32_t result = ability.List(LOG_TYPE, fileInfos);
    ASSERT_EQ(result, 0);
}

HWTEST_F(AdapterLoglibraryAbilityTest, LoglibraryAbilityListTest002, testing::ext::TestSize.Level1)
{
    std::vector<HiviewFileInfo> fileInfos;
    HiviewServiceAbility ability;
    int32_t result = ability.List(NON_LOG_TYPE, fileInfos);
    ASSERT_EQ(result, HiviewNapiErrCode::ERR_INNER_INVALID_LOGTYPE);
}

HWTEST_F(AdapterLoglibraryAbilityTest, LoglibraryAbilityCopyTest001, testing::ext::TestSize.Level1)
{
    std::string fileName = "AbilityCopy.txt";
    FileUtil::SaveStringToFile(SOURCE_PATH + fileName, "AbilityCopytestcontent", true);
    HiviewServiceAbility ability;
    int32_t result = ability.Copy(LOG_TYPE, fileName, DEST_PATH);
    ASSERT_EQ(result, -1);
}

HWTEST_F(AdapterLoglibraryAbilityTest, LoglibraryAbilityCopyTest002, testing::ext::TestSize.Level1)
{
    std::string fileName = "AbilityCopy.txt";
    HiviewServiceAbility ability;
    int32_t result = ability.Copy(NON_LOG_TYPE, fileName, DEST_PATH);
    ASSERT_EQ(result, HiviewNapiErrCode::ERR_INNER_INVALID_LOGTYPE);
}

HWTEST_F(AdapterLoglibraryAbilityTest, LoglibraryAbilityMoveTest001, testing::ext::TestSize.Level1)
{
    std::string fileName = "AbilityMove.txt";
    FileUtil::SaveStringToFile(SOURCE_PATH + fileName, "AbilityMovetestcontent", true);
    HiviewServiceAbility ability;
    int32_t result = ability.Move(LOG_TYPE, fileName, DEST_PATH);
    ASSERT_EQ(result, -1);
}

HWTEST_F(AdapterLoglibraryAbilityTest, LoglibraryAbilityMoveTest002, testing::ext::TestSize.Level1)
{
    std::string fileName = "AbilityMove.txt";
    HiviewServiceAbility ability;
    int32_t result = ability.Move(NON_LOG_TYPE, fileName, DEST_PATH);
    ASSERT_EQ(result, HiviewNapiErrCode::ERR_INNER_INVALID_LOGTYPE);
}

HWTEST_F(AdapterLoglibraryAbilityTest, LoglibraryAbilityRemoveTest001, testing::ext::TestSize.Level1)
{
    std::string fileName = "AbilityRemove.txt";
    FileUtil::SaveStringToFile(SOURCE_PATH + fileName, "AbilityRemovetestcontent", true);
    HiviewServiceAbility ability;
    int32_t result = ability.Remove(LOG_TYPE, fileName);
    ASSERT_EQ(result, 0);
}

HWTEST_F(AdapterLoglibraryAbilityTest, LoglibraryAbilityRemoveTest002, testing::ext::TestSize.Level1)
{
    std::string fileName = "AbilityRemove.txt";
    HiviewServiceAbility ability;
    int32_t result = ability.Remove(NON_LOG_TYPE, fileName);
    ASSERT_EQ(result, HiviewNapiErrCode::ERR_INNER_INVALID_LOGTYPE);
}
}
}