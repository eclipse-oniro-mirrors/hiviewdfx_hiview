/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef HIVIEWDFX_HIVIEW_TRACE_UTIL_H
#define HIVIEWDFX_HIVIEW_TRACE_UTIL_H

#include <cinttypes>
#include <string>
#include <vector>

#include "hitrace_dump.h"

namespace OHOS {
namespace HiviewDFX {
using namespace Hitrace;

namespace FlowControlName {
inline constexpr char XPERF[] = "Xperf";
inline constexpr char XPOWER[] = "Xpower";
inline constexpr char RELIABILITY[] = "Reliability";
inline constexpr char HIVIEW[] = "Hiview";
inline constexpr char OTHER[] = "Other";
inline constexpr char APP[] = "APP";
inline constexpr char BEHAVIOR[] = "behavior";
inline constexpr char TELEMETRY[] = "Telemetry";
}

// used for log
namespace CallerName {
inline constexpr char XPERF[] = "Xperf";
inline constexpr char XPERF_EX[] = "XperfEx";
inline constexpr char XPOWER[] = "Xpower";
inline constexpr char RELIABILITY[] = "Reliability";
inline constexpr char HIVIEW[] = "Hiview";
inline constexpr char OTHER[] = "Other";
inline constexpr char SCREEN[] = "Screen";
inline constexpr char COMMAND[] = "traceCommand";
};

namespace PrefixName {
inline constexpr char APP[] = "APP";
inline constexpr char XPERF[] = "Xperf";
inline constexpr char XPOWER[] = "Xpower";
inline constexpr char RELIABILITY[] = "Reliability";
inline constexpr char HIVIEW[] = "Hiview";
inline constexpr char OTHER[] = "Other";
inline constexpr char SCREEN[] = "Screen";
};

enum class TraceScenario : uint8_t {
    TRACE_COMMAND,
    TRACE_COMMON,
    TRACE_DYNAMIC,
    TRACE_TELEMETRY,
};

struct DumpTraceArgs {
    TraceScenario scenario;
    uint32_t maxDuration = 0;
    uint64_t happenTime = 0;
};

using DumpTraceCallback = std::function<void(TraceRetInfo)>;

enum class TraceStateCode : uint8_t {
    SUCCESS,
    DENY, // Change state deny
    FAIL, // Invoke dump or drop interface in wrong state
    POLICY_ERROR,
    UPDATE_TIME,
    NO_TRIGGER
};

enum class TraceFlowCode : uint8_t {
    TRACE_ALLOW,
    TRACE_DUMP_DENY,
    TRACE_UPLOAD_DENY,
    TRACE_HAS_CAPTURED_TRACE
};

enum class TelemetryPolicy {
    DEFAULT,
    POWER,
    MANUAL
};

struct TraceRet {
    TraceRet() = default;

    explicit TraceRet(TraceStateCode stateError) : stateError_(stateError) {}

    explicit TraceRet(TraceErrorCode codeError) : codeError_(codeError) {}

    explicit TraceRet(TraceFlowCode codeError) : flowError_(codeError) {}

    TraceStateCode GetStateError()
    {
        return stateError_;
    }

    TraceErrorCode GetCodeError()
    {
        return codeError_;
    }

    TraceFlowCode GetFlowError()
    {
        return flowError_;
    }

    bool IsSuccess()
    {
        bool isStateSuccess = stateError_ == TraceStateCode::SUCCESS || stateError_ == TraceStateCode::NO_TRIGGER ||
            stateError_ == TraceStateCode::UPDATE_TIME;
        return isStateSuccess && codeError_ == TraceErrorCode::SUCCESS && flowError_ == TraceFlowCode::TRACE_ALLOW;
    }

    TraceStateCode stateError_ = TraceStateCode::SUCCESS;
    TraceErrorCode codeError_ = TraceErrorCode::SUCCESS;
    TraceFlowCode flowError_ = TraceFlowCode::TRACE_ALLOW;
};

class TelemetryCallback {
public:
    virtual ~TelemetryCallback() = default;
    virtual void OnTelemetryStart() = 0;
    virtual void OnTelemetryFinish() = 0;
    virtual void OnTelemetryTraceOn() = 0;
    virtual void OnTelemetryTraceOff() = 0;
};
}
}
#endif // HIVIEWDFX_HIVIEW_TRACE_UTIL_H
