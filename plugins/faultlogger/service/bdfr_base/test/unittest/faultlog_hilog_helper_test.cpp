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

#include <unistd.h>
#include <sys/syscall.h>

#include "faultlog_hilog_helper.h"
#include "hiview_logger.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
using namespace FaultlogHilogHelper;
DEFINE_LOG_LABEL(0xD002D11, "FaultloggerUT");

/**
 * @tc.name: ReadHilogUnittest001
 * @tc.desc: Faultlogger::ReadHilog
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, ReadHilogUnittest001, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. write log to hilog.
     */
    HIVIEW_LOGI("write log to hilog");

    /**
     * @tc.steps: step2. Create a pipe.
     */
    int fds[2] = {-1, -1}; // 2: one read pipe, one write pipe
    int ret = pipe(fds);
    ASSERT_EQ(ret, 0) << "Failed to create pipe for get log.";

    /**
     * @tc.steps: step3. ReadHilog.
     */
    int32_t pid = getpid();
    int childPid = fork();
    ASSERT_GE(childPid, 0);
    if (childPid == 0) {
        syscall(SYS_close, fds[0]);


        int rc = DoGetHilogProcess(pid, fds[1]);
        syscall(SYS_close, fds[1]);
        _exit(rc);
    } else {
        syscall(SYS_close, fds[1]);
        // read log from fds[0]
        HIVIEW_LOGI("read hilog start");
        std::string log = ReadHilogTimeout(fds[0]);
        syscall(SYS_close, fds[0]);
        ASSERT_TRUE(!log.empty());
    }
    waitpid(childPid, nullptr, 0);
}

/**
 * @tc.name: ReadHilogUnittest002
 * @tc.desc: Faultlogger::ReadHilog
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, ReadHilogUnittest002, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. write log to hilog.
     */
    HIVIEW_LOGI("write log to hilog");

    /**
     * @tc.steps: step2. Create a pipe.
     */
    int fds[2] = {-1, -1}; // 2: one read pipe, one write pipe
    int ret = pipe(fds);
    ASSERT_EQ(ret, 0) << "Failed to create pipe for get log.";

    /**
     * @tc.steps: step3. ReadHilog.
     */
    int32_t pid = getpid();
    int childPid = fork();
    ASSERT_GE(childPid, 0);
    if (childPid == 0) {
        syscall(SYS_close, fds[0]);
        sleep(7); // Delay for 7 seconds, causing the read end to timeout and exit

        int rc = DoGetHilogProcess(pid, fds[1]);
        syscall(SYS_close, fds[1]);
        _exit(rc);
    } else {
        syscall(SYS_close, fds[1]);
        // read log from fds[0]
        HIVIEW_LOGI("read hilog start");
        std::string log = ReadHilogTimeout(fds[0]);
        syscall(SYS_close, fds[0]);
        ASSERT_TRUE(log.empty());
    }
    waitpid(childPid, nullptr, 0);
}

/**
 * @tc.name: ReadHilogUnittest003
 * @tc.desc: Faultlogger::ReadHilog
 * @tc.type: FUNC
 */
HWTEST(FaultloggerUtilsUnittest, ReadHilogUnittest003, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. write log to hilog.
     */
    HIVIEW_LOGI("write log to hilog");

    /**
     * @tc.steps: step2. Create a pipe.
     */
    int fds[2] = {-1, -1}; // 2: one read pipe, one write pipe
    int ret = pipe(fds);
    ASSERT_EQ(ret, 0) << "Failed to create pipe for get log.";

    /**
     * @tc.steps: step3. ReadHilog.
     */
    int32_t pid = getpid();
    int childPid = fork();
    ASSERT_GE(childPid, 0);
    if (childPid == 0) {
        syscall(SYS_close, fds[0]);
        syscall(SYS_close, fds[1]);
        _exit(0);
    } else {
        syscall(SYS_close, fds[1]);
        // read log from fds[0]
        HIVIEW_LOGI("read hilog start");
        std::string log = ReadHilogTimeout(fds[0]);
        syscall(SYS_close, fds[0]);
        ASSERT_TRUE(log.empty());
    }
    waitpid(childPid, nullptr, 0);
}
} // namespace HiviewDFX
} // namespace OHOS
