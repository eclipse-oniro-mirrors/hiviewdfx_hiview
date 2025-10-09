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

#include "faultlog_ext_connection.h"

#include <unistd.h>

#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002D11, "FaultloggerExt-Con");

void FaultLogExtConnection::OnFault()
{
    HIVIEW_LOGI("begin OnFault.");
    if (proxy_ == nullptr) {
        HIVIEW_LOGE("proxy is nullptr");
        return;
    }
    proxy_->OnFaultReportReady();
}

void FaultLogExtConnection::OnAbilityConnectDone(
    const AppExecFwk::ElementName& element, const sptr<IRemoteObject>& remoteObject, int32_t resultCode)
{
    HIVIEW_LOGI("On ability connect.");
    proxy_ = (new (std::nothrow) FaultLogExt::FaultLogExtProxy(remoteObject));
    if (proxy_ == nullptr) {
        HIVIEW_LOGE("proxy is nullptr");
        return;
    }
    isConnected_.store(true);
    OnFault();
}

void FaultLogExtConnection::OnAbilityDisconnectDone(const AppExecFwk::ElementName& element, int32_t resultCode)
{
    HIVIEW_LOGI("On ability disconnect.");
    isConnected_.store(false);
}

bool FaultLogExtConnection::IsConnected()
{
    return isConnected_.load();
}
} // namespace HiviewDFX
} // namespace OHOS
