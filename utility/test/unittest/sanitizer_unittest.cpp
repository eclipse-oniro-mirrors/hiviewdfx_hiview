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
 * @tc.name: SmartParserSanitizerTest001
 * @tc.desc: Smart parser non-encrypted sanitizer log.
 * @tc.type: FUNC
 */
HWTEST(SmartParserSanitizerTest, SmartParserSanitizerTest001, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. parser log
     */
    const std::string logPath = std::string{LogUtil::SMART_PARSER_TEST_DIR}  +
        "/SmartParserSanitizerTest001/sanitizer-12345-12345678910111234";
    const std::string eventType = "ADDR_SANITIZER";
    auto eventInfos = SmartParser::Analysis(logPath, SMART_PARSER_PATH, eventType);

    /**
     * @tc.steps: step2. check result
     */
    std::string endStack = "/data/storage/el1/bundle/libs/arm64/libentry.so+0x54174\n";
    endStack += "/system/lib64/platformsdk/libace_napi.z.so+0x73b24\n";
    endStack += "/system/lib64/module/arkcompiler/stub.an+0xe91600\n";
    endStack += "/system/lib64/module/arkcompiler/stub.an+0xed36dc8\n";

    ASSERT_EQ(eventInfos["END_STACK"], endStack);

    Tbox::FilterTrace(eventInfos, eventType);
    ASSERT_EQ(eventInfos["Reason"], "stack-buffer-overflow");
    ASSERT_EQ(eventInfos["FIRST_FRAME"], "/data/storage/el1/bundle/libs/arm64/libentry.so+0x54174");
    ASSERT_EQ(eventInfos["SECOND_FRAME"], "/system/lib64/platformsdk/libace_napi.z.so+0x73b24");
    ASSERT_EQ(eventInfos["LAST_FRAME"], "/system/lib64/module/arkcompiler/stub.an+0xed36dc8");
}

/**
 * @tc.name: SmartParserSanitizerTest002
 * @tc.desc: Smart parser non-encrypted sanitizer log.
 * @tc.type: FUNC
 */
HWTEST(SmartParserSanitizerTest, SmartParserSanitizerTest002, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. parser log
     */
    const std::string logPath = std::string{LogUtil::SMART_PARSER_TEST_DIR}  +
        "/SmartParserSanitizerTest002/sanitizer-12345-12345678910111234";
    const std::string eventType = "ADDR_SANITIZER";
    auto eventInfos = SmartParser::Analysis(logPath, SMART_PARSER_PATH, eventType);

    /**
     * @tc.steps: step2. check result
     */
    std::string endStack = "/data/storage/el1/bundle/libs/arm64/libentry.so+0x54174\n";
    endStack += "/system/lib64/platformsdk/libace_napi.z.so+0x73b24\n";
    endStack += "/system/lib64/module/arkcompiler/stub.an+0xe91600\n";
    endStack += "/system/lib64/module/arkcompiler/stub.an+0xed36dc8\n";

    ASSERT_EQ(eventInfos["END_STACK"], endStack);

    Tbox::FilterTrace(eventInfos, eventType);
    ASSERT_EQ(eventInfos["Reason"], "use-after-free");
    ASSERT_EQ(eventInfos["FIRST_FRAME"], "/data/storage/el1/bundle/libs/arm64/libentry.so+0x54174");
    ASSERT_EQ(eventInfos["SECOND_FRAME"], "/system/lib64/platformsdk/libace_napi.z.so+0x73b24");
    ASSERT_EQ(eventInfos["LAST_FRAME"], "/system/lib64/module/arkcompiler/stub.an+0xed36dc8");
}

/**
 * @tc.name: SmartParserSanitizerTest003
 * @tc.desc: Smart parser non-encrypted sanitizer log.
 * @tc.type: FUNC
 */
HWTEST(SmartParserSanitizerTest, SmartParserSanitizerTest003, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. parser log
     */
    const std::string logPath = std::string{LogUtil::SMART_PARSER_TEST_DIR}  +
        "/SmartParserSanitizerTest003/sanitizer-12345-12345678910111234";
    const std::string eventType = "ADDR_SANITIZER";
    auto eventInfos = SmartParser::Analysis(logPath, SMART_PARSER_PATH, eventType);

    /**
     * @tc.steps: step2. check result
     */
    std::string endStack = "/data/storage/el1/bundle/libs/arm64/libc++.so+0x54174\n";
    endStack += "/system/lib64/platformsdk/libace_napi.z.so+0x73b24\n";
    endStack += "/system/lib64/module/arkcompiler/stub.an+0xe91600\n";
    endStack += "/system/lib64/module/arkcompiler/stub.an+0xed36dc8\n";

    ASSERT_EQ(eventInfos["END_STACK"], endStack);

    Tbox::FilterTrace(eventInfos, eventType);
    ASSERT_EQ(eventInfos["Reason"], "TSAN");
    ASSERT_EQ(eventInfos["FIRST_FRAME"], "/system/lib64/platformsdk/libace_napi.z.so+0x73b24");
    ASSERT_EQ(eventInfos["SECOND_FRAME"], "/system/lib64/module/arkcompiler/stub.an+0xe91600");
    ASSERT_EQ(eventInfos["LAST_FRAME"], "/system/lib64/module/arkcompiler/stub.an+0xed36dc8");
}

/**
 * @tc.name: SmartParserSanitizerTest004
 * @tc.desc: Smart parser non-encrypted sanitizer log.
 * @tc.type: FUNC
 */
HWTEST(SmartParserSanitizerTest, SmartParserSanitizerTest004, testing::ext::TestSize.Level3)
{
    /**
     * @tc.steps: step1. parser log
     */
    const std::string logPath = std::string{LogUtil::SMART_PARSER_TEST_DIR}  +
        "/SmartParserSanitizerTest004/sanitizer-12345-12345678910111234";
    const std::string eventType = "ADDR_SANITIZER";
    auto eventInfos = SmartParser::Analysis(logPath, SMART_PARSER_PATH, eventType);

    /**
     * @tc.steps: step2. check result
     */
    std::string endStack = "/data/storage/el1/bundle/libs/arm64/libc++.so+0x54174\n";
    endStack += "/system/lib64/platformsdk/libace_napi.z.so+0x73b24\n";
    endStack += "/system/lib64/module/arkcompiler/stub.an+0xe91600\n";
    endStack += "/system/lib64/module/arkcompiler/stub.an+0xed36dc8\n";

    ASSERT_EQ(eventInfos["END_STACK"], endStack);

    Tbox::FilterTrace(eventInfos, eventType);
    ASSERT_EQ(eventInfos["Reason"], "UBSAN");
    ASSERT_EQ(eventInfos["FIRST_FRAME"], "/system/lib64/platformsdk/libace_napi.z.so+0x73b24");
    ASSERT_EQ(eventInfos["SECOND_FRAME"], "/system/lib64/module/arkcompiler/stub.an+0xe91600");
    ASSERT_EQ(eventInfos["LAST_FRAME"], "/system/lib64/module/arkcompiler/stub.an+0xed36dc8");
}
} // namespace HiviewDFX
} // namespace OHOS
