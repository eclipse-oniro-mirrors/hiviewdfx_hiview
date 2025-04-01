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

namespace CallerName {
const std::string XPERF = "Xperf";
const std::string XPOWER = "Xpower";
const std::string RELIABILITY = "Reliability";
const std::string HIVIEW = "Hiview";
const std::string OTHER = "Other";
const std::string SCREEN = "Screen";
};

namespace ClientName {
const std::string COMMAND = "Command";
const std::string COMMON_DEV = "Other";
const std::string APP = "APP";
const std::string BETACLUB = "BetaClub";
};

namespace BusinessName {
const std::string BEHAVIOR = "behavior";
const std::string TELEMETRY = "Telemetry";
}

enum class TraceScenario : uint8_t {
    TRACE_COMMAND,
    TRACE_COMMON,
    TRACE_DYNAMIC,
    TRACE_TELEMETRY,
};

enum class TraceStateCode : uint8_t {
    SUCCESS,
    DENY, // Change state deny
    FAIL // Invoke dump or drop interface in wrong state
};

enum class TraceFlowCode : uint8_t {
    TRACE_ALLOW,
    TRACE_DUMP_DENY,
    TRACE_UPLOAD_DENY,
    TRACE_HAS_CAPTURED_TRACE
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
        return stateError_ == TraceStateCode::SUCCESS && codeError_ == TraceErrorCode::SUCCESS &&
            flowError_ == TraceFlowCode::TRACE_ALLOW;
    }

    TraceStateCode stateError_ = TraceStateCode::SUCCESS;
    TraceErrorCode codeError_ = TraceErrorCode::SUCCESS;
    TraceFlowCode flowError_ = TraceFlowCode::TRACE_ALLOW;
};
}
}
#endif // HIVIEWDFX_HIVIEW_TRACE_UTIL_H
