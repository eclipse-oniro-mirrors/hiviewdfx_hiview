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
    virtual CollectResult<int32_t> OpenSnapshot(const std::vector<std::string>& tagGroups) override;
    virtual CollectResult<std::vector<std::string>> DumpSnapshot(TraceClient client) override;
    virtual CollectResult<int32_t> OpenRecording(const std::string& tags) override;
    virtual CollectResult<int32_t> RecordingOn() override;
    virtual CollectResult<std::vector<std::string>> RecordingOff() override;
    virtual CollectResult<int32_t> Close() override;
    virtual CollectResult<int32_t> CaptureDurationTrace(AppCaller &appCaller) override;
};

std::shared_ptr<TraceCollector> TraceCollector::Create()
{
    return std::make_shared<TraceCollectorImpl>();
}

CollectResult<int32_t> TraceCollectorImpl::OpenSnapshot(const std::vector<std::string>& tagGroups)
{
    return HiViewServiceTraceDelegate::OpenSnapshot(tagGroups);
}

CollectResult<std::vector<std::string>> TraceCollectorImpl::DumpSnapshot(TraceClient client)
{
    return HiViewServiceTraceDelegate::DumpSnapshot(client);
}

CollectResult<int32_t> TraceCollectorImpl::OpenRecording(const std::string& tags)
{
    return HiViewServiceTraceDelegate::OpenRecording(tags);
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
} // UCollectClient
} // HiViewDFX
} // OHOS
