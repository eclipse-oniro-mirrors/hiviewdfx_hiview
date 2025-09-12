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

#ifndef FAULT_LOG_EXT_CONNECTION_H
#define FAULT_LOG_EXT_CONNECTION_H

#include "ability_connect_callback_stub.h"
#include "fault_log_ext_proxy.h"

namespace OHOS {
namespace HiviewDFX {
class FaultLogExtConnection : public AAFwk::AbilityConnectionStub {
public:
    /**
     * @brief OnFault.
     */
    void OnFault();

    /**
     * @brief The OnAbilityConnectDone callback.
     *
     * @param element The element.
     * @param remoteObject The remote object.
     * @param resultCode The result code.
     */
    void OnAbilityConnectDone(
        const AppExecFwk::ElementName &element, const sptr<IRemoteObject> &remoteObject, int32_t resultCode) override;

    /**
     * @brief The OnAbilityDisconnectDone callback.
     *
     * @param element The element.
     * @param resultCode The result code.
     */
    void OnAbilityDisconnectDone(const AppExecFwk::ElementName &element, int32_t resultCode) override;
    bool IsConnected();

private:
    sptr<FaultLogExt::FaultLogExtProxy> proxy_ = nullptr;
    std::atomic<bool> isConnected_{false};
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // FAULT_LOG_EXT_CONNECTION_H
