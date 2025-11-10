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
#include <string>
#include <vector>

#include "smart_parser.h"
#include "tbox.h"
#include "log_util.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace HiviewDFX {

/**
 * @tc.name: SmartParserSysWarningTest001
 * @tc.desc: Smart parser syswarning log.
 * @tc.type: FUNC
 */
HWTEST(SmartParserSysWarningTest, SmartParserSysWarningTest001, testing::ext::TestSize.Level0)
{
    /**
     * @tc.steps: step1. parser log
     */
    const std::string logPath = std::string{LogUtil::SMART_PARSER_TEST_DIR}  +
        "/SmartParserSysWarningTest001/syswarning-com.ohos.launcher-20010025-20251107170236-19700324235211000.tmp";
    const std::string eventType = "SYS_WARNING";
    auto eventInfos = SmartParser::Analysis(logPath, SMART_PARSER_PATH, eventType);

    /**
     * @tc.steps: step2. check result
     */
    ASSERT_EQ(eventInfos["TIME_OUT"], "test testTimer 1110 ms ago");

    std::string endStack = "#00 pc 00000000000bb328 /system/lib64/libc.so(__epoll_pwait+8)\n";
    endStack += "#01 pc 000000000000d370 /system/lib64/libeventhandler.z.so";
    endStack += "(OHOS::AppExecFwk::EpollIoWaiter::WaitFor(std::__1::unique_lock<std::__1::mutex>&, long)+192)\n";
    endStack += "#02 pc 0000000000011db4 /system/lib64/libeventhandler.z.so";
    endStack += "(OHOS::AppExecFwk::EventQueue::WaitUntilLocked(std::__1::chrono::time_point<";
    endStack += "std::__1::chrono::steady_clock, std::__1::chrono::duration<long long, ";
    endStack += "std::__1::ratio<1l, 1000000000l> > > const&, std::__1::unique_lock<std::__1::mutex>&)+96)\n";
    endStack +=
        R"(#03 pc 0000000000011cf8 /system/lib64/libeventhandler.z.so(OHOS::AppExecFwk::EventQueue::GetEvent()+112)
#04 pc 0000000000017728 /system/lib64/libeventhandler.z.so
#05 pc 000000000001608c /system/lib64/libeventhandler.z.so(OHOS::AppExecFwk::EventRunner::Run()+96)
#06 pc 000000000009380c /system/lib64/libappkit_native.z.so(OHOS::AppExecFwk::MainThread::Start()+372)
#07 pc 00000000000144b8 /system/bin/appspawn
#08 pc 00000000000139ec /system/bin/appspawn
#09 pc 000000000001fd90 /system/lib64/libbegetutil.z.so
#10 pc 000000000001f980 /system/lib64/libbegetutil.z.so
#11 pc 000000000001ccd0 /system/lib64/libbegetutil.z.so(ProcessEvent+108)
#12 pc 000000000001c6cc /system/lib64/libbegetutil.z.so
#13 pc 00000000000128b4 /system/bin/appspawn
#14 pc 000000000001053c /system/bin/appspawn
#15 pc 000000000006afa4 /system/lib64/libc.so(__libc_init+112)
)";

    ASSERT_EQ(eventInfos["END_STACK"], endStack);

    Tbox::FilterTrace(eventInfos, eventType);
    std::string firstFrame = "/system/lib64/libeventhandler.z.so";
    firstFrame += "(OHOS::AppExecFwk::EpollIoWaiter::WaitFor(std::__1::unique_lock<std::__1::mutex>&, long)+192";
    ASSERT_EQ(eventInfos["FIRST_FRAME"], firstFrame);
    std::string secondFrame = "/system/lib64/libeventhandler.z.so";
    secondFrame += "(OHOS::AppExecFwk::EventQueue::WaitUntilLocked";
    secondFrame += "(std::__1::chrono::time_point<std::__1::chrono::steady_clock, ";
    secondFrame += "std::__1::chrono::duration<long long, std::__1::ratio<1l, 1000000000l> > > const&, ";
    secondFrame += "std::__1::unique_lock<std::__1::mutex>&)+96";
    ASSERT_EQ(eventInfos["SECOND_FRAME"], secondFrame);
    ASSERT_EQ(eventInfos["LAST_FRAME"], "/system/bin/appspawn");
}
} // namespace HiviewDFX
} // namespace OHOS
