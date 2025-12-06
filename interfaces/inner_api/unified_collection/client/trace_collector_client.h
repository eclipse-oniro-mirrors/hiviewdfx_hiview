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
#ifndef INTERFACES_INNER_API_UNIFIED_COLLECTION_CLIENT_TRACE_COLLECTOR_H
#define INTERFACES_INNER_API_UNIFIED_COLLECTION_CLIENT_TRACE_COLLECTOR_H
#include <cinttypes>
#include <memory>
#include <string>
#include <vector>

#include "collect_result.h"
#include "trace_caller.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectClient {
constexpr int32_t ACTION_ID_START_TRACE = 1;
constexpr int32_t ACTION_ID_DUMP_TRACE = 2;

struct TraceParam {
    uint32_t bufferSize = 0;
    std::string clockType;
    bool isOverWrite = true;
    uint32_t fileSizeLimit = 0;
};

class TraceCollector {
public:
    TraceCollector() = default;
    virtual ~TraceCollector() = default;

public:
    virtual CollectResult<std::vector<std::string>> DumpSnapshot(
        UCollect::TraceClient client = UCollect::TraceClient::COMMON_DEV) = 0;
    virtual CollectResult<int32_t> OpenTrace(const std::vector<std::string>& tags, const TraceParam& param,
        const std::vector<int32_t>& filterPids = {}) = 0;
    virtual CollectResult<int32_t> RecordingOn() = 0;
    virtual CollectResult<std::vector<std::string>> RecordingOff() = 0;
    virtual CollectResult<int32_t> Close() = 0;
    // use for hap main looper
    virtual CollectResult<int32_t> CaptureDurationTrace(AppCaller &appCaller) = 0;

    static std::shared_ptr<TraceCollector> Create();
}; // TraceCollector
} // UCollectClient
} // HiviewDFX
} // OHOS
#endif // INTERFACES_INNER_API_UNIFIED_COLLECTION_CLIENT_TRACE_COLLECTOR_H
