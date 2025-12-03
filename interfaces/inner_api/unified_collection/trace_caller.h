/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#ifndef INTERFACES_INNER_API_UNIFIED_COLLECTION_TRACE_CALLER_H
#define INTERFACES_INNER_API_UNIFIED_COLLECTION_TRACE_CALLER_H

namespace OHOS {
namespace HiviewDFX {
namespace UCollectClient {
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
    bool isBusinessJank = false;       // is business jank or not, control output file name
};
}

namespace UCollect {

// Used for utility interface, for hiview inner only
enum TraceCaller {
    RELIABILITY,
    XPERF,
    XPERF_EX,
    XPOWER,
    HIVIEW,
    SCREEN
};

// Used for ipc client interface, for outside
enum TraceClient {
    COMMAND,
    COMMON_DEV,
};

// Used for telemetry interface param
enum class TeleModule {
    XPOWER,
    XPERF,
    RELIABILITY
};
}
} // HiviewDFX
} // OHOS
#endif // INTERFACES_INNER_API_UNIFIED_COLLECTION_TRACE_CALLER_H