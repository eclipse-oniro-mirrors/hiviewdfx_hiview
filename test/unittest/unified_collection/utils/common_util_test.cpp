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

#include "common_util.h"
#include "file_util.h"
#include "time_util.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;

class CommonUtilTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

namespace {
constexpr int32_t MAX_FILE_NUM = 5;
const std::string TEST_PATH = "/data/test/hiview/unified_collection";
}

/**
 * @tc.name: CommonUtilTest001
 * @tc.desc: used to test common util in UCollect
 * @tc.type: FUNC
*/
HWTEST_F(CommonUtilTest, CommonUtilTest001, TestSize.Level1)
{
    std::string testStr;
    std::string type;
    int64_t value = 0;
    ASSERT_EQ(CommonUtil::ParseTypeAndValue(testStr, type, value), false);
    testStr = "key: 1";
    ASSERT_EQ(CommonUtil::ParseTypeAndValue(testStr, type, value), true);
    ASSERT_EQ(type, "key");
    ASSERT_EQ(value, 1);
    testStr = "key: 1 kB";
    ASSERT_EQ(CommonUtil::ParseTypeAndValue(testStr, type, value), true);
    ASSERT_EQ(type, "key");
    ASSERT_EQ(value, 1);
}

/**
 * @tc.name: CommonUtilTest002
 * @tc.desc: used to test common util in UCollect
 * @tc.type: FUNC
*/
HWTEST_F(CommonUtilTest, CommonUtilTest002, TestSize.Level1)
{
    int ret = CommonUtil::GetFileNameNum("fileName", ".txt");
    ASSERT_EQ(ret, 0);
    ret = CommonUtil::GetFileNameNum("fileName_1", ".txt");
    ASSERT_EQ(ret, 0);
    ret = CommonUtil::GetFileNameNum("fileName.txt_", ".txt");
    ASSERT_EQ(ret, 0);
    ret = CommonUtil::GetFileNameNum("fileName_.txt", ".txt");
    ASSERT_EQ(ret, 0);
    ret = CommonUtil::GetFileNameNum("fileName_1.txt", ".txt");
    ASSERT_EQ(ret, 1);
}

/**
 * @tc.name: CommonUtilTest003
 * @tc.desc: used to test common util in UCollect
 * @tc.type: FUNC
*/
HWTEST_F(CommonUtilTest, CommonUtilTest003, TestSize.Level1)
{
    int32_t ret = CommonUtil::ReadNodeWithOnlyNumber("invalid_path");
    ASSERT_EQ(ret, 0);
    ret = CommonUtil::ReadNodeWithOnlyNumber("/proc/1/oom_score_adj");
    ASSERT_NE(ret, 0);
}

/**
 * @tc.name: CommonUtilTest004
 * @tc.desc: used to test CreateExportFile, file name without pid.
 * @tc.type: FUNC
*/
HWTEST_F(CommonUtilTest, CommonUtilTest004, TestSize.Level1)
{
    // path not exit.
    FileUtil::ForceRemoveDirectory(TEST_PATH);
    ASSERT_TRUE(FileUtil::FileExists(CommonUtil::CreateExportFile(TEST_PATH, MAX_FILE_NUM, "file_name_", ".txt")));

    uint64_t fileTime = TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC;
    std::string timeFormat1 = TimeUtil::TimestampFormatToDate(fileTime - 1, "%Y%m%d%H%M%S");
    std::string timeFormat2 = TimeUtil::TimestampFormatToDate(fileTime, "%Y%m%d%H%M%S");
    FileUtil::CreateFile(TEST_PATH + "/file_name_" + timeFormat1 + "_11.txt");
    FileUtil::CreateFile(TEST_PATH + "/file_name_" + timeFormat2 + ".txt");
    FileUtil::CreateFile(TEST_PATH + "/file_name_" + timeFormat2 + "_1.txt");
    FileUtil::CreateFile(TEST_PATH + "/file_name_" + timeFormat2 + "_2.txt");
    FileUtil::CreateFile(TEST_PATH + "/file_name_" + timeFormat2 + "_10.txt");

    ASSERT_TRUE(FileUtil::FileExists(CommonUtil::CreateExportFile(TEST_PATH, MAX_FILE_NUM, "file_name_", ".txt")));
    ASSERT_FALSE(FileUtil::FileExists(TEST_PATH + "/file_name_" + timeFormat1 + "_11.txt"));

    ASSERT_TRUE(FileUtil::FileExists(CommonUtil::CreateExportFile(TEST_PATH, MAX_FILE_NUM, "file_name_", ".txt")));
    ASSERT_FALSE(FileUtil::FileExists(TEST_PATH + "/file_name_" + timeFormat2 + ".txt"));

    ASSERT_TRUE(FileUtil::FileExists(CommonUtil::CreateExportFile(TEST_PATH, MAX_FILE_NUM, "file_name_", ".txt")));
    ASSERT_FALSE(FileUtil::FileExists(TEST_PATH + "/file_name_" + timeFormat2 + "_1.txt"));
}

/**
 * @tc.name: CommonUtilTest005
 * @tc.desc: used to test CreateExportFile, file name with pid.
 * @tc.type: FUNC
*/
HWTEST_F(CommonUtilTest, CommonUtilTest005, TestSize.Level1)
{
    // path not exit.
    FileUtil::ForceRemoveDirectory(TEST_PATH);
    ASSERT_TRUE(FileUtil::FileExists(CommonUtil::CreateExportFile(TEST_PATH, MAX_FILE_NUM, "file_", ".txt", "12_")));

    uint64_t fileTime = TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC;
    std::string timeFormat1 = TimeUtil::TimestampFormatToDate(fileTime - 1, "%Y%m%d%H%M%S");
    std::string timeFormat2 = TimeUtil::TimestampFormatToDate(fileTime, "%Y%m%d%H%M%S");
    FileUtil::CreateFile(TEST_PATH + "/file_33_" + timeFormat1 + "_11.txt");
    FileUtil::CreateFile(TEST_PATH + "/file_4_" + timeFormat2 + ".txt");
    FileUtil::CreateFile(TEST_PATH + "/file_5_" + timeFormat2 + "_1.txt");
    FileUtil::CreateFile(TEST_PATH + "/file_2_" + timeFormat2 + "_2.txt");
    FileUtil::CreateFile(TEST_PATH + "/file_11_" + timeFormat2 + "_10.txt");

    ASSERT_TRUE(FileUtil::FileExists(CommonUtil::CreateExportFile(TEST_PATH, MAX_FILE_NUM, "file_", ".txt", "12_")));
    ASSERT_FALSE(FileUtil::FileExists(TEST_PATH + "/file_33_" + timeFormat1 + "_11.txt"));

    ASSERT_TRUE(FileUtil::FileExists(CommonUtil::CreateExportFile(TEST_PATH, MAX_FILE_NUM, "file_", ".txt", "2_")));
    ASSERT_FALSE(FileUtil::FileExists(TEST_PATH + "/file_4_" + timeFormat2 + ".txt"));

    ASSERT_TRUE(FileUtil::FileExists(CommonUtil::CreateExportFile(TEST_PATH, MAX_FILE_NUM, "file_", ".txt", "1_")));
    ASSERT_FALSE(FileUtil::FileExists(TEST_PATH + "/file_5_" + timeFormat2 + "_1.txt"));
}
