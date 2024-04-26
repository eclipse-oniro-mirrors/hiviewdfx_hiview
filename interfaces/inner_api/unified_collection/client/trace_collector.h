/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

namespace OHOS {
namespace HiviewDFX {
namespace UCollectClient {
constexpr int32_t ACTION_ID_START_TRACE = 1;
constexpr int32_t ACTION_ID_DUMP_TRACE = 2;

struct AppCaller {
    int32_t actionId;          // 1: start trace; 2: dump trace
    std::string bundleName;    // app bundle name
    std::string bundleVersion; // app bundle version
    std::string threadName;    // app thread name
    int32_t foreground;        // app foreground
    int32_t uid;               // app user id
    int32_t pid;               // app process id
    int64_t happenTime;  // jank happend time, millisecond unit
    int64_t beginTime;   // message handle begin time, millisecond unit
    int64_t endTime;     // message handle end time, millisecond unit
};

class TraceCollector {
public:
    TraceCollector() = default;
    virtual ~TraceCollector() = default;
    enum Caller {
        RELIABILITY,
        XPERF,
        XPOWER,
        BETACLUB,
        DEVELOP,
        OTHER,
    };

public:
    virtual CollectResult<int32_t> OpenSnapshot(const std::vector<std::string>& tagGroups) = 0;
    virtual CollectResult<std::vector<std::string>> DumpSnapshot(Caller caller = Caller::OTHER) = 0;
    virtual CollectResult<int32_t> OpenRecording(const std::string& tags) = 0;
    virtual CollectResult<int32_t> RecordingOn() = 0;
    virtual CollectResult<std::vector<std::string>> RecordingOff() = 0;
    virtual CollectResult<int32_t> Close() = 0;
    virtual CollectResult<int32_t> Recover() = 0;
    // use for hap main looper
    virtual CollectResult<int32_t> CaptureDurationTrace(AppCaller &appCaller) = 0;

    static std::shared_ptr<TraceCollector> Create();
}; // TraceCollector
} // UCollectClient
} // HiviewDFX
} // OHOS
#endif // INTERFACES_INNER_API_UNIFIED_COLLECTION_CLIENT_TRACE_COLLECTOR_H
