/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "xpower_event_test.h"

#include <iostream>
#include "xpower_event.h"
#include "xpower_event_common.h"
#include <parameters.h>

using namespace testing::ext;
using namespace OHOS::HiviewDFX;

void ReportXPowerEventTest::SetUp()
{
    /**
     * @tc.setup: create work directories
     */
    printf("SetUp.\n");
}
void ReportXPowerEventTest::SetUpTestCase()
{
    printf("SetUpTestCase.\n");
}

void ReportXPowerEventTest::TearDownTestCase()
{
    printf("TearDownTestCase.\n");
}

void ReportXPowerEventTest::TearDown()
{
    printf("TearDown.\n");
}

/**
 * @tc.name: ReportXPowerEventTest001
 * @tc.desc: used to test ReportXPowerStackSysEvent
 * @tc.type: FUNC
 */
HWTEST_F(ReportXPowerEventTest, ReportXPowerEventTest001, TestSize.Level0)
{
    printf("disable parameter and test.\n");
    bool succ = OHOS::system::SetParameter(PROP_XPOWER_OPTIMIZE_ENABLE, "0");
    ASSERT_TRUE(succ);
    sleep(1);
    int param = OHOS::system::GetIntParameter(PROP_XPOWER_OPTIMIZE_ENABLE, 0);
    ASSERT_EQ(param, 0);
    int ret = ReportXPowerStackSysEvent("XPOWER_HIVIEW_API_TEST", "info=1,succ=true");
    ASSERT_EQ(ret, ERR_PROP_NOT_ENABLE);

    printf("enable parameter and test.\n");
    succ = OHOS::system::SetParameter(PROP_XPOWER_OPTIMIZE_ENABLE, "1");
    ASSERT_TRUE(succ);
    sleep(1);
    param = OHOS::system::GetIntParameter(PROP_XPOWER_OPTIMIZE_ENABLE, 0);
    ASSERT_EQ(param, 1);
    ret = ReportXPowerStackSysEvent("XPOWER_HIVIEW_API_TEST", "info=2,succ=true");
    ASSERT_EQ(ret, ERR_SUCCESS);

    printf("enable parameter and test default info.\n");
    ret = ReportXPowerStackSysEvent("XPOWER_HIVIEW_API_TEST");
    ASSERT_EQ(ret, ERR_SUCCESS);
}
