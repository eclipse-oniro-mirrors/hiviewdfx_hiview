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
namespace UCollect {

// Used for utility interface, for hiview inner only
enum TraceCaller {
    RELIABILITY,
    XPERF,
    XPOWER,
    BETACLUB,
    OTHER,
    HIVIEW
};

// Used for ipc client interface, for outside
enum TraceClient {
    COMMAND,
    COMMON_DEV,
    APP
};

// Used for telemetry interface param
enum class TeleModule {
    XPOWER,
    XPERF,
};
}
} // HiviewDFX
} // OHOS
#endif // INTERFACES_INNER_API_UNIFIED_COLLECTION_TRACE_CALLER_H