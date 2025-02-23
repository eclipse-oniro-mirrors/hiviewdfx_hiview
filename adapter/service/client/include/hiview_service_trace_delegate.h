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

#ifndef OHOS_HIVIEWDFX_ADAPTER_SERVICE_IDL_INCLUDE_TRACE_DELEGATE_H
#define OHOS_HIVIEWDFX_ADAPTER_SERVICE_IDL_INCLUDE_TRACE_DELEGATE_H

#include <vector>

#include "client/trace_collector_client.h"
#include "collect_result.h"
#include "hiview_remote_service.h"
#include "hiview_service_ability_proxy.h"
#include "parcel.h"

namespace OHOS {
namespace HiviewDFX {
class HiViewServiceTraceDelegate {
public:
    static CollectResult<int32_t> OpenSnapshot(const std::vector<std::string>& tagGroups);
    static CollectResult<std::vector<std::string>> DumpSnapshot(int32_t caller);
    static CollectResult<int32_t> OpenRecording(const std::string& tags);
    static CollectResult<int32_t> RecordingOn();
    static CollectResult<std::vector<std::string>> RecordingOff();
    static CollectResult<int32_t> Close();
    static CollectResult<int32_t> CaptureDurationTrace(UCollectClient::AppCaller &appCaller);

private:
    template<typename T>
    static CollectResult<T> TraceCalling(
        std::function<CollectResultParcelable<T>(HiviewServiceAbilityProxy&)> proxyHandler)
    {
        CollectResult<T> ret;
        if (proxyHandler == nullptr) {
            return ret;
        }
        auto service = RemoteService::GetHiViewRemoteService();
        if (service == nullptr) {
            return ret;
        }
        HiviewServiceAbilityProxy proxy(service);
        auto traceRet = proxyHandler(proxy);
        return traceRet.result_;
    }
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // OHOS_HIVIEWDFX_ADAPTER_SERVICE_IDL_INCLUDE_TRACE_DELEGATE_H
