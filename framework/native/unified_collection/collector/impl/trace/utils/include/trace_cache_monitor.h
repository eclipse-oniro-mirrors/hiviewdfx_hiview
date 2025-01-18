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
#include "trace_behavior_controller.h"

namespace OHOS {
namespace HiviewDFX {

class TraceCacheMonitor {
public:
    TraceCacheMonitor(int32_t lowMemThreshold);
    ~TraceCacheMonitor();
    void RunMonitorCircle(int32_t interval);
    bool UseCacheTimeQuota(int32_t usedQuota);
private:
    void SetCacheOn();
    void SetCacheOff();
    void CountDownCacheOff();
    bool IsCacheOn();
    bool IsLowMemState();
    void SleepandUpdateCacheStatus(int32_t interval);

private:
    TraceBehaviorController behaviorController_;
    std::shared_ptr<UCollectUtil::MemoryCollector> collector_;
    int32_t lowMemThreshold_ = 0;
    bool isCacheOn_ = false;
    bool isWaitingForNormal_ = false;
    int32_t cacheDuration_ = 0;
    int32_t cacheOffCountdown_ = 0;
}; 

} // namespace HiviewDFX
} // namespace OHOS
#endif // FRAMEWORK_NATIVE_UNIFIED_COLLECTION_COLLECTOR_TRACE_CACHE_MONITOR_H
