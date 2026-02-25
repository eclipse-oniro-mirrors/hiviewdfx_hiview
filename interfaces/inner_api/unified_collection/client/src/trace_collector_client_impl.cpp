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
#include "trace_collector_client.h"

#include "hiview_service_trace_delegate.h"

using namespace OHOS::HiviewDFX::UCollect;

namespace OHOS {
namespace HiviewDFX {
namespace UCollectClient {
class TraceCollectorImpl : public TraceCollector {
public:
    TraceCollectorImpl() = default;
    virtual ~TraceCollectorImpl() = default;

public:
    CollectResult<int32_t> OpenTrace(const std::vector<std::string>& tags, const TraceParam& param,
        const std::vector<int32_t>& filterPids = {}) override;
    CollectResult<std::vector<std::string>> DumpSnapshot(TraceClient client) override;
    CollectResult<int32_t> RecordingOn() override;
    CollectResult<std::vector<std::string>> RecordingOff() override;
    CollectResult<int32_t> Close() override;
    CollectResult<int32_t> CaptureDurationTrace(AppCaller &appCaller) override;
    void RequestAppTrace(const TraceConfig& traceConfig, std::shared_ptr<RequestTraceCallBack> callback) override;
};

std::shared_ptr<TraceCollector> TraceCollector::Create()
{
    return std::make_shared<TraceCollectorImpl>();
}

CollectResult<int32_t> TraceCollectorImpl::OpenTrace(const std::vector<std::string> &tags, const TraceParam &param,
    const std::vector<int32_t> &filterPids)
{
    return HiViewServiceTraceDelegate::OpenTrace(tags, param, filterPids);
}

CollectResult<std::vector<std::string>> TraceCollectorImpl::DumpSnapshot(TraceClient client)
{
    return HiViewServiceTraceDelegate::DumpSnapshot(client);
}

CollectResult<int32_t> TraceCollectorImpl::RecordingOn()
{
    return HiViewServiceTraceDelegate::RecordingOn();
}

CollectResult<std::vector<std::string>> TraceCollectorImpl::RecordingOff()
{
    return HiViewServiceTraceDelegate::RecordingOff();
}

CollectResult<int32_t> TraceCollectorImpl::Close()
{
    return HiViewServiceTraceDelegate::Close();
}

CollectResult<int32_t> TraceCollectorImpl::CaptureDurationTrace(AppCaller &appCaller)
{
    return HiViewServiceTraceDelegate::CaptureDurationTrace(appCaller);
}

void TraceCollectorImpl::RequestAppTrace(const TraceConfig &traceConfig, std::shared_ptr<RequestTraceCallBack> callback)
{
    return HiViewServiceTraceDelegate::RequestAppTrace(traceConfig, callback);
}
} // UCollectClient
} // HiViewDFX
} // OHOS
