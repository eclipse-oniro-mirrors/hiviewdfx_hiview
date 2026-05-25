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
#include <fcntl.h>
#include <file_util.h>
#include <gtest/gtest.h>

#include "faultlogger_base.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
constexpr uint32_t MAX_NAME_LENGTH = 4096;
constexpr const char* const TEST_TEMP_FILE = "/data/test/testfile";
/**
 * @tc.name: QuerySelfFaultLog001
 * @tc.desc: Test calling querySelfFaultLog Func
 * @tc.type: FUNC
 */
HWTEST(FaultloggerBaseTest, QuerySelfFaultLog001, testing::ext::TestSize.Level3)
{
    FaultloggerBase faultloggerBase;
    faultloggerBase.QuerySelfFaultLog(100001, 0, 10, 101);
    faultloggerBase.QuerySelfFaultLog(100001, 0, 4, 101);
    auto ret = faultloggerBase.QuerySelfFaultLog(100001, 0, -1, 101);
    ASSERT_TRUE(ret.empty());
}

/**
 * @tc.name: GetGwpAsanGrayscaleState001
 * @tc.desc: Test calling GwpAsanGrayscal Func
 * @tc.type: FUNC
 */
HWTEST(FaultloggerBaseTest, GetGwpAsanGrayscaleState001, testing::ext::TestSize.Level3)
{
    FaultloggerBase faultloggerBase;
    GwpAsanParams params1 {
        .alwaysEnabled = false,
        .sampleRate = 1000,
        .maxSimutaneousAllocations = 2000,
        .duration = 5,
        .isRecover = false,
    };
    GwpAsanParams params2 {
        .alwaysEnabled = true,
        .sampleRate = 2523,
        .maxSimutaneousAllocations = 2000,
        .duration = 5,
        .isRecover = true,
    };
    faultloggerBase.EnableGwpAsanGrayscale(params1, static_cast<int64_t>(getuid()));
    faultloggerBase.EnableGwpAsanGrayscale(params2, static_cast<int64_t>(getuid()));
    faultloggerBase.DisableGwpAsanGrayscale(static_cast<int64_t>(getuid()));
    ASSERT_TRUE(faultloggerBase.GetGwpAsanGrayscaleState(static_cast<int64_t>(getuid())) >= 0);
}

/**
 * @tc.name: FaultlogDumpTest001
 * @tc.desc: Test calling FaultlogDump Func
 * @tc.type: FUNC
 */
HWTEST(FaultloggerBaseTest, FaultlogDumpTest001, testing::ext::TestSize.Level3)
{
    FaultloggerBase faultloggerBase;
    std::vector<std::string> cmds;
    const std::string paramStr = "invalid parameters.\n";
    std::string cmdResult;
    cmds.push_back("-f");
    cmds.push_back("");
    int fd = TEMP_FAILURE_RETRY(open("/data/test/testFile", O_CREAT | O_WRONLY | O_TRUNC, 770));
    std::string result;
    faultloggerBase.FaultLogDumpByCommands(fd, cmds);
    FileUtil::LoadStringFromFile("/data/test/testFile", result);
    cmdResult += paramStr;
    ASSERT_EQ(result, cmdResult);
    for (size_t i = 0; i < MAX_NAME_LENGTH + 1; i++) {
        cmds[1] += "1";
    }
    faultloggerBase.FaultLogDumpByCommands(fd, cmds);
    FileUtil::LoadStringFromFile("/data/test/testFile", result);
    cmdResult += paramStr;
    ASSERT_EQ(result, cmdResult);
    cmds[0] = "-f";
    cmds[1] = "test";
    faultloggerBase.FaultLogDumpByCommands(fd, cmds);
    FileUtil::LoadStringFromFile("/data/test/testFile", result);
    cmdResult += paramStr;
    ASSERT_EQ(result, cmdResult);
    cmds[0] = "-f";
    cmds[1] = "testtesttestte";
    faultloggerBase.FaultLogDumpByCommands(fd, cmds);
    FileUtil::LoadStringFromFile("/data/test/testFile", result);
    cmdResult += paramStr;
    ASSERT_EQ(result, cmdResult);
    cmds[0] = "-m";
    cmds[1] = "";
    faultloggerBase.FaultLogDumpByCommands(fd, cmds);
    FileUtil::LoadStringFromFile("/data/test/testFile", result);
    cmdResult += paramStr;
    ASSERT_EQ(result, cmdResult);
    cmds[0] = "";
    cmds[1] = "";
    faultloggerBase.FaultLogDumpByCommands(fd, cmds);
    FileUtil::LoadStringFromFile("/data/test/testFile", result);
    ASSERT_NE(faultloggerBase.GetExtensionDelayTime(), 0);
}

/**
 * @tc.name: EnableGwpAsanInner001
 * @tc.desc: Test calling EnableGwpAsanInner Func
 * @tc.type: FUNC
 */
HWTEST(FaultloggerBaseTest, EnableGwpAsanInner001, testing::ext::TestSize.Level3)
{
    FaultloggerBase faultloggerBase;
    GwpAsanParams params {
        .alwaysEnabled = true,
        .sampleRate = 1000,
        .maxSimutaneousAllocations = 2000,
        .duration = 5,
        .isRecover = false,
    };
    ASSERT_TRUE(faultloggerBase.EnableGwpAsanInner(params, "test_process") >= 0);
}

/**
 * @tc.name: EnableGwpAsanInner002
 * @tc.desc: Test calling EnableGwpAsanInner Func
 * @tc.type: FUNC
 */
HWTEST(FaultloggerBaseTest, EnableGwpAsanInner002, testing::ext::TestSize.Level3)
{
    FaultloggerBase faultloggerBase;
    GwpAsanParams params {
        .alwaysEnabled = false,
        .sampleRate = 500,
        .maxSimutaneousAllocations = 100,
        .duration = 3,
        .isRecover = true,
    };
    ASSERT_FALSE(faultloggerBase.EnableGwpAsanInner(params, "test_process_not_dha"));
}

/**
 * @tc.name: CheckCallerIsDha001
 * @tc.desc: Test calling CheckCallerIsAllowed Func
 * @tc.type: FUNC
 */
HWTEST(FaultloggerBaseTest, CheckCallerIsDha001, testing::ext::TestSize.Level3)
{
    FaultloggerBase faultloggerBase;
    ASSERT_FALSE(faultloggerBase.CheckCallerIsAllowed());
}
} // namespace HiviewDFX
} // namespace OHOS
