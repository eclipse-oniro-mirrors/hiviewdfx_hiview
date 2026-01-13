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

#include "collect_device_client.h"
#include "common_utils.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;

class CollectDeviceClientTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

int32_t GetTestPid()
{
    auto systemuiPid = CommonUtils::GetPidByName("com.ohos.systemui");
    auto launcherPid = CommonUtils::GetPidByName("com.ohos.sceneboard");
    auto pid = static_cast<int32_t>(systemuiPid > 0 ? systemuiPid : launcherPid);
    return pid;
}

/**
 * @tc.name: CollectDeviceClientTest001
 * @tc.desc: used to test func in CollectDeviceClient
 * @tc.type: FUNC
*/
HWTEST_F(CollectDeviceClientTest, CollectDeviceClientTest001, TestSize.Level1)
{
    auto deviceClient = std::make_shared<CollectDeviceClient>();
    int ret = deviceClient->Open();
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: CollectDeviceClientTest002
 * @tc.desc: used to test func in CollectDeviceClient
 * @tc.type: FUNC
*/
HWTEST_F(CollectDeviceClientTest, CollectDeviceClientTest002, TestSize.Level1)
{
    auto deviceClient = std::make_shared<CollectDeviceClient>();
    (void)deviceClient->Open();
    std::shared_ptr<ProcessCpuData> data = deviceClient->FetchProcessCpuData();
    auto item = data->GetNextProcess();
    ASSERT_NE(item, nullptr);
}

/**
 * @tc.name: CollectDeviceClientTest003
 * @tc.desc: used to test func in CollectDeviceClient
 * @tc.type: FUNC
*/
HWTEST_F(CollectDeviceClientTest, CollectDeviceClientTest003, TestSize.Level1)
{
    auto deviceClient = std::make_shared<CollectDeviceClient>();
    (void)deviceClient->Open();
    std::shared_ptr<ProcessCpuData> data = deviceClient->FetchProcessCpuData(GetTestPid());
    auto item = data->GetNextProcess();
    ASSERT_NE(item, nullptr);
}

/**
 * @tc.name: CollectDeviceClientTest004
 * @tc.desc: used to test func in CollectDeviceClient
 * @tc.type: FUNC
*/
HWTEST_F(CollectDeviceClientTest, CollectDeviceClientTest004, TestSize.Level1)
{
    auto deviceClient = std::make_shared<CollectDeviceClient>();
    (void)deviceClient->Open();
    std::shared_ptr<ThreadCpuData> data = deviceClient->FetchThreadCpuData(GetTestPid());
    auto item = data->GetNextThread();
    ASSERT_NE(item, nullptr);
}

/**
 * @tc.name: CollectDeviceClientTest005
 * @tc.desc: used to test func in CollectDeviceClient
 * @tc.type: FUNC
*/
HWTEST_F(CollectDeviceClientTest, CollectDeviceClientTest005, TestSize.Level1)
{
    auto deviceClient = std::make_shared<CollectDeviceClient>();
    (void)deviceClient->Open();
    std::shared_ptr<ThreadCpuData> data = deviceClient->FetchSelfThreadCpuData(getpid());
    auto item = data->GetNextThread();
    ASSERT_NE(item, nullptr);
}
