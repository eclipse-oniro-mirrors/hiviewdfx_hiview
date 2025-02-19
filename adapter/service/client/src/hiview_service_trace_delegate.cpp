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

#include "hiview_service_trace_delegate.h"

#include "hiview_err_code.h"
#include "hiview_logger.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace HiviewDFX {
CollectResult<int32_t> HiViewServiceTraceDelegate::OpenSnapshot(const std::vector<std::string>& tagGroups)
{
    auto proxyHandler = [&tagGroups] (HiviewServiceAbilityProxy& proxy) {
        return proxy.OpenSnapshotTrace(tagGroups);
    };
    return TraceCalling<int32_t>(proxyHandler);
}

CollectResult<std::vector<std::string>> HiViewServiceTraceDelegate::DumpSnapshot(int32_t caller)
{
    auto proxyHandler = [caller] (HiviewServiceAbilityProxy& proxy) {
        return proxy.DumpSnapshotTrace(caller);
    };
    return TraceCalling<std::vector<std::string>>(proxyHandler);
}

CollectResult<int32_t> HiViewServiceTraceDelegate::OpenRecording(const std::string& tags)
{
    auto proxyHandler = [&tags] (HiviewServiceAbilityProxy& proxy) {
        return proxy.OpenRecordingTrace(tags);
    };
    return TraceCalling<int32_t>(proxyHandler);
}

CollectResult<int32_t> HiViewServiceTraceDelegate::RecordingOn()
{
    auto proxyHandler = [] (HiviewServiceAbilityProxy& proxy) {
        return proxy.RecordingTraceOn();
    };
    return TraceCalling<int32_t>(proxyHandler);
}

CollectResult<std::vector<std::string>> HiViewServiceTraceDelegate::RecordingOff()
{
    auto proxyHandler = [] (HiviewServiceAbilityProxy& proxy) {
        return proxy.RecordingTraceOff();
    };
    return TraceCalling<std::vector<std::string>>(proxyHandler);
}

CollectResult<int32_t> HiViewServiceTraceDelegate::Close()
{
    auto proxyHandler = [] (HiviewServiceAbilityProxy& proxy) {
        return proxy.CloseTrace();
    };
    return TraceCalling<int32_t>(proxyHandler);
}

CollectResult<int32_t> HiViewServiceTraceDelegate::CaptureDurationTrace(UCollectClient::AppCaller &appCaller)
{
    auto proxyHandler = [&appCaller] (HiviewServiceAbilityProxy& proxy) {
        return proxy.CaptureDurationTrace(appCaller);
    };
    return TraceCalling<int32_t>(proxyHandler);
}
} // namespace HiviewDFX
} // namespace OHOS
