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

#include "trace_collector_impl.h"

#include <climits>
#include <memory>
#include <mutex>

#include "hiview_logger.h"
#include "parameter_ex.h"
#include "trace_decorator.h"
#include "trace_flow_controller.h"
#include "trace_manager.h"
#include "trace_utils.h"

using namespace OHOS::HiviewDFX;
using OHOS::HiviewDFX::TraceFlowController;
using namespace OHOS::HiviewDFX::Hitrace;
using namespace OHOS::HiviewDFX::UCollectUtil;
using namespace OHOS::HiviewDFX::UCollect;

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
namespace {
DEFINE_LOG_TAG("UCollectUtil-TraceCollector");
std::mutex g_dumpTraceMutex;
constexpr int32_t FULL_TRACE_DURATION = -1;
}

std::shared_ptr<TraceCollector> TraceCollector::Create()
{
    return std::make_shared<TraceDecorator>(std::make_shared<TraceCollectorImpl>());
}

CollectResult<std::vector<std::string>> TraceCollectorImpl::DumpTraceWithDuration(
    TraceCollector::Caller &caller, uint32_t timeLimit)
{
    if (timeLimit > INT32_MAX) {
        return StartDumpTrace(caller, INT32_MAX);
    }
    return StartDumpTrace(caller, static_cast<int32_t>(timeLimit));
}

CollectResult<std::vector<std::string>> TraceCollectorImpl::DumpTrace(TraceCollector::Caller &caller)
{
    return StartDumpTrace(caller, FULL_TRACE_DURATION);
}

CollectResult<std::vector<std::string>> TraceCollectorImpl::StartDumpTrace(TraceCollector::Caller &caller,
    int32_t timeLimit)
{
    HIVIEW_LOGI("trace caller is %{public}s.", EnumToString(caller).c_str());
    CollectResult<std::vector<std::string>> result;
    if (!Parameter::IsBetaVersion() && !Parameter::IsUCollectionSwitchOn()) {
        result.retCode = UcError::UNSUPPORT;
        HIVIEW_LOGI("hitrace service not permitted to load on current version");
        return result;
    }
    std::lock_guard<std::mutex> lock(g_dumpTraceMutex);
    std::shared_ptr<TraceFlowController> controlPolicy = std::make_shared<TraceFlowController>();
    // check 1, judge whether need to dump
    if (!controlPolicy->NeedDump(caller)) {
        result.retCode = UcError::TRACE_OVER_FLOW;
        HIVIEW_LOGI("trace is over flow, can not dump.");
        return result;
    }

    TraceRetInfo traceRetInfo;
    if (timeLimit == FULL_TRACE_DURATION) {
        traceRetInfo = OHOS::HiviewDFX::Hitrace::DumpTrace();
    } else {
        traceRetInfo = OHOS::HiviewDFX::Hitrace::DumpTrace(timeLimit);
    }
    // check 2, judge whether to upload or not
    if (!controlPolicy->NeedUpload(caller, traceRetInfo)) {
        result.retCode = UcError::TRACE_OVER_FLOW;
        HIVIEW_LOGI("trace is over flow, can not upload.");
        return result;
    }
    if (traceRetInfo.errorCode == TraceErrorCode::SUCCESS) {
        if (caller == TraceCollector::Caller::DEVELOP) {
            result.data = traceRetInfo.outputFiles;
        } else {
            std::vector<std::string> outputFiles = GetUnifiedFiles(traceRetInfo, caller);
            result.data = outputFiles;
        }
    }

    result.retCode = TransCodeToUcError(traceRetInfo.errorCode);
    // step3： update db
    controlPolicy->StoreDb();
    HIVIEW_LOGI("DumpTrace, retCode = %{public}d, data.size = %{public}zu.", result.retCode, result.data.size());
    return result;
}

CollectResult<int32_t> TraceCollectorImpl::TraceOn()
{
    CollectResult<int32_t> result;
    TraceErrorCode ret = OHOS::HiviewDFX::Hitrace::DumpTraceOn();
    result.retCode = TransCodeToUcError(ret);
    HIVIEW_LOGI("TraceOn, ret = %{public}d.", result.retCode);
    return result;
}

CollectResult<std::vector<std::string>> TraceCollectorImpl::TraceOff()
{
    CollectResult<std::vector<std::string>> result;
    TraceRetInfo ret = OHOS::HiviewDFX::Hitrace::DumpTraceOff();
    if (ret.errorCode == TraceErrorCode::SUCCESS) {
        result.data = ret.outputFiles;
    }
    result.retCode = TransCodeToUcError(ret.errorCode);
    HIVIEW_LOGI("TraceOff, ret = %{public}d, data.size = %{public}zu.", result.retCode, result.data.size());
    return result;
}
} // UCollectUtil
} // HiViewDFX
} // OHOS
