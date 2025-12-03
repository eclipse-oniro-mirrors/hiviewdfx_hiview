/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include "hiview_service_trace_delegate.h"

#include "app_caller_parcelable.h"
#include "hiview_service_ability_proxy.h"

namespace OHOS {
namespace HiviewDFX {

CollectResult<std::vector<std::string>> HiViewServiceTraceDelegate::DumpSnapshot(int32_t client)
{
    auto proxyHandler = [client] (
        const sptr<IRemoteObject>& remote, CollectResult<std::vector<std::string>>& collectResult, int32_t& errNo) {
        return HiviewServiceAbilityProxy(remote).DumpSnapshotTrace(client, errNo, collectResult.data);
    };
    return TraceCalling<std::vector<std::string>>(proxyHandler);
}

CollectResult<int32_t> HiViewServiceTraceDelegate::OpenTrace(const std::vector<std::string>& tags,
    const UCollectClient::TraceParam& param, const std::vector<int32_t>& filterPids)
{
    auto proxyHandler = [&tags, &param, &filterPids] (
    const sptr<IRemoteObject>& remote, CollectResult<int32_t>& collectResult, int32_t& error) {
        return HiviewServiceAbilityProxy(remote).OpenTrace(tags, param, filterPids, error, collectResult.data);
    };
    return TraceCalling<int32_t>(proxyHandler);
}

CollectResult<int32_t> HiViewServiceTraceDelegate::RecordingOn()
{
    auto proxyHandler = [] (
        const sptr<IRemoteObject>& remote, CollectResult<int32_t>& collectResult, int32_t& errNo) {
        return HiviewServiceAbilityProxy(remote).RecordingTraceOn(errNo, collectResult.data);
    };
    return TraceCalling<int32_t>(proxyHandler);
}

CollectResult<std::vector<std::string>> HiViewServiceTraceDelegate::RecordingOff()
{
    auto proxyHandler = [] (
        const sptr<IRemoteObject>& remote, CollectResult<std::vector<std::string>>& collectResult, int32_t& errNo) {
        return HiviewServiceAbilityProxy(remote).RecordingTraceOff(errNo, collectResult.data);
    };
    return TraceCalling<std::vector<std::string>>(proxyHandler);
}

CollectResult<int32_t> HiViewServiceTraceDelegate::Close()
{
    auto proxyHandler = [] (
        const sptr<IRemoteObject>& remote, CollectResult<int32_t>& collectResult, int32_t& errNo) {
        return HiviewServiceAbilityProxy(remote).CloseTrace(errNo, collectResult.data);
    };
    return TraceCalling<int32_t>(proxyHandler);
}

CollectResult<int32_t> HiViewServiceTraceDelegate::CaptureDurationTrace(UCollectClient::AppCaller &appCaller)
{
    AppCallerParcelable appCallerParcelable(appCaller);
    auto proxyHandler = [&appCallerParcelable] (
        const sptr<IRemoteObject>& remote, CollectResult<int32_t>& collectResult, int32_t& errNo) {
        return HiviewServiceAbilityProxy(remote).CaptureDurationTrace(appCallerParcelable, errNo, collectResult.data);
    };
    return TraceCalling<int32_t>(proxyHandler);
}
} // namespace HiviewDFX
} // namespace OHOS
