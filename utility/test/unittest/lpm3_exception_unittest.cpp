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
 * @tc.name: SmartParserLpm3ExceptionTest001
 * @tc.desc: Smart paerser lpm3 exception log.
 * @tc.type: FUNC
 */
HWTEST(SmartParserLpm3ExceptionTest, SmartParserLpm3ExceptionTest001, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. parser log
     */
    const std::string logPath = std::string{LogUtil::SMART_PARSER_TEST_DIR}  +
        "/SmartParserLpm3ExceptionTest001/19700101000000-00000007/ap_log/fastboot_log";
    const std::string eventType = "LPM3EXCEPTION";
    auto eventInfos = SmartParser::Analysis(logPath, SMART_PARSER_PATH, eventType);

    /**
     * @tc.steps: step2. check result
     */
    ASSERT_EQ(eventInfos["FIRST_FRAME"], "0xFF");
    ASSERT_EQ(eventInfos["SECOND_FRAME"], "0x0");
    ASSERT_EQ(eventInfos["LAST_FRAME"], "sr position:[17]lpmcu sys resume io");
}

/**
 * @tc.name: SmartParserLpm3ExceptionTest002
 * @tc.desc: Smart paerser BBK lpm3 exception log.
 * @tc.type: FUNC
 */
HWTEST(SmartParserLpm3ExceptionTest, SmartParserLpm3ExceptionTest002, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. parser log
     */
    const std::string logPath = std::string{LogUtil::SMART_PARSER_TEST_DIR}  +
        "/SmartParserLpm3ExceptionTest002/19700101000000-00000007/ap_log/fastboot_log";
    const std::string eventType = "LPM3EXCEPTION";
    auto eventInfos = SmartParser::Analysis(logPath, SMART_PARSER_PATH, eventType);

    /**
     * @tc.steps: step2. check result
     */
    ASSERT_EQ(eventInfos["FIRST_FRAME"], "00000043");
    ASSERT_EQ(eventInfos["SECOND_FRAME"], "00000003");
    ASSERT_EQ(eventInfos["LAST_FRAME"], "sr position:[27]kernel resume out");
}
} // namespace HiviewDFX
} // namespace OHOS
