/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "hiview_sa_test.h"

#include <string>
#include <unistd.h>

#include "common_utils.h"
#include "hiview_service_ability_proxy.h"
#include "if_system_ability_manager.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace HiviewDFX {
void HiviewSATest::SetUpTestCase() {}

void HiviewSATest::TearDownTestCase() {}

void HiviewSATest::SetUp() {}

void HiviewSATest::TearDown() {}

sptr<IRemoteObject> GetHiviewRemoteObject()
{
    sptr<ISystemAbilityManager> serviceManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (serviceManager == nullptr) {
        printf("serviceManager == nullptr");
        return nullptr;
    }
    sptr<IRemoteObject> abilityObject = serviceManager->CheckSystemAbility(DFX_SYS_HIVIEW_ABILITY_ID);
    if (abilityObject == nullptr) {
        printf("abilityObjext == nullptr");
        return nullptr;
    }
    return abilityObject;
}

/**
 * @tc.name: CommonTest001
 * @tc.desc: Check whether the SA is successfully obtained.
 * @tc.type: FUNC
 * @tc.require: AR000FJLO2
 */
HWTEST_F(HiviewSATest, CommonTest001, testing::ext::TestSize.Level3)
{
    sptr<IRemoteObject> abilityObjext = GetHiviewRemoteObject();
    if (abilityObjext == nullptr) {
        printf("CheckSystemAbility error \r\n");
        assert(false);
    }
    auto hiviewSAProxy = new HiviewServiceAbilityProxy(abilityObjext);
    if (hiviewSAProxy == nullptr) {
        printf("hiviewSAProxy == nullptr");
        ASSERT_NE(hiviewSAProxy, nullptr);
    }

    printf("end \r\n");
}

/**
 * @tc.name: CommonTest002
 * @tc.desc: Check hidumper -s 1201.
 * @tc.type: FUNC
 * @tc.require: AR000FJLO2
 */
HWTEST_F(HiviewSATest, CommonTest002, testing::ext::TestSize.Level3)
{
    printf("system(\"hidumper -s 1201\") \r\n");
    char buffer[256];
    FILE* fp = popen("hidumper -s 1201", "r");

    ASSERT_NE(fp, nullptr);
    fgets(buffer, sizeof(buffer), fp);
    printf("%s", buffer);
    pclose(fp);
    std::string str(buffer);
    if (str.find("Error") != std::string::npos) {
        printf("hidumper -s 1201 fail!\r\n");
        FAIL();
    }
}

/**
 * @tc.name: CommonTest003
 * @tc.desc: Check GetGraphicUsage
 * @tc.type: FUNC
 */
HWTEST_F(HiviewSATest, CommonTest003, testing::ext::TestSize.Level3)
{
    sptr<IRemoteObject> abilityObjext = GetHiviewRemoteObject();
    if (abilityObjext == nullptr) {
        printf("CheckSystemAbility error \r\n");
        return;
    }
    auto hiviewSAProxy = new HiviewServiceAbilityProxy(abilityObjext);
    auto systemuiPid = CommonUtils::GetPidByName("com.ohos.systemui");
    auto launcherPid = CommonUtils::GetPidByName("com.ohos.sceneboard");
    auto pid = static_cast<int32_t>(systemuiPid > 0 ? systemuiPid : launcherPid);
    if (pid <= 0) {
        std::cout << "Get pid failed" << std::endl;
        return;
    }
    CollectResult<int32_t> data = hiviewSAProxy->GetGraphicUsage().result_;
    ASSERT_EQ(data.retCode, UCollect::UcError::SUCCESS);
    std::cout << "GetGraphicUsage result:" << data.data << std::endl;
    ASSERT_GE(data.data, 0);
}
}
}