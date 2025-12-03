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
#include "hiview_err_code.h"
#include "hiview_remote_service.h"
#include "iremote_proxy.h"

namespace OHOS {
namespace HiviewDFX {
class HiViewServiceTraceDelegate {
public:
    static CollectResult<std::vector<std::string>> DumpSnapshot(int32_t client);
    static CollectResult<int32_t> OpenTrace(const std::vector<std::string>& tags,
        const UCollectClient::TraceParam& param, const std::vector<int32_t>& filterPids);
    static CollectResult<int32_t> RecordingOn();
    static CollectResult<std::vector<std::string>> RecordingOff();
    static CollectResult<int32_t> Close();
    static CollectResult<int32_t> CaptureDurationTrace(UCollectClient::AppCaller &appCaller);

private:
    template<typename T>
    static CollectResult<T> TraceCalling(
        std::function<int32_t(const sptr<IRemoteObject>&, CollectResult<T>&, int32_t&)> proxyHandler)
    {
        CollectResult<T> collectResult;
        auto service = RemoteService::GetHiViewRemoteService();
        if (service == nullptr) {
            return collectResult;
        }
        int32_t errNo = UCollect::UcError::UNSUPPORT;
        int32_t ret = proxyHandler(service, collectResult, errNo);
        if (ret == 0) {
            collectResult.retCode = static_cast<UCollect::UcError>(errNo);
            return collectResult;
        }
        if (ret == TraceErrCode::ERR_PERMISSION_CHECK) {
            collectResult.retCode = UCollect::UcError::PERMISSION_CHECK_FAILED;
        }
        return collectResult;
    }
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // OHOS_HIVIEWDFX_ADAPTER_SERVICE_IDL_INCLUDE_TRACE_DELEGATE_H
