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

#include <gtest/gtest.h>
#include "trace_collector.h"
#include "file_util.h"
#include "test_trace_state_machine.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
namespace {
const std::string TEST_SAND_BOX_PATH = "/data/test/test_sand_box";
constexpr int32_t HIVIEW_UID = 1201;
}
class TraceImplTest : public testing::Test {
public:
    void SetUp() override
    {
        if (!FileUtil::FileExists(TEST_SAND_BOX_PATH)) {
            FileUtil::ForceCreateDirectory(TEST_SAND_BOX_PATH);
        }
        setuid(HIVIEW_UID); // hiview uid
    }

    void TearDown() override
    {
        if (FileUtil::FileExists(TEST_SAND_BOX_PATH)) {
            FileUtil::ForceRemoveDirectory(TEST_SAND_BOX_PATH);
        }
    }

    static void SetUpTestCase()
    {
        if (!FileUtil::FileExists(TEST_DB_PATH)) {
            if (!FileUtil::ForceCreateDirectory(TEST_DB_PATH)) {
                std::cout << "failed create dir:" << TEST_DB_PATH <<std::endl;
            }
        }
        if (!FileUtil::FileExists(TEST_SRC_PATH)) {
            if (!FileUtil::ForceCreateDirectory(TEST_SRC_PATH)) {
                std::cout << "failed create dir:" << TEST_SRC_PATH <<std::endl;
            }
        }
        if (!FileUtil::FileExists(TEST_CONFIG_PATH)) {
            if (!FileUtil::ForceCreateDirectory(TEST_CONFIG_PATH)) {
                std::cout << "failed create dir:" << TEST_CONFIG_PATH <<std::endl;
            }
        }
        CreateTraceFile(TRACE_TEST_SRC1);
    }

    static void TearDownTestCase()
    {
        if (FileUtil::FileExists(TEST_DB_PATH)) {
            if (!FileUtil::ForceRemoveDirectory(TEST_DB_PATH)) {
                std::cout << "remove dir:" << TEST_DB_PATH << " failed" << std::endl;
            }
        }
        if (FileUtil::FileExists(TEST_SRC_PATH)) {
            if (!FileUtil::ForceRemoveDirectory(TEST_SRC_PATH)) {
                std::cout << "remove dir:" << TEST_SRC_PATH << " failed" << std::endl;
            }
        }
        if (FileUtil::FileExists(TEST_CONFIG_PATH)) {
            if (!FileUtil::ForceRemoveDirectory(TEST_CONFIG_PATH)) {
                std::cout << "remove dir:" << TEST_CONFIG_PATH << " failed" << std::endl;
            }
        }
    }
};

/**
 * @tc.name: TraceImplTest001
 * @tc.desc: used to test app system trace
 * @tc.type: FUNC
*/
HWTEST_F(TraceImplTest, TraceImplTest001, TestSize.Level1)
{
    // prerequisites
    int32_t bufferSize = 100;
    std::string prefix = "test_prefix";
    std::string target = prefix + "_" + TRACE_TEST_ID1;
    auto collector = UCollectUtil::TraceCollector::Create();
    UCollect::AppBundleInfo appInfo {1000, 1100, "com.test.package", TEST_SAND_BOX_PATH, false};
    MockTraceStateMachine::GetInstance().SetTraceRet(TraceStateCode::SUCCESS, TraceErrorCode::SUCCESS);
    auto result1 = collector->OpenAppSystemTrace(bufferSize, appInfo);
    ASSERT_EQ(result1.retCode, UCollect::SUCCESS);

    MockTraceStateMachine::GetInstance().SetCurrentAppPid(appInfo.pid);
    MockTraceStateMachine::GetInstance().SetTraceInfo({
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 100,
        .outputFiles = { TRACE_TEST_SRC1 }
    });
    auto result12 = collector->DumpAppSystemTrace(prefix, 2000, appInfo);
    ASSERT_EQ(result1.retCode, UCollect::SUCCESS);
    std::cout << "trace:" << result12.data << std::endl;
    ASSERT_TRUE(IsContainSrcTrace(TEST_SAND_BOX_PATH, target));

    // uid 1000 dump again will over duration limits
    auto collector2 = UCollectUtil::TraceCollector::Create();
    UCollect::AppBundleInfo appInfo2 {1000, 1200, "com.test.package", TEST_SAND_BOX_PATH, false};
    MockTraceStateMachine::GetInstance().SetTraceRet(TraceStateCode::SUCCESS, TraceErrorCode::SUCCESS);
    auto result2 = collector->OpenAppSystemTrace(bufferSize, appInfo);
    ASSERT_EQ(result2.retCode, UCollect::TRACE_OVER_FLOW);
}

/**
 * @tc.name: TraceUtilsTest002
 * @tc.desc: used to test trace ret trans to uc error
 * @tc.type: FUNC
*/
HWTEST_F(TraceImplTest, TraceImplTest002, TestSize.Level1)
{
    // prerequisites
    int32_t bufferSize = 100;
    auto collector = UCollectUtil::TraceCollector::Create();
    UCollect::AppBundleInfo appInfo {1001, 1201, "com.test.package", TEST_SAND_BOX_PATH, false};
    MockTraceStateMachine::GetInstance().SetTraceRet(TraceStateCode::SUCCESS, TraceErrorCode::SUCCESS);
    auto result1 = collector->OpenAppSystemTrace(bufferSize, appInfo);
    ASSERT_EQ(result1.retCode, UCollect::SUCCESS);

    MockTraceStateMachine::GetInstance().SetCurrentAppPid(appInfo.pid);
    MockTraceStateMachine::GetInstance().SetTraceInfo({
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 100,
        .outputFiles = { TRACE_TEST_SRC2 } // trace src file not exist
    });
    UCollect::AppBundleInfo appInfo2 {1001, 1201, "com.test.package", TEST_SAND_BOX_PATH, false};
    std::string prefix = "test_prefix";
    auto result2 = collector->DumpAppSystemTrace(prefix, 100, appInfo2);
    ASSERT_EQ(result2.retCode, UCollect::WRITE_FAILED); // open time do not greater than 0
}

/**
 * @tc.name: TraceUtilsTest003
 * @tc.desc: used to test trace ret trans to uc error
 * @tc.type: FUNC
*/
HWTEST_F(TraceImplTest, TraceImplTest003, TestSize.Level1)
{
    // prerequisites
    int32_t bufferSize = 100;
    auto collector = UCollectUtil::TraceCollector::Create();
    UCollect::AppBundleInfo appInfo {1002, 1201, "com.test.package", TEST_SAND_BOX_PATH, false};
    MockTraceStateMachine::GetInstance().SetTraceRet(TraceStateCode::SUCCESS, TraceErrorCode::SUCCESS);
    auto result1 = collector->OpenAppSystemTrace(bufferSize, appInfo);
    ASSERT_EQ(result1.retCode, UCollect::SUCCESS);

    MockTraceStateMachine::GetInstance().SetCurrentAppPid(appInfo.pid);
    MockTraceStateMachine::GetInstance().SetTraceInfo({
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 100,
        .outputFiles = {} // trace src file empty
    });
    UCollect::AppBundleInfo appInfo2 {1002, 1201, "com.test.package", TEST_SAND_BOX_PATH, false};
    std::string prefix = "test_prefix";
    auto result2 = collector->DumpAppSystemTrace(prefix, 100, appInfo);
    ASSERT_EQ(result2.retCode, UCollect::SYSTEM_ERROR);
}

/**
 * @tc.name: TraceUtilsTest004
 * @tc.desc: used to test trace ret trans to uc error
 * @tc.type: FUNC
*/
HWTEST_F(TraceImplTest, TraceImplTest004, TestSize.Level1)
{
    // prerequisites
    int32_t bufferSize = 100;
    auto collector = UCollectUtil::TraceCollector::Create();
    UCollect::AppBundleInfo appInfo {1003, 1201, "com.test.package", TEST_SAND_BOX_PATH, false};
    MockTraceStateMachine::GetInstance().SetTraceRet(TraceStateCode::SUCCESS, TraceErrorCode::SUCCESS);
    auto result1 = collector->OpenAppSystemTrace(bufferSize, appInfo);
    ASSERT_EQ(result1.retCode, UCollect::SUCCESS);

    MockTraceStateMachine::GetInstance().SetCurrentAppPid(appInfo.pid);
    MockTraceStateMachine::GetInstance().SetTraceInfo({
        .errorCode = TraceErrorCode::SUCCESS,
        .fileSize = 100,
        .outputFiles = { "" } // trace src file empty
    });
    UCollect::AppBundleInfo appInfo2 {1003, 1201, "com.test.package", TEST_SAND_BOX_PATH, false};
    std::string prefix = "test_prefix";
    auto result2 = collector->DumpAppSystemTrace(prefix, 100, appInfo);
    ASSERT_EQ(result2.retCode, UCollect::SYSTEM_ERROR);
}
