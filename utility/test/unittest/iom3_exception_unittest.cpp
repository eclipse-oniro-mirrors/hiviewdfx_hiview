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

#include "smart_parser.h"
#include "tbox.h"
#include "log_util.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace HiviewDFX {

/**
 * @tc.name: SmartParserIOM3ExceptionTest001
 * @tc.desc: Smart paerser SENSORHUBCRASH log.
 * @tc.type: FUNC
 */
HWTEST(SmartParserIOM3ExceptionTest, SmartParserIOM3ExceptionTest001, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. parser log
     */
    const std::string logPath = LogUtil::SMART_PARSER_TEST_DIR +
        "/SmartParserIOM3ExceptionTest001/history_log";
    const std::string eventType = "SENSORHUBCRASH";
    auto eventInfos = SmartParser::Analysis(logPath, SMART_PARSER_PATH, eventType);

    /**
     * @tc.steps: step2. check result
     */
    ASSERT_EQ(eventInfos["LAST_FRAME"], "46a64, 46a60");
}
} // namespace HiviewDFX
} // namespace OHOS
