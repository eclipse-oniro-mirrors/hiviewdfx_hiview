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
#include "hilog_collector.h"
#include <iostream>
#include <unistd.h>
#include <gtest/gtest.h>
#include "hiview_logger.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace OHOS::HiviewDFX::UCollect;

class HilogCollectorTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

#ifdef UNIFIED_COLLECTOR_HILOG_ENABLE
namespace {
DEFINE_LOG_TAG("HilogCollectorTest");
const std::string TEST_STR = "HilogCollectorTest";
constexpr uint32_t HILOG_LINE_NUM = 100;
constexpr uint32_t SLEEP_TIME = 300 * 1000; // 300ms
constexpr uint32_t LOG_SLEEP_TIME = 1000; // 1ms
}

/**
 * @tc.name: HilogCollectorTest001
 * @tc.desc: write hilog, collect hilog success
 * @tc.type: FUNC
*/
HWTEST_F(HilogCollectorTest, HilogCollectorTest001, TestSize.Level1)
{
    int childPid = fork();
    ASSERT_GE(childPid, 0);

    if (childPid == 0) {
        // clear hilog buffer at first
        execl("/system/bin/hilog", "hilog", "-r", nullptr);
        _exit(EXIT_SUCCESS);
    } else {
        usleep(SLEEP_TIME);
        for (uint32_t idx = 0; idx < HILOG_LINE_NUM; idx++) {
            HIVIEW_LOGE("%{public}s", TEST_STR.c_str());
            usleep(LOG_SLEEP_TIME);
        }

        // get current process log
        usleep(SLEEP_TIME);
        std::shared_ptr<HilogCollector> collector = HilogCollector::Create();
        pid_t pid = getpid();
        CollectResult<std::string> result = collector->CollectLastLog(pid, HILOG_LINE_NUM);
        ASSERT_TRUE(result.retCode == UcError::SUCCESS);
        ASSERT_TRUE(result.data.find(TEST_STR) != std::string::npos);
        int lineNum = std::count(result.data.begin(), result.data.end(), '\n');
        std::cout << "line num :" << lineNum << std::endl;
        ASSERT_TRUE(lineNum >= HILOG_LINE_NUM);

        // get child process log
        result = collector->CollectLastLog(childPid, HILOG_LINE_NUM);
        ASSERT_TRUE(result.retCode == UcError::SUCCESS);
        ASSERT_TRUE(result.data == "");
    }
}
#else
/**
 * @tc.name: HilogCollectorTest001
 * @tc.desc: empty test
 * @tc.type: FUNC
*/
HWTEST_F(HilogCollectorTest, HilogCollectorTest001, TestSize.Level1)
{
    std::shared_ptr<HilogCollector> collector = HilogCollector::Create();
    CollectResult<std::string> result = collector->CollectLastLog(0, 0);
    ASSERT_TRUE(result.retCode == UcError::FEATURE_CLOSED);
}
#endif
