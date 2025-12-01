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

#ifndef HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_TRACE_COLLECTOR_IMPL_H
#define HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_TRACE_COLLECTOR_IMPL_H

#include "ffrt.h"
#include "trace_collector.h"

namespace OHOS::HiviewDFX::UCollectUtil {
using namespace UCollect;
class TraceCollectorImpl : public TraceCollector {
public:
    ~TraceCollectorImpl() override = default;

public:
    CollectResult<std::vector<std::string>> DumpTrace(TraceCaller caller) override;
    CollectResult<std::vector<std::string>> DumpTraceWithDuration(
        TraceCaller caller, uint32_t maxDuration, uint64_t happenTime) override;
    CollectResult<std::vector<std::string>> DumpTrace(const std::string& callName, bool isNeedFlowControl) override;
    CollectResult<std::string> DumpAppTrace(const UCollectClient::AppCaller& appCaller) override;
    CollectResult<std::vector<std::string>> DumpTraceWithFilter(TeleModule module,
        uint32_t maxDuration, uint64_t happenTime) override;
    CollectResult<int32_t> FilterTraceOn(TeleModule module, uint64_t postTime) override;
    CollectResult<int32_t> FilterTraceOff(TeleModule module) override;
    void PrepareTrace() override;

private:
    CollectResult<std::vector<std::string>> StartDumpTrace(TraceCaller &caller, uint32_t timeLimit,
        uint64_t happenTime = 0);
    void RecoverTmpTrace();
    void ClearInvalidLinkTrace();

    std::unique_ptr<ffrt::queue> ffrtQueue_;
    ffrt::task_handle handle_;
    std::mutex postMutex_;
    std::mutex dumpMutex_;
};
} // namespace OHOS::HiviewDFX::UCollectUtil


#endif // HIVIEW_FRAMEWORK_NATIVE_UNIFIED_COLLECTION_TRACE_COLLECTOR_IMPL_H
