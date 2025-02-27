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

#ifndef FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_CACHE_MONITOR_H
#define FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_CACHE_MONITOR_H

#include "memory_collector.h"
#include "trace_behavior_storage.h"
#include "trace_flow_controller.h"

namespace OHOS {
namespace HiviewDFX {

namespace {
enum TraceCacheMonitorState {
    EXIT = 0,
    RUNNING = 1,
    INTERRUPT = 2,
};
}

class TraceCacheMonitor {
public:
    TraceCacheMonitor();
    ~TraceCacheMonitor();
    void SetLowMemThreshold(int32_t threshold);
    void RunMonitorLoop();
    void ExitMonitorLoop();
private:
    void MonitorFfrtTask();
    void RunMonitorCycle(int32_t interval);
    bool IsLowMemState();
    void SetCacheOn();
    void SetCacheOff();
    void CountDownCacheOff();

private:
    std::mutex stateMutex_;
    TraceCacheMonitorState monitorState_ = EXIT;
    std::shared_ptr<UCollectUtil::MemoryCollector> collector_;
    std::shared_ptr<TraceFlowController> flowController_;
    bool isCacheOn_ = false;
    bool isWaitingForRecovery_ = false;
    int32_t lowMemThreshold_ = 0;
    int32_t cacheDuration_ = 0;
    int32_t cacheOffCountdown_ = 0;
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_CACHE_MONITOR_H
