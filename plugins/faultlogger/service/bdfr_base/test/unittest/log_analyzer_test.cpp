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

#include "log_analyzer.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {

/**
 * @tc.name: AnalysisFaultlogTest001
 * @tc.desc: create cpp crash event and check AnalysisFaultlog
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, AnalysisFaultlogTest001, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create a cpp crash event and pass it to faultlogger
     * @tc.expected: AnalysisFaultlog return expected result
     */
    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 0;
    info.pid = 7497;
    info.faultLogType = FaultLogType::CPP_CRASH;
    info.module = "com.example.testapplication";
    info.reason = "TestReason";
    std::map<std::string, std::string> eventInfos;
    ASSERT_EQ(AnalysisFaultlog(info, eventInfos), false);
    ASSERT_EQ(!eventInfos["FINGERPRINT"].empty(), true);
}

/**
 * @tc.name: AnalysisFaultlogTest002
 * @tc.desc: create Js crash FaultLogInfo and check AnalysisFaultlog
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, AnalysisFaultlogTest002, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. create Js crash FaultLogInfo
     * @tc.expected: AnalysisFaultlog return expected result
     */
    FaultLogInfo info;
    info.time = 1607161163;
    info.id = 0;
    info.pid = 7497;
    info.faultLogType = FaultLogType::JS_CRASH;
    info.module = "com.example.testapplication";
    info.reason = "TestReason";
    std::map<std::string, std::string> eventInfos;
    ASSERT_EQ(AnalysisFaultlog(info, eventInfos), false);
    ASSERT_EQ(!eventInfos["FINGERPRINT"].empty(), true);
}
} // namespace HiviewDFX
} // namespace OHOS
