/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#include "param_update_test.h"

#include "file_ex.h"
#include "file_util.h"
#include "param_manager.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;

namespace {
    const std::string TEST_CONFIG_FILE = "/data/system/hiview/test.txt";
    const std::string TEST_ANCO_CONFIG_PATH = "/data/system/hiview/anco/";
    const std::string CERT_CONFIG_FILE_FULL_NAME = "/data/test/test_data/CERT_PRE.config";
    const std::string CERT_ENC_FILE_FULL_NAME =
        "/data/service/el1/public/update/param_service/install/system/etc/HIVIEWPARA/DEFAULT/CERT.ENC";
    const std::string SOURCE_TEST_CONFIG_FILE =
        "/data/service/el1/public/update/param_service/install/system/etc/HIVIEWPARA/DEFAULT/test.txt";
    constexpr int CHAR_0 = 48; // '0'
    constexpr int CHAR_9 = 57; // '9'
    constexpr int CHAR_A = 65; // 'A'
    constexpr int CHAR_F = 70; // 'F'
    constexpr int HEX_BASE = 10;

int ToNum(char a)
{
    if (a >= CHAR_0 && a <= CHAR_9) {
        return static_cast<int>(a - '0');
    }

    if (a >= CHAR_A && a <= CHAR_F) {
        return (static_cast<int>(a - 'A') + HEX_BASE);
    }
    return 0;
}

void CreateEncFile()
{
    std::string content;
    FileUtil::LoadStringFromFile(CERT_CONFIG_FILE_FULL_NAME, content);
    std::vector<char> saveContent;
    const int hightBit = 4;
    const int hexLen = 2;
    const int secondHex = 1;
    for (int i = 0; i < content.length();) {
        saveContent.push_back(static_cast<char>(ToNum(content[i]) << hightBit) + (ToNum(content[i + secondHex])));
        i += hexLen;
    }

    (void)OHOS::SaveBufferToFile(CERT_ENC_FILE_FULL_NAME, saveContent, true);
}
}

void ParamUpdateTest::SetUp()
{
    /**
     * @tc.setup: create an event loop and multiple event handlers
     */
    std::string configDir("/data/test/test_data/hiview_platform_config");
    if (!platform.InitEnvironment(configDir)) {
        std::cout << "fail to init environment" << std::endl;
    } else {
        std::cout << "init environment successful" << std::endl;
    }
}

void ParamUpdateTest::TearDown()
{
    /**
     * @tc.teardown: destroy the event loop we have created
     */
    FileUtil::RemoveFile(TEST_CONFIG_FILE);
    FileUtil::ForceRemoveDirectory(TEST_ANCO_CONFIG_PATH);
}

/**
 * @tc.name: PlatformConfigParse001
 * @tc.desc: parse a correct config file and check result
 * @tc.type: FUNC
 * @tc.require: AR000DPTT5
 */
HWTEST_F(ParamUpdateTest, ParamUpdateTest001, TestSize.Level1)
{
    ParamManager::InitParam();
    ASSERT_FALSE(FileUtil::FileExists(TEST_CONFIG_FILE));
    CreateEncFile();
    ParamManager::InitParam();
    if (!FileUtil::FileExists(PUBKEY_PATH)) {
        ASSERT_FALSE(FileUtil::FileExists(TEST_CONFIG_FILE));
    } else {
        ASSERT_TRUE(FileUtil::FileExists(TEST_CONFIG_FILE));
    }
    FileUtil::RemoveFile(TEST_CONFIG_FILE);
    FileUtil::SaveStringToFile(SOURCE_TEST_CONFIG_FILE, "1234");
    ParamManager::InitParam();
    ASSERT_FALSE(FileUtil::FileExists(TEST_CONFIG_FILE));
}