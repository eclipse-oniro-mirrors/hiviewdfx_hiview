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
#ifndef FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_FLOW_CONTROLLER_H
#define FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_FLOW_CONTROLLER_H

#include <memory>
#include <string>

#include "app_caller_event.h"
#include "hitrace_dump.h"
#include "trace_storage.h"
#include "telemetry_storage.h"
#include "trace_behavior_storage.h"

using OHOS::HiviewDFX::Hitrace::TraceErrorCode;
using OHOS::HiviewDFX::Hitrace::TraceRetInfo;

namespace OHOS {
namespace HiviewDFX {
namespace {
const std::string DB_PATH = "/data/log/hiview/unified_collection/trace/";
}

enum class CacheFlow {
    SUCCESS,
    OVER_FLOW,
    EXIT
};

class TraceFlowController {
public:
    explicit TraceFlowController(const std::string &caller, const std::string& dbPath = DB_PATH);
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
    void CleanOldAppTrace(int32_t dateNum);

    CacheFlow UseCacheTimeQuota(int32_t interval);

    TelemetryFlow InitTelemetryData(const std::map<std::string, int64_t> &flowControlQuotas, int64_t &beginTime,
        int64_t &endTime);

    void ClearTelemetryData();

    TelemetryFlow NeedTelemetryDump(const std::string& module, int64_t traceSize);

private:
    void InitTraceDb(const std::string& dbPath);
    void InitTraceStorage(const std::string& caller);

private:
    std::shared_ptr<NativeRdb::RdbStore> dbStore_;
    std::shared_ptr<TraceStorage> traceStorage_;
    std::shared_ptr<AppEventTaskStorage> appTaskStore_;
    std::shared_ptr<TraceBehaviorStorage> behaviorTaskStore_;
    std::shared_ptr<TeleMetryStorage> teleMetryStorage_;
};
} // HiViewDFX
} // OHOS
#endif // FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_FLOW_CONTROLLER_H
