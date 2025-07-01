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
 * @tc.name: SmartParserPanicTest001
 * @tc.desc: Smart paerser non-encrypted panic log.
 * @tc.type: FUNC
 */
HWTEST(SmartParserPanicTest, SmartParserPanicTest001, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. parser log
     */
    const std::string logPath = std::string{LogUtil::SMART_PARSER_TEST_DIR}  +
        "/SmartParserPanicTest001/19700101000000-00000007/ap_log/dmesg-ramoops-0";
    const std::string eventType = "PANIC";
    auto eventInfos = SmartParser::Analysis(logPath, SMART_PARSER_PATH, eventType);

    /**
     * @tc.steps: step2. check result
     */
    std::string endStack = "<dump_backtrace+0x0/0x8>\n";
    endStack += "<show_stack+0x24/0x30>\n";
    endStack += "<dump_stack+0xc0/0x11c>\n";
    endStack += "<panic+0x178/0x3f0>\n";
    endStack += "<sysrq_handle_term+0x0/0x38>\n";
    endStack += "<__handle_sysrq+0x170/0x1b8>\n";
    endStack += "<write_sysrq_trigger+0xb4/0xe0>\n";
    endStack += "<proc_reg_write+0x90/0x1a0>\n";
    endStack += "<vfs_write+0x14c/0x410>\n";
    endStack += "<ksys_write+0x84/0xf4>\n";
    endStack += "<__arm64_sys_write+0x28/0x34>\n";
    endStack += "<el0_svc_common+0x134/0x22c>\n";
    endStack += "<el0_svc_compat+0x1c/0x28>\n";
    endStack += "<el0_sync_compat_handler+0xc0/0xf0>\n";
    endStack += "<el0_sync_compat+0x184/0x1c0>\n";

    ASSERT_EQ(eventInfos["END_STACK"], endStack);

    Tbox::FilterTrace(eventInfos, eventType);
    ASSERT_EQ(eventInfos["FIRST_FRAME"], "sysrq_handle_term+0x0/0x38");
    ASSERT_EQ(eventInfos["SECOND_FRAME"], "__handle_sysrq+0x170/0x1b8");
    ASSERT_EQ(eventInfos["LAST_FRAME"], "el0_sync_compat+0x184/0x1c0");
}

/**
 * @tc.name: SmartParserPanicTest002
 * @tc.desc: Smart paerser encrypted panic log.
 * @tc.type: FUNC
 */
HWTEST(SmartParserPanicTest, SmartParserPanicTest002, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. parser log
     */
    const std::string logPath = std::string{LogUtil::SMART_PARSER_TEST_DIR}  +
        "/SmartParserPanicTest002/19700101000000-00000007/ap_log/dmesg-ramoops-0";
    const std::string eventType = "PANIC";
    auto eventInfos = SmartParser::Analysis(logPath, SMART_PARSER_PATH, eventType);

    /**
     * @tc.steps: step2. check result
     */
    std::string endStack = "<dump_backtrace+0x0/0x0>\n";
    endStack += "<show_stack+0x0/0x0>\n";
    endStack += "<dump_stack+0x0/0x0>\n";
    endStack += "<panic+0x0/0x0>\n";
    endStack += "<sysrq_handle_term+0x0/0x0>\n";
    endStack += "<__handle_sysrq+0x0/0x0>\n";
    endStack += "<write_sysrq_trigger+0x0/0x0>\n";
    endStack += "<proc_reg_write+0x0/0x0>\n";
    endStack += "<vfs_write+0x0/0x0>\n";
    endStack += "<ksys_write+0x0/0x0>\n";
    endStack += "<__arm64_sys_write+0x0/0x0>\n";
    endStack += "<el0_svc_common+0x0/0x0>\n";
    endStack += "<el0_svc_compat+0x0/0x0>\n";
    endStack += "<el0_sync_compat_handler+0x0/0x0>\n";
    endStack += "<el0_sync_compat+0x0/0x0>\n";

    ASSERT_EQ(eventInfos["END_STACK"], endStack);

    Tbox::FilterTrace(eventInfos, eventType);
    ASSERT_EQ(eventInfos["FIRST_FRAME"], "sysrq_handle_term+0x0/0x0");
    ASSERT_EQ(eventInfos["SECOND_FRAME"], "__handle_sysrq+0x0/0x0");
    ASSERT_EQ(eventInfos["LAST_FRAME"], "el0_sync_compat+0x0/0x0");
}

/**
 * @tc.name: SmartParserPanicTest003
 * @tc.desc: Smart paerser hguard abnormal panic log.
 * @tc.type: FUNC
 */
HWTEST(SmartParserPanicTest, SmartParserPanicTest003, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. parser log
     */
    const std::string logPath = std::string{LogUtil::SMART_PARSER_TEST_DIR}  +
        "/SmartParserPanicTest003/19700101000000-00000007/ap_log/dmesg-ramoops-0";
    const std::string eventType = "PANIC";
    auto eventInfos = SmartParser::Analysis(logPath, SMART_PARSER_PATH, eventType);

    /**
     * @tc.steps: step2. check result
     */
    std::string endStack = "<dump_backtrace+0x0/0x0>\n";
    endStack += "<show_stack+0x0/0x0>\n";
    endStack += "<dump_stack+0x0/0x0>\n";
    endStack += "<panic+0x0/0x0>\n";
    endStack += "<sysrq_handle_term+0x0/0x0>\n";
    endStack += "<__handle_sysrq+0x0/0x0>\n";
    endStack += "<write_sysrq_trigger+0x0/0x0>\n";
    endStack += "<proc_reg_write+0x0/0x0>\n";
    endStack += "<vfs_write+0x0/0x0>\n";
    endStack += "<ksys_write+0x0/0x0>\n";
    endStack += "<__arm64_sys_write+0x0/0x0>\n";
    endStack += "<el0_svc_common+0x0/0x0>\n";
    endStack += "<el0_svc_compat+0x0/0x0>\n";
    endStack += "<el0_sync_compat_handler+0x0/0x0>\n";
    endStack += "<el0_sync_compat+0x0/0x0>\n";

    ASSERT_EQ(eventInfos["END_STACK"], endStack);

    Tbox::FilterTrace(eventInfos, eventType);
    ASSERT_EQ(eventInfos["FIRST_FRAME"], "sysrq_handle_term+0x0/0x0");
    ASSERT_EQ(eventInfos["SECOND_FRAME"], "__handle_sysrq+0x0/0x0");
    ASSERT_EQ(eventInfos["LAST_FRAME"], "el0_sync_compat+0x0/0x0");
}
} // namespace HiviewDFX
} // namespace OHOS
