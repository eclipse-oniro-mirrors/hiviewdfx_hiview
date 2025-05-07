/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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
#include "event_server_test.h"

#include "event_server.h"
#include "file_util.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;

namespace {
const std::string BBOX_PATH = "/dev/bbox";
}

void EventServerTest::SetUp()
{
    printf("EventServerTest SetUp\n");
    platform.GetPluginMap();
}

void EventServerTest::TearDown()
{
    printf("EventServerTest TearDown\n");
}

/**
 * @tc.name: EventServerTest001
 * @tc.desc: BBoxDevice test.
 * @tc.type: FUNC
 * @tc.require: issueI5NULM
 */
HWTEST_F(EventServerTest, EventServerTest001, TestSize.Level3)
{
    BBoxDevice bbox;
    ASSERT_TRUE(bbox.GetEvents() > 0);
    ASSERT_NE(bbox.GetName(), "");
    int res = bbox.Open();
    ASSERT_TRUE(res >= 0);
    res = bbox.Close();
    ASSERT_TRUE(res >= 0);
}
