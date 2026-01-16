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

#include "process_status.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;

class ProcessStatusTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

/**
 * @tc.name: ProcessStatusTest001
 * @tc.desc: used to test func of ProcessStatus class
 * @tc.type: FUNC
*/
HWTEST_F(ProcessStatusTest, ProcessStatusTest001, TestSize.Level1)
{
    std::string procName = ProcessStatus::GetInstance().GetProcessName(-1);
    ASSERT_EQ(procName, "");
    procName = ProcessStatus::GetInstance().GetProcessName(1);
    ASSERT_EQ(procName, "init");
    procName = ProcessStatus::GetInstance().GetProcessName(1);
    ASSERT_EQ(procName, "init");
}

/**
 * @tc.name: ProcessStatusTest002
 * @tc.desc: used to test func of ProcessStatus class
 * @tc.type: FUNC
*/
HWTEST_F(ProcessStatusTest, ProcessStatusTest002, TestSize.Level1)
{
    ProcessState state = ProcessStatus::GetInstance().GetProcessState(1);
    ASSERT_EQ(state, ProcessState::BACKGROUND);
    uint64_t lastForegroundTime = ProcessStatus::GetInstance().GetProcessLastForegroundTime(1);
    ASSERT_EQ(lastForegroundTime, 0);
}

/**
 * @tc.name: ProcessStatusTest003
 * @tc.desc: used to test func of ProcessStatus class
 * @tc.type: FUNC
*/
HWTEST_F(ProcessStatusTest, ProcessStatusTest003, TestSize.Level1)
{
    // update init process to forground state for test purpose
    ProcessStatus::GetInstance().NotifyProcessState(1, ProcessState::FOREGROUND);
    ProcessState state = ProcessStatus::GetInstance().GetProcessState(1);
    ASSERT_EQ(state, ProcessState::FOREGROUND);
    uint64_t lastForegroundTime = ProcessStatus::GetInstance().GetProcessLastForegroundTime(1);
    ASSERT_GT(lastForegroundTime, 0);
}
