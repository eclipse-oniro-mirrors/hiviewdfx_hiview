/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "bundle_util_test.h"

#include "bundle_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr int32_t TEST_UID = 20010000;
constexpr int32_t INVALID_UID = -1;
constexpr int32_t SYSTEM_UID = 1000;
constexpr char TEST_MAIN_DIR[] = "base";
constexpr char TEST_SUB_DIR[] = "cache";
constexpr char TEST_PATH_HOLDER[] = "com.test.demo";
constexpr char EMPTY_STRING[] = "";
}

void BundleUtilTest::SetUpTestCase()
{
}

void BundleUtilTest::TearDownTestCase()
{
}

void BundleUtilTest::SetUp()
{
}

void BundleUtilTest::TearDown()
{
}

/**
 * @tc.name: BundleUtilTest001
 * @tc.desc: Test GetSandBoxPath with valid parameters
 * @tc.type: FUNC
 */
HWTEST_F(BundleUtilTest, BundleUtilTest001, testing::ext::TestSize.Level3)
{
    std::string result = BundleUtil::GetSandBoxPath(
        TEST_UID, TEST_MAIN_DIR, TEST_PATH_HOLDER, TEST_SUB_DIR);
    ASSERT_FALSE(result.empty());
    ASSERT_TRUE(result.find("/data/app/el2/") != std::string::npos);
    ASSERT_TRUE(result.find(TEST_MAIN_DIR) != std::string::npos);
    ASSERT_TRUE(result.find(TEST_PATH_HOLDER) != std::string::npos);
    ASSERT_TRUE(result.find(TEST_SUB_DIR) != std::string::npos);
}

/**
 * @tc.name: BundleUtilTest002
 * @tc.desc: Test GetSandBoxPath with empty mainDir
 * @tc.type: FUNC
 */
HWTEST_F(BundleUtilTest, BundleUtilTest002, testing::ext::TestSize.Level3)
{
    std::string result = BundleUtil::GetSandBoxPath(
        TEST_UID, EMPTY_STRING, TEST_PATH_HOLDER, TEST_SUB_DIR);
    ASSERT_TRUE(result.empty());
}

/**
 * @tc.name: BundleUtilTest003
 * @tc.desc: Test GetSandBoxPath with empty pathHolder
 * @tc.type: FUNC
 */
HWTEST_F(BundleUtilTest, BundleUtilTest003, testing::ext::TestSize.Level3)
{
    std::string result = BundleUtil::GetSandBoxPath(
        TEST_UID, TEST_MAIN_DIR, EMPTY_STRING, TEST_SUB_DIR);
    ASSERT_TRUE(result.empty());
}

/**
 * @tc.name: BundleUtilTest004
 * @tc.desc: Test GetSandBoxPath with empty subDir
 * @tc.type: FUNC
 */
HWTEST_F(BundleUtilTest, BundleUtilTest004, testing::ext::TestSize.Level3)
{
    std::string result = BundleUtil::GetSandBoxPath(
        TEST_UID, TEST_MAIN_DIR, TEST_PATH_HOLDER, EMPTY_STRING);
    ASSERT_FALSE(result.empty());
    ASSERT_TRUE(result.find(TEST_MAIN_DIR) != std::string::npos);
    ASSERT_TRUE(result.find(TEST_PATH_HOLDER) != std::string::npos);
}

/**
 * @tc.name: BundleUtilTest005
 * @tc.desc: Test GetSandBoxPath with invalid uid
 * @tc.type: FUNC
 */
HWTEST_F(BundleUtilTest, BundleUtilTest005, testing::ext::TestSize.Level3)
{
    std::string result = BundleUtil::GetSandBoxPath(
        INVALID_UID, TEST_MAIN_DIR, TEST_PATH_HOLDER, TEST_SUB_DIR);
    ASSERT_FALSE(result.empty());
}

/**
 * @tc.name: BundleUtilTest006
 * @tc.desc: Test GetSandBoxPath with system uid
 * @tc.type: FUNC
 */
HWTEST_F(BundleUtilTest, BundleUtilTest006, testing::ext::TestSize.Level3)
{
    std::string result = BundleUtil::GetSandBoxPath(
        SYSTEM_UID, TEST_MAIN_DIR, TEST_PATH_HOLDER, TEST_SUB_DIR);
    ASSERT_FALSE(result.empty());
    ASSERT_TRUE(result.find("/data/app/el2/0/") != std::string::npos);
}

/**
 * @tc.name: BundleUtilTest007
 * @tc.desc: Test GetSandBoxPath with three parameters - empty mainDir
 * @tc.type: FUNC
 */
HWTEST_F(BundleUtilTest, BundleUtilTest007, testing::ext::TestSize.Level3)
{
    std::string result = BundleUtil::GetSandBoxPath(TEST_UID, EMPTY_STRING, TEST_SUB_DIR);
    ASSERT_TRUE(result.empty());
}

/**
 * @tc.name: BundleUtilTest08
 * @tc.desc: Test GetApplicationNameById with invalid uid
 * @tc.type: FUNC
 */
HWTEST_F(BundleUtilTest, BundleUtilTest008, testing::ext::TestSize.Level3)
{
    std::string result = BundleUtil::GetApplicationNameById(INVALID_UID);
    ASSERT_TRUE(result.empty());
}

/**
 * @tc.name: BundleUtilTest009
 * @tc.desc: Test GetUidByBundleName with empty bundle name
 * @tc.type: FUNC
 */
HWTEST_F(BundleUtilTest, BundleUtilTest009, testing::ext::TestSize.Level3)
{
    int32_t result = BundleUtil::GetUidByBundleName(EMPTY_STRING);
    ASSERT_EQ(result, -1);
}

/**
 * @tc.name: BundleUtilTest010
 * @tc.desc: Test GetUidByBundleName with invalid bundle name
 * @tc.type: FUNC
 */
HWTEST_F(BundleUtilTest, BundleUtilTest010, testing::ext::TestSize.Level3)
{
    int32_t result = BundleUtil::GetUidByBundleName("com.invalid.bundle.name");
    ASSERT_EQ(result, -1);
}

/**
 * @tc.name: BundleUtilTest011
 * @tc.desc: Test IsDebugHap with invalid uid
 * @tc.type: FUNC
 */
HWTEST_F(BundleUtilTest, BundleUtilTest011, testing::ext::TestSize.Level3)
{
    bool result = BundleUtil::IsDebugHap(INVALID_UID);
    ASSERT_FALSE(result);
}

/**
 * @tc.name: BundleUtilTest012
 * @tc.desc: Test IsDebugHap with system uid
 * @tc.type: FUNC
 */
HWTEST_F(BundleUtilTest, BundleUtilTest012, testing::ext::TestSize.Level3)
{
    bool result = BundleUtil::IsDebugHap(SYSTEM_UID);
    ASSERT_FALSE(result);
}

/**
 * @tc.name: BundleUtilTest013
 * @tc.desc: Test GetSandBoxPath path format verification
 * @tc.type: FUNC
 */
HWTEST_F(BundleUtilTest, BundleUtilTest013, testing::ext::TestSize.Level3)
{
    std::string mainDir = "base";
    std::string pathHolder = "com.example.test";
    std::string subDir = "files";
    int32_t uid = 20020000;
    
    std::string result = BundleUtil::GetSandBoxPath(uid, mainDir, pathHolder, subDir);
    ASSERT_FALSE(result.empty());
    
    std::string expectedPrefix = "/data/app/el2/100/";
    ASSERT_TRUE(result.find(expectedPrefix) != std::string::npos);
}

/**
 * @tc.name: BundleUtilTest014
 * @tc.desc: Test GetSandBoxPath with different user ids
 * @tc.type: FUNC
 */
HWTEST_F(BundleUtilTest, BundleUtilTest014, testing::ext::TestSize.Level3)
{
    int32_t uid1 = 20010000;
    int32_t uid2 = 20020000;
    
    std::string result1 = BundleUtil::GetSandBoxPath(uid1, TEST_MAIN_DIR, TEST_PATH_HOLDER, TEST_SUB_DIR);
    std::string result2 = BundleUtil::GetSandBoxPath(uid2, TEST_MAIN_DIR, TEST_PATH_HOLDER, TEST_SUB_DIR);
    
    ASSERT_TRUE(result1.find("/data/app/el2/100/") != std::string::npos);
    ASSERT_TRUE(result2.find("/data/app/el2/100/") != std::string::npos);
}
} // namespace HiviewDFX
} // namespace OHOS