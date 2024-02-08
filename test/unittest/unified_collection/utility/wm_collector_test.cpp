/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include <iostream>

#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "wm_collector.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace OHOS::HiviewDFX::UCollect;
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
        "ohos.permission.DUMP",
    };
    NativeTokenGet(perms, 1); // 1 is the size of the array which consists of required permissions.
}

void DisablePermissionAccess()
{
    NativeTokenGet(nullptr, 0); // empty permission array.
}
}

class WmCollectorTest : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};
    static void SetUpTestCase() {};
    static void TearDownTestCase() {};
};

/**
 * @tc.name: WmCollectorTest001
 * @tc.desc: used to test WmCollector.ExportWindowsInfo
 * @tc.type: FUNC
*/
HWTEST_F(WmCollectorTest, WmCollectorTest001, TestSize.Level1)
{
    EnablePermissionAccess();
    std::shared_ptr<WmCollector> collector = WmCollector::Create();
    auto result = collector->ExportWindowsInfo();
    std::cout << "export windows info result " << result.retCode << std::endl;
    ASSERT_TRUE(result.retCode == UcError::SUCCESS);
    DisablePermissionAccess();
}

/**
 * @tc.name: WmCollectorTest002
 * @tc.desc: used to test WmCollector.ExportWindowsMemory
 * @tc.type: FUNC
*/
HWTEST_F(WmCollectorTest, WmCollectorTest002, TestSize.Level1)
{
    EnablePermissionAccess();
    std::shared_ptr<WmCollector> collector = WmCollector::Create();
    auto result = collector->ExportWindowsMemory();
    std::cout << "export windows memory result " << result.retCode << std::endl;
    ASSERT_TRUE(result.retCode == UcError::SUCCESS);
    DisablePermissionAccess();
}
