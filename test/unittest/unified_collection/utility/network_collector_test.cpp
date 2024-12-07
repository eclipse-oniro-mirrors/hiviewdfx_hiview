/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
#ifdef UNIFIED_COLLECTOR_NETWORK_ENABLE
#include <iostream>

#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#endif

#include "network_collector.h"

#ifdef UNIFIED_COLLECTOR_NETWORK_ENABLE
#include "token_setproc.h"
#ifdef COMMUNICATION_WIFI_ENABLE
#include "wifi_device.h"
#endif
#endif

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace OHOS::HiviewDFX::UCollect;

#ifdef UNIFIED_COLLECTOR_NETWORK_ENABLE
namespace {
void NativeTokenGet(const char* perms[], int size)
{
    uint64_t tokenId;
    NativeTokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = size,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .aplStr = "system_basic",
    };

    infoInstance.processName = "UCollectionUtilityUnitTest";
    tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
}

void EnablePermissionAccess()
{
    const char* perms[] = {
        "ohos.permission.GET_WIFI_INFO",
    };
    NativeTokenGet(perms, 1); // 1 is the size of the array which consists of required permissions.
}

void DisablePermissionAccess()
{
    NativeTokenGet(nullptr, 0); // empty permission array.
}

bool IsWifiEnabled()
{
#ifdef COMMUNICATION_WIFI_ENABLE
    std::shared_ptr<OHOS::Wifi::WifiDevice> wifiDevicePtr =
        OHOS::Wifi::WifiDevice::GetInstance(OHOS::WIFI_DEVICE_SYS_ABILITY_ID);
    if (wifiDevicePtr == nullptr) {
        return false;
    }
    bool isActive = false;
    wifiDevicePtr->IsWifiActive(isActive);
    if (!isActive) {
        return false;
    }
    OHOS::Wifi::WifiLinkedInfo linkInfo;
    int ret = wifiDevicePtr->GetLinkedInfo(linkInfo);
    return ret == OHOS::Wifi::WIFI_OPT_SUCCESS;
#else
    return false;
#endif
}
}
#endif

class NetworkCollectorTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

#ifdef UNIFIED_COLLECTOR_NETWORK_ENABLE
/**
 * @tc.name: NetworkCollectorTest001
 * @tc.desc: used to test NetworkCollector.CollectRate
 * @tc.type: FUNC
*/
HWTEST_F(NetworkCollectorTest, NetworkCollectorTest001, TestSize.Level1)
{
    EnablePermissionAccess();
    std::shared_ptr<NetworkCollector> collector = NetworkCollector::Create();
    CollectResult<NetworkRate> data = collector->CollectRate();
    std::cout << "collect network rate result" << data.retCode << std::endl;
    if (IsWifiEnabled()) {
        ASSERT_TRUE(data.retCode == UcError::SUCCESS);
    }
    DisablePermissionAccess();
}
#else
/**
 * @tc.name: NetworkCollectorTest001
 * @tc.desc: used to test empty NetworkCollector
 * @tc.type: FUNC
*/
HWTEST_F(NetworkCollectorTest, NetworkCollectorTest001, TestSize.Level1)
{
    std::shared_ptr<NetworkCollector> collector = NetworkCollector::Create();
    CollectResult<NetworkRate> data = collector->CollectRate();
    ASSERT_TRUE(data.retCode == UcError::FEATURE_CLOSED);
}
#endif
