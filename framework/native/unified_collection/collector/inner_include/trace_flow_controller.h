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
#ifndef FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_FLOW_CONTROLLER_H
#define FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_FLOW_CONTROLLER_H

#include <memory>
#include <string>

#include "hitrace_dump.h"
#include "trace_collector.h"
#include "trace_storage.h"
#include "app_caller_event.h"

using OHOS::HiviewDFX::Hitrace::TraceErrorCode;
using OHOS::HiviewDFX::Hitrace::TraceRetInfo;
using OHOS::HiviewDFX::UCollect::UcError;

namespace OHOS {
namespace HiviewDFX {
class TraceFlowController {
public:
    TraceFlowController(UCollect::TraceCaller caller = UCollect::TraceCaller::INVALIDITY);
    ~TraceFlowController() = default;
    bool NeedDump();
    bool NeedUpload(int64_t traceSize);
    void StoreDb();

    /**
     * @brief app whether report jank event trace today
     *
     * @param uid app user id
     * @param happenTime main thread jank happen time, millisecond
     * @return true: has report trace event today; false: has not report trace event today
     */
    bool HasCallOnceToday(int32_t uid, uint64_t happenTime);

    /**
     * @brief save who capture trace
     *
     * @param appEvent app caller
     * @return true: save success; false: save fail
     */
    bool RecordCaller(std::shared_ptr<AppCallerEvent> appEvent);

    /**
     * @brief clean which remain in share create by app
     *
     */
    void CleanOldAppTrace();
private:
    void InitTraceDb();
    void InitTraceStorage();
    std::string GetDate();
    bool IsLowerLimit(int64_t nowSize, int64_t traceSize, int64_t limitSize);
    TraceFlowRecord QueryDb();

private:
    struct TraceFlowRecord traceFlowRecord_;
    std::shared_ptr<TraceStorage> traceStorage_;
    UCollect::TraceCaller caller_;
};
} // HiViewDFX
} // OHOS
#endif // FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_FLOW_CONTROLLER_H
