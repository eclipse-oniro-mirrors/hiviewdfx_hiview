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
#include <fstream>
#include <memory>
#include <mutex>
#include <string>

#include "hiview_logger.h"
#include "trace_decorator.h"
#include "trace_flow_controller.h"
#include "trace_manager.h"

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
constexpr uint32_t MB_TO_KB = 1024;
constexpr uint32_t KB_TO_BYTE = 1024;
}

std::shared_ptr<TraceCollector> TraceCollector::Create()
{
    return std::make_shared<TraceDecorator>(std::make_shared<TraceCollectorImpl>());
}

CollectResult<std::vector<std::string>> TraceCollectorImpl::DumpTraceWithDuration(
    UCollect::TraceCaller &caller, uint32_t timeLimit, uint64_t happenTime)
{
    if (timeLimit > INT32_MAX) {
        return StartDumpTrace(caller, INT32_MAX, happenTime);
    }
    return StartDumpTrace(caller, static_cast<int32_t>(timeLimit), happenTime);
}

CollectResult<std::vector<std::string>> TraceCollectorImpl::DumpTrace(UCollect::TraceCaller &caller)
{
    return StartDumpTrace(caller, FULL_TRACE_DURATION, static_cast<uint64_t>(0));
}

CollectResult<std::vector<std::string>> TraceCollectorImpl::StartDumpTrace(UCollect::TraceCaller &caller,
    int32_t timeLimit, uint64_t happenTime)
{
    HIVIEW_LOGI("trace caller is %{public}s.", EnumToString(caller).c_str());
    CollectResult<std::vector<std::string>> result;
    if (OHOS::HiviewDFX::Hitrace::GetTraceMode() != OHOS::HiviewDFX::Hitrace::TraceMode::SERVICE_MODE) {
        result.retCode = UcError::UNSUPPORT;
        HIVIEW_LOGI("hitrace service not permitted to load on current version");
        return result;
    }
    std::lock_guard<std::mutex> lock(g_dumpTraceMutex);
    std::shared_ptr<TraceFlowController> controlPolicy = std::make_shared<TraceFlowController>(caller);
    // check 1, judge whether need to dump
    if (!controlPolicy->NeedDump()) {
        result.retCode = UcError::TRACE_OVER_FLOW;
        HIVIEW_LOGI("trace is over flow, can not dump.");
        return result;
    }
    DumpEvent dumpEvent;
    TraceRetInfo traceRetInfo = RecordTraceEvent(dumpEvent, caller, timeLimit, happenTime);
    int64_t traceSize = GetTraceSize(traceRetInfo);
    if (traceSize <= INT32_MAX * MB_TO_KB * KB_TO_BYTE) {
        dumpEvent.fileSize = traceSize / MB_TO_KB / KB_TO_BYTE;
    }
    // check 2, judge whether to upload or not
    if (!controlPolicy->NeedUpload(traceSize)) {
        result.retCode = UcError::TRACE_OVER_FLOW;
        HIVIEW_LOGI("trace is over flow, can not upload.");
        dumpEvent.errorCode = result.retCode;
        WriteDumpTraceHisysevent(dumpEvent);
        return result;
    }
    if (traceRetInfo.errorCode == TraceErrorCode::SUCCESS) {
        if (caller == UCollect::TraceCaller::DEVELOP) {
            result.data = traceRetInfo.outputFiles;
        } else {
            std::vector<std::string> outputFiles = GetUnifiedFiles(traceRetInfo, caller);
            result.data = outputFiles;
        }
    }
    result.retCode = TransCodeToUcError(traceRetInfo.errorCode);
    dumpEvent.errorCode = result.retCode;
    WriteDumpTraceHisysevent(dumpEvent);
    // step3： update db
    controlPolicy->StoreDb();
    HIVIEW_LOGI("DumpTrace, retCode = %{public}d, data.size = %{public}zu.", result.retCode, result.data.size());
    return result;
}

TraceRetInfo TraceCollectorImpl::RecordTraceEvent(DumpEvent &dumpEvent, UCollect::TraceCaller &caller,
    int32_t timeLimit, uint64_t happenTime)
{
    TraceRetInfo traceRetInfo;
    dumpEvent.caller = EnumToString(caller);
    dumpEvent.reqDuration = timeLimit;
    dumpEvent.reqTime = happenTime;
    dumpEvent.execTime = TimeUtil::GenerateTimestamp();
    auto start = std::chrono::steady_clock::now();
    if (timeLimit == FULL_TRACE_DURATION) {
        traceRetInfo = OHOS::HiviewDFX::Hitrace::DumpTrace(0, happenTime);
    } else {
        traceRetInfo = OHOS::HiviewDFX::Hitrace::DumpTrace(timeLimit, happenTime);
    }
    auto end = std::chrono::steady_clock::now();
    dumpEvent.execDuration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    dumpEvent.coverDuration = traceRetInfo.coverDuration;
    dumpEvent.coverRatio = traceRetInfo.coverRatio;
    dumpEvent.tagGroup = traceRetInfo.tagGroup;
    LoadMemoryInfo(dumpEvent);
    return traceRetInfo;
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
